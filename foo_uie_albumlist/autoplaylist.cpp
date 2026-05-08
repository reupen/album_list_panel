#include "stdafx.h"

#include "main_tree_builder.h"

namespace alp {

namespace {

constexpr GUID by_autoplaylist_client_id{0x148e08f0, 0x4ab7, 0x4914, {0xb5, 0x6e, 0xb0, 0x4c, 0xb1, 0x33, 0x05, 0x64}};

bool compare_part(std::string_view text, std::string_view expected)
{
    if (text.find(alp::branch_marker) == std::string::npos)
        return stricmp_utf8_ex(text.data(), text.size(), expected.data(), expected.size()) == 0;

    auto branches = alp::get_branches(text);
    std::string buffer;

    for (size_t branch_index{0}; branch_index < branches.branch_count; branch_index++) {
        buffer.clear();

        for (const auto& segment : branches.segments) {
            const auto& choice = branches.choices[segment.m_current_choice];
            buffer.append(&text[choice.m_start], choice.m_end - choice.m_start);
        }

        if (stricmp_utf8_ex(buffer.data(), buffer.size(), expected.data(), expected.size()) == 0)
            return true;

        for (auto& segment : branches.segments | std::views::reverse) {
            segment.m_current_choice++;

            if (segment.m_current_choice < segment.m_choices_end)
                break;

            segment.m_current_choice = segment.m_choices_begin;
        }
    }

    return false;
}

std::string get_autoplaylist_name(auto&& labels_by_node)
{
    auto leaf_labels = labels_by_node | ranges::views::take(2) | ranges::views::transform([](auto&& labels) {
        const auto leaf = labels.back();

        if (leaf.empty())
            return "(blank)"sv;

        return std::string_view(labels.back());
    }) | ranges::to_vector;

    if (labels_by_node.size() > 2)
        leaf_labels.emplace_back("…"sv);

    return mmh::join<const decltype(leaf_labels)&, std::string_view, std::string>(leaf_labels, ", "sv).c_str();
}

} // namespace

class ByFormatAutoplaylistClient : public autoplaylist_client_v3 {
public:
    ByFormatAutoplaylistClient(wil::zstring_view view_titleformat, std::vector<std::vector<std::string>> node_labels,
        auto&& filter, std::string sort_titleformat)
        : m_view_titleformat(view_titleformat)
        , m_titleformat_object(titleformat_compiler::get()->compile(view_titleformat.c_str()))
        , m_labels_by_node(std::move(node_labels))
        , m_filter(std::forward<decltype(filter)>(filter))
        , m_sort_titleformat(std::move(sort_titleformat))
    {
    }

    GUID get_guid() override { return by_autoplaylist_client_id; }

    void get_configuration(stream_writer* p_stream, abort_callback& p_abort) override
    {
        p_stream->write_lendian_t(false, p_abort);
        p_stream->write_string(m_view_titleformat.c_str(), p_abort);
        p_stream->write_string(m_filter.c_str(), p_abort);
        p_stream->write_lendian_t(false, p_abort);
        p_stream->write_string(m_sort_titleformat.c_str(), p_abort);
        p_stream->write_lendian_t(gsl::narrow<uint32_t>(m_labels_by_node.size()), p_abort);

        for (const auto& node_labels : m_labels_by_node) {
            p_stream->write_lendian_t(gsl::narrow<uint32_t>(node_labels.size()), p_abort);

            for (const auto& label : node_labels)
                p_stream->write_string(label.c_str(), p_abort);
        }
    }

    void show_ui(t_size p_source_playlist) override {}
    void set_full_refresh_notify(completion_notify::ptr notify) override { m_completion_notify = notify; }
    bool show_ui_available() override { return false; }
    void get_display_name(pfc::string_base& out) override { out = "Album list panel"; }

    bool supports_async() override { return true; }
    void filter_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data* dataIfAvailable, bool* out,
        abort_callback& abort) override
    {
        if (!m_filter.empty()) {
            const auto filter_obj
                = search_filter_manager_v2::get()->create_ex(m_filter.c_str(), m_completion_notify, 0);
            filter_obj->test_multi_ex(items, out, abort);
        } else {
            std::fill_n(out, items.size(), true);
        }

        concurrency::combinable<std::string> format_buffer;
        concurrency::combinable<std::string> unescaped_segment_buffer;
        concurrency::combinable<std::vector<std::string_view>> segments_buffer;

        concurrency::parallel_for(size_t{}, items.size(),
            [this, &items, &out, &dataIfAvailable, &format_buffer, &unescaped_segment_buffer, &segments_buffer](
                size_t index) {
                metadb_info_container::ptr info_ptr;
                metadb_v2_rec_t rec;

                if (!out[index]) {
                    return;
                }

                if (dataIfAvailable) {
                    rec = dataIfAvailable->get(index);
                    if (!rec.info.is_valid()) {
                        out[index] = false;
                        return;
                    }
                } else if (!items[index]->get_info_ref(info_ptr)) {
                    out[index] = false;
                    return;
                }

                const playable_location& location = items[index]->get_location();
                MetaBranchTitleformatHook tf_hook_file_info(
                    location, dataIfAvailable ? &rec.info->info() : &info_ptr->info());
                VerticalBarTitleformatTextFilter tf_hook_text_filter;

                auto& formatted_title = format_buffer.local();
                mmh::StringAdaptor interop_title(formatted_title);
                items[index]->format_title(
                    &tf_hook_file_info, interop_title, m_titleformat_object, &tf_hook_text_filter);

                auto& segments = segments_buffer.local();
                segments.clear();

                auto segments_view = formatted_title | std::views::split("|"sv)
                    | std::views::transform([](auto&& range) { return std::string_view(range.data(), range.size()); });

                ranges::copy(segments_view, std::back_inserter(segments));

                auto& unescaped_segment = unescaped_segment_buffer.local();

                out[index] = ranges::any_of(m_labels_by_node, [&](auto&& labels) {
                    if (segments.size() < labels.size())
                        return false;

                    return ranges::all_of(ranges::views::zip(segments, labels), [&](const auto& pair) {
                        auto& [segment, expected] = pair;
                        unescape_vertical_bar(segment, unescaped_segment);
                        return compare_part(unescaped_segment, expected);
                    });
                });
            });
    }

    bool sort_v2(metadb_handle_list_cref p_items, t_size* p_orderbuffer, abort_callback& abort) override
    {
        if (m_sort_titleformat.empty())
            return false;

        service_ptr_t<titleformat_object> sort_script;
        titleformat_compiler::get()->compile_safe_ex(sort_script, m_sort_titleformat.c_str());

        std::span permutation(p_orderbuffer, p_items.size());
        mmh::fill_identity_permutation(permutation);
        fbh::sort_metadb_handle_list_by_format_get_permutation(p_items, permutation, sort_script, nullptr);

        return true;
    }

    bool supports_get_contents() override { return false; }

    fb2k::arrayRef get_contents(abort_callback& a) override { return {}; }

private:
    std::string m_view_titleformat;
    titleformat_object::ptr m_titleformat_object;
    std::vector<std::vector<std::string>> m_labels_by_node;
    std::string m_filter;
    std::string m_sort_titleformat;
    completion_notify::ptr m_completion_notify;
};

class ByDirAutoplaylistClient : public autoplaylist_client_v3 {
public:
    ByDirAutoplaylistClient(
        std::vector<std::vector<std::string>> node_labels, auto&& filter, std::string sort_titleformat)
        : m_labels_by_node(std::move(node_labels))
        , m_filter(std::forward<decltype(filter)>(filter))
        , m_sort_titleformat(std::move(sort_titleformat))
    {
    }

    GUID get_guid() override { return by_autoplaylist_client_id; }

    void get_configuration(stream_writer* p_stream, abort_callback& p_abort) override
    {
        p_stream->write_lendian_t(true, p_abort);
        p_stream->write_string(m_filter.c_str(), p_abort);
        p_stream->write_lendian_t(false, p_abort);
        p_stream->write_string(m_sort_titleformat.c_str(), p_abort);
        p_stream->write_lendian_t(gsl::narrow<uint32_t>(m_labels_by_node.size()), p_abort);

        for (const auto& node_labels : m_labels_by_node) {
            p_stream->write_lendian_t(gsl::narrow<uint32_t>(node_labels.size()), p_abort);

            for (const auto& label : node_labels)
                p_stream->write_string(label.c_str(), p_abort);
        }
    }

    void show_ui(t_size p_source_playlist) override {}
    void set_full_refresh_notify(completion_notify::ptr notify) override { m_completion_notify = notify; }
    bool show_ui_available() override { return false; }
    void get_display_name(pfc::string_base& out) override { out = "Album list panel"; }

    bool supports_async() override { return true; }
    void filter_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data* dataIfAvailable, bool* out,
        abort_callback& abort) override
    {
        if (!m_filter.empty()) {
            const auto filter_obj
                = search_filter_manager_v2::get()->create_ex(m_filter.c_str(), m_completion_notify, 0);
            filter_obj->test_multi_ex(items, out, abort);
        } else {
            std::fill_n(out, items.size(), true);
        }

        const auto library_api = library_manager::get();
        concurrency::combinable<std::vector<std::string_view>> segments_buffer;

        concurrency::parallel_for(
            size_t{}, items.size(), [this, &items, &out, &library_api, &segments_buffer](size_t index) {
                metadb_info_container::ptr info_ptr;
                metadb_v2_rec_t rec;

                if (!out[index]) {
                    return;
                }

                std::string path;
                mmh::StringAdaptor adapted_path(path);
                library_api->get_relative_path(items[index], adapted_path);

                auto& segments = segments_buffer.local();
                segments.clear();

                auto segments_view = path | ranges::views::split_when([](char c) {
                    return alp::path_separators.find(c) != std::string_view::npos;
                }) | ranges::views::transform([](auto&& range) {
                    return std::string_view(&*range.begin(), std::ranges::distance(range));
                });

                ranges::copy(segments_view, std::back_inserter(segments));

                out[index] = ranges::any_of(m_labels_by_node, [&](auto&& labels) {
                    if (segments.size() < labels.size())
                        return false;

                    return ranges::all_of(ranges::views::zip(segments, labels), [](const auto& pair) {
                        auto& [segment, expected] = pair;
                        return metadb::path_compare_ex(segment.data(), segment.size(), expected.data(), expected.size())
                            == 0;
                    });
                });
            });
    }

    bool sort_v2(metadb_handle_list_cref p_items, t_size* p_orderbuffer, abort_callback& abort) override
    {
        if (m_sort_titleformat.empty())
            return false;

        service_ptr_t<titleformat_object> sort_script;
        titleformat_compiler::get()->compile_safe_ex(sort_script, "%path_sort%");

        std::span permutation(p_orderbuffer, p_items.size());
        mmh::fill_identity_permutation(permutation);
        fbh::sort_metadb_handle_list_by_format_get_permutation(p_items, permutation, sort_script, nullptr);

        return true;
    }

    bool supports_get_contents() override { return false; }

    fb2k::arrayRef get_contents(abort_callback& a) override { return {}; }

private:
    std::vector<std::vector<std::string>> m_labels_by_node;
    std::string m_filter;
    std::string m_sort_titleformat;
    completion_notify::ptr m_completion_notify;
};

namespace {

class AutoplaylistClientFactory : public autoplaylist_client_factory {
public:
    GUID get_guid() override { return by_autoplaylist_client_id; }
    autoplaylist_client_ptr instantiate(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override
    {
        const auto is_by_dir = p_stream->read_lendian_t<bool>(p_abort);
        const auto view_titleformat = is_by_dir ? pfc::string8() : p_stream->read_string(p_abort);
        const auto filter = p_stream->read_string(p_abort);

        [[maybe_unused]] const auto is_force_sorted = p_stream->read_lendian_t<bool>(p_abort);

        std::string sort_titleformat;
        mmh::StringAdaptor adapted_sort_titleformat(sort_titleformat);
        p_stream->read_string(adapted_sort_titleformat, p_abort);

        const auto node_count = p_stream->read_lendian_t<uint32_t>(p_abort);
        std::vector<std::vector<std::string>> labels_by_node(node_count);

        for (auto& node_labels : labels_by_node) {
            const auto label_count = p_stream->read_lendian_t<uint32_t>(p_abort);
            node_labels.resize(label_count);

            for (auto& label : node_labels)
                label = p_stream->read_string(p_abort);
        }

        if (is_by_dir)
            return fb2k::service_new<ByDirAutoplaylistClient>(
                std::move(labels_by_node), std::move(filter), std::move(sort_titleformat));

        return fb2k::service_new<ByFormatAutoplaylistClient>(
            view_titleformat, std::move(labels_by_node), std::move(filter), std::move(sort_titleformat));
    }
};

service_factory_t<AutoplaylistClientFactory> _autoplaylist_client_factory;

} // namespace

void create_autoplaylist(bool is_by_dir, wil::zstring_view view_titleformat,
    std::vector<std::vector<wil::zstring_view>> node_labels, std::string_view filter)
{
    assert(!node_labels.empty());

    if (node_labels.empty())
        return;

    const auto playlist_name = get_autoplaylist_name(node_labels);

    const auto playlist_api = playlist_manager_v4::get();
    const auto playlist_index = playlist_api->create_playlist(playlist_name.c_str(), playlist_name.size(), SIZE_MAX);

    const auto autoplaylist_api = autoplaylist_manager_v2::get();

    auto node_labels_strings = node_labels | ranges::to<std::vector<std::vector<std::string>>>;

    std::string sort_titleformat;

    if (!cfg_add_items_use_core_sort) {
        if (is_by_dir)
            sort_titleformat = "%path_sort%";
        else
            std::format_to(std::back_inserter(sort_titleformat), "{}|%path_sort%", view_titleformat.c_str());
    }

    auto client = is_by_dir ? autoplaylist_client::ptr(fb2k::service_new<ByDirAutoplaylistClient>(
                                  std::move(node_labels_strings), filter, std::move(sort_titleformat)))
                            : autoplaylist_client::ptr(fb2k::service_new<ByFormatAutoplaylistClient>(view_titleformat,
                                  std::move(node_labels_strings), filter, std::move(sort_titleformat)));

    autoplaylist_api->add_client(std::move(client), playlist_index, 0);

    playlist_api->set_active_playlist(playlist_index);
}

} // namespace alp
