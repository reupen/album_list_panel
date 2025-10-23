#include "stdafx.h"
#include "tree_view_populator.h"
#include "utils.h"

namespace {

constexpr auto vertical_bar = "|"sv;
constexpr auto vertical_bar_replacement = "\uEFA0"sv;

std::string unescape_vertical_bar(std::string_view text)
{
    return alp::utils::replace_substring(text, vertical_bar_replacement, vertical_bar);
}

template <typename String>
const char* c_str(String& string)
{
    return string.c_str();
}

template <>
const char* c_str(const char* const& string)
{
    return string;
}

} // namespace

class VerticalBarTitleformatTextFilter : public titleformat_text_filter {
public:
    void write(const GUID& p_inputtype, pfc::string_receiver& p_out, const char* p_data, t_size p_data_length) override
    {
        // titleformat_text_filter_impl_reserved_chars only filters for titleformat_inputtypes::meta
        if (p_inputtype != titleformat_inputtypes::meta) {
            p_out.add_string(p_data, p_data_length);
            return;
        }

        const size_t real_length = strnlen(p_data, p_data_length);

        const std::string_view input{p_data, real_length};
        size_t start{};

        while (start < real_length) {
            const size_t end = input.find(vertical_bar, start);

            if (end == std::string_view::npos) {
                p_out.add_string(p_data + start, p_data_length - start);
                break;
            }

            p_out.add_string(p_data + start, end - start);
            p_out.add_string(vertical_bar_replacement.data(), vertical_bar_replacement.size());
            start = end + 1;
        }
    }
};

struct process_bydir_entry {
    metadb_handle* m_track{};
    std::string_view m_path;

    process_bydir_entry() {}

    process_bydir_entry(metadb_handle* m_track, std::string_view m_path) : m_track(m_track), m_path(std::move(m_path))
    {
    }

    const std::vector<std::string_view>& segments() const
    {
        if (!m_segments) {
            m_segments = ranges::views::split_when(m_path, [](char c) {
                return "\\|/"sv.find(c) != std::string_view::npos;
            }) | ranges::views::transform([](auto&& range) {
                return std::string_view(&*range.begin(), std::ranges::distance(range));
            }) | ranges::to<std::vector>;
        }

        return *m_segments;
    }

    static int s_compare_segment(std::string_view p_path1, std::string_view p_path2)
    {
        return metadb::path_compare_ex(p_path1.data(), p_path1.size(), p_path2.data(), p_path2.size());
    }

    static int s_compare(const process_bydir_entry& p_item1, const process_bydir_entry& p_item2)
    {
        return metadb::path_compare_ex(
            p_item1.m_path.data(), p_item1.m_path.size(), p_item2.m_path.data(), p_item2.m_path.size());
    }

    enum {
        is_bydir = true
    };

private:
    mutable std::optional<std::vector<std::string_view>> m_segments;
};

struct process_byformat_entry {
    metadb_handle* m_track{};
    std::string m_path;

    process_byformat_entry() {}

    process_byformat_entry(metadb_handle* m_track, std::string m_path) : m_track(m_track), m_path(std::move(m_path)) {}

    process_byformat_entry(const process_byformat_entry& other) : m_track(other.m_track), m_path(other.m_path)
    {
        assert(!other.m_segments);
    }

    process_byformat_entry(process_byformat_entry&& other) noexcept
        : m_track(other.m_track)
        , m_path(std::move(other.m_path))
    {
        assert(!other.m_segments);
    }

    process_byformat_entry& operator=(const process_byformat_entry& other)
    {
        if (this == &other)
            return *this;

        assert(!other.m_segments);
        m_track = other.m_track;
        m_path = other.m_path;
        return *this;
    }

    process_byformat_entry& operator=(process_byformat_entry&& other) noexcept
    {
        if (this == &other)
            return *this;

        assert(!other.m_segments);
        m_track = other.m_track;
        m_path = std::move(other.m_path);
        return *this;
    }

    const std::vector<std::string_view>& segments() const
    {
        if (!m_segments) {
            m_segments = m_path | std::views::split("|"sv)
                | std::views::transform([](auto&& range) { return std::string_view(range.data(), range.size()); })
                | ranges::to<std::vector>;
        }

        return *m_segments;
    }

    static int s_compare_segment(std::string_view p_path1, std::string_view p_path2)
    {
        return stricmp_utf8_ex(p_path1.data(), p_path1.size(), p_path2.data(), p_path2.size());
    }

    static int s_compare(const process_byformat_entry& p_item1, const process_byformat_entry& p_item2)
    {
        return stricmp_utf8(c_str(p_item1.m_path), c_str(p_item2.m_path));
    }

    enum {
        is_bydir = false
    };

private:
    mutable std::optional<std::vector<std::string_view>> m_segments;
};

template <typename t_entry>
class ProcessEntryListWrapper : public pfc::list_base_const_t<metadb_handle_ptr> {
public:
    explicit ProcessEntryListWrapper(std::span<const t_entry> items) : m_items(items) {}

    size_t get_count() const override { return m_items.size(); }

    void get_item_ex(metadb_handle_ptr& p_out, size_t n) const override { p_out = m_items[n].m_track; }

private:
    std::span<const t_entry> m_items;
};

template <typename t_entry>
static void process_level_recur_t(std::span<const t_entry> items, node_ptr p_parent, bool b_add_only, size_t level = 0)
{
    p_parent->set_bydir(t_entry::is_bydir);
    p_parent->set_data(ProcessEntryListWrapper<t_entry>(items), !b_add_only);

    assert(items.size() > 0);

    auto chunks = items | ranges::views::filter([level](const auto& item) { return level < item.segments().size(); })
        | ranges::views::chunk_by([level](const auto& left, const auto& right) {
              return t_entry::s_compare_segment(left.segments()[level], right.segments()[level]) == 0;
          });

    for (auto&& chunk : chunks) {
        const auto segment = chunk.front().segments()[level];

        bool b_new{};
        const auto next_parent = p_parent->find_or_add_child(unescape_vertical_bar(segment), !b_add_only, b_new);
        process_level_recur_t<t_entry>({&chunk.front(), gsl::narrow_cast<size_t>(ranges::distance(chunk))}, next_parent,
            b_add_only || b_new, level + 1);
    }
}

struct process_byformat_branch_segment {
    size_t m_choices_begin{};
    size_t m_choices_end{};
    size_t m_current_choice{};
};

struct process_byformat_branch_choice {
    size_t m_start{};
    size_t m_end{};
};

constexpr auto branch_marker = '\4';
constexpr auto branch_delimiter = '\5';

template <typename List>
size_t process_byformat_add_branches(metadb_handle* handle, std::string text, List& entries)
{
    const char* p_text = text.data();

    if (text.find(branch_marker) == std::string::npos) {
        entries.push_back({handle, std::move(text)});
        return 1;
    }

    size_t branch_count{1};

    std::vector<process_byformat_branch_segment> segments;
    std::vector<process_byformat_branch_choice> choices;

    // compute segments and branch count
    size_t pos{};
    while (p_text[pos] != 0) {
        // begin choice
        if (p_text[pos] == branch_marker) {
            pos++;

            const auto segment_first_choice = choices.size();

            auto choice_start = pos;

            while (p_text[pos] != 0 && p_text[pos] != branch_marker) {
                if (p_text[pos] == branch_delimiter) {
                    choices.emplace_back(choice_start, pos);
                    ++pos;
                    choice_start = pos;
                } else
                    ++pos;
            }

            choices.emplace_back(choice_start, pos);

            if (p_text[pos] == branch_marker)
                ++pos;

            segments.emplace_back(segment_first_choice, choices.size(), segment_first_choice);
            branch_count *= choices.size() - segment_first_choice;
        } else {
            const auto segment_first_choice = choices.size();
            const auto choice_start = pos;

            while (p_text[pos] != 0 && p_text[pos] != branch_marker)
                ++pos;

            choices.emplace_back(choice_start, pos);
            segments.emplace_back(segment_first_choice, choices.size(), segment_first_choice);
        }
    }

    // assemble branches
    for (size_t branch_index{0}; branch_index < branch_count; branch_index++) {
        std::string buffer;

        for (const auto& segment : segments) {
            const auto& choice = choices[segment.m_current_choice];
            buffer.append(&p_text[choice.m_start], choice.m_end - choice.m_start);
        }

        entries.push_back({handle, std::move(buffer)});

        for (auto& segment : segments | std::views::reverse) {
            segment.m_current_choice++;

            if (segment.m_current_choice < segment.m_choices_end)
                break;

            segment.m_current_choice = segment.m_choices_begin;
        }
    }

    return branch_count;
}

void AlbumListWindow::build_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& tracks, bool preserve_existing)
{
    m_filter_ptr.release();

    static_api_ptr_t<library_manager> api;
    pfc::string8 pattern;

    if (m_wnd_edit)
        uGetWindowText(m_wnd_edit, pattern);

    if (m_wnd_edit && !pattern.is_empty()) {
        const auto callback
            = [p_this = service_ptr_t<AlbumListWindow>{this}](auto&& code) { p_this->on_task_completion(0, code); };
        const auto completion_notify_ptr = fb2k::makeCompletionNotify(callback);

        try {
            m_filter_ptr
                = static_api_ptr_t<search_filter_manager_v2>()->create_ex(pattern, completion_notify_ptr, NULL);

            pfc::array_t<bool> mask;
            mask.set_count(tracks.get_count());
            m_filter_ptr->test_multi(tracks, mask.get_ptr());
            tracks.remove_mask(pfc::bit_array_not(pfc::bit_array_table(mask.get_ptr(), mask.get_count())));
        } catch (const pfc::exception&) {
        }
    }

    if (is_bydir()) {
        const size_t count = tracks.get_count();

        if (count > 0) {
            pfc::list_t<process_bydir_entry> entries;
            entries.set_size(count);
            pfc::array_t<pfc::string8> strings;
            strings.set_size(count);

            for (size_t n{0}; n < count; n++) {
                api->get_relative_path(tracks[n], strings[n]);
                entries[n].m_path = {strings[n], strings[n].length()};
                entries[n].m_track = tracks[n].get_ptr();
            }

            mmh::single_reordering_sort(entries, process_bydir_entry::s_compare, false);

            if (!preserve_existing || !m_root)
                m_root = std::make_shared<Node>("All music"s, this, 0);

            process_level_recur_t<process_bydir_entry>({entries.get_ptr(), count}, m_root, !preserve_existing);
        }
    } else {
        const size_t count{tracks.get_count()};

        if (count > 0) {
            service_ptr_t<titleformat_object> script;
            static_api_ptr_t<titleformat_compiler>()->compile_safe(script, get_hierarchy());
            concurrency::concurrent_vector<process_byformat_entry> entries;

            const auto metadb_v2_api = metadb_v2::tryGet();

            if (metadb_v2_api.is_valid()) {
                metadb_v2_api->queryMultiParallel_(
                    tracks, [&tracks, &script, &entries](size_t index, const metadb_v2::rec_t& rec) {
                        if (!rec.info.is_valid())
                            return;

                        metadb_handle_v2::ptr track;
                        track &= tracks[index];
                        const playable_location& location = track->get_location();

                        MetaBranchTitleformatHook tf_hook_file_info(location, &rec.info->info());
                        VerticalBarTitleformatTextFilter tf_hook_text_filter;
                        std::string formatted_title;
                        mmh::StringAdaptor interop_title(formatted_title);
                        track->formatTitle_v2(rec, &tf_hook_file_info, interop_title, script, &tf_hook_text_filter);
                        process_byformat_add_branches(track.get_ptr(), std::move(formatted_title), entries);
                    });
            } else {
                concurrency::parallel_for(size_t{0}, count, [&tracks, &script, &entries](size_t n) {
                    metadb_info_container::ptr info_ptr;

                    if (!tracks[n]->get_info_ref(info_ptr))
                        return;

                    const playable_location& location = tracks[n]->get_location();
                    MetaBranchTitleformatHook tf_hook_file_info(location, &info_ptr->info());
                    VerticalBarTitleformatTextFilter tf_hook_text_filter;
                    std::string formatted_title;
                    mmh::StringAdaptor interop_title(formatted_title);
                    tracks[n]->format_title(&tf_hook_file_info, interop_title, script, &tf_hook_text_filter);
                    process_byformat_add_branches(tracks[n].get_ptr(), std::move(formatted_title), entries);
                });
            }

            const size_t size = entries.size();

            pfc::list_t<process_byformat_entry> entries_sorted;
            entries_sorted.set_size(size);
            for (size_t i{0}; i < size; ++i)
                entries_sorted[i] = std::move(entries[i]);

            mmh::Permutation perm(size);
            mmh::sort_get_permutation(entries_sorted, perm, process_byformat_entry::s_compare, false, false, true);
            mmh::destructive_reorder(entries_sorted, perm);

            if (!preserve_existing || !m_root)
                m_root = std::make_shared<Node>("All music"s, this, 0);
            process_level_recur_t<process_byformat_entry>({entries_sorted.get_ptr(), size}, m_root, !preserve_existing);
        }
    }
    if (!preserve_existing && m_root) {
        m_root->sort_children();
    }
}

void g_node_remove_tracks_recur(const node_ptr& ptr, const metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks)
{
    if (!ptr)
        return;

    size_t count{ptr->get_tracks().get_count()};

    if (!count)
        return;

    size_t index{};
    bit_array_bittable mask(count);
    bool b_found{false};

    const metadb_handle_ptr* p_entries = ptr->get_tracks().get_ptr();
    const node_ptr* p_nodes = ptr->get_children().data();

    for (size_t i{0}; i < count; i++) {
        if (pfc::bsearch_simple_inline_t(p_tracks.get_ptr(), p_tracks.get_count(), p_entries[i], index)) {
            mask.set(i, true);
            b_found = true;
        }
    }

    if (b_found)
        ptr->remove_tracks(mask);

    count = ptr->get_children().size();
    for (size_t i{0}; i < count; i++) {
        g_node_remove_tracks_recur(p_nodes[i], p_tracks);
    }
}

void AlbumListWindow::remove_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks)
{
    g_node_remove_tracks_recur(m_root, p_tracks);
}

void AlbumListWindow::on_items_added(const pfc::list_base_const_t<metadb_handle_ptr>& p_const_data)
{
    if (m_library_v4.is_valid() && !m_library_v4->is_initialized())
        return;

    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add = p_const_data;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove;

    update_tree(to_add, to_remove, true);
}

void AlbumListWindow::on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr>& p_data_const)
{
    if (m_library_v4.is_valid() && !m_library_v4->is_initialized())
        return;

    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove = p_data_const;

    update_tree(to_add, to_remove, true);
}

void AlbumListWindow::on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr>& p_const_data)
{
    metadb_handle_list_t<pfc::alloc_fast_aggressive> modified = p_const_data;

    update_tree(modified, modified, true);
}

void AlbumListWindow::on_items_modified_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data& data)
{
    if (!m_library_v4->is_initialized())
        return;

    on_items_modified(items);
}

void AlbumListWindow::on_library_initialized()
{
    if (cfg_populate_on_init)
        refresh_tree();
}

void AlbumListWindow::update_tree(metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_add,
    metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_remove, bool preserve_existing)
{
    if (preserve_existing && !m_populated)
        return;

    SetWindowRedraw(m_wnd_tv, FALSE);
    std::optional redraw_on_reset = gsl::finally([wnd_tv = m_wnd_tv] { SetWindowRedraw(wnd_tv, TRUE); });

    if (!preserve_existing && m_populated)
        delete_all_nodes();

    if (preserve_existing && to_remove.get_count()) {
        mmh::in_place_sort(to_remove, pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, false);
    }

    try {
        if (preserve_existing)
            remove_nodes(to_remove);
        build_nodes(to_add, preserve_existing);
    } catch (pfc::exception const& e) {
        pfc::string_formatter formatter;
        popup_message::g_show(
            formatter << "Album list panel: An error occurred while generating the tree (" << e << ").", "Error",
            popup_message::icon_error);
        delete_all_nodes();
        return;
    }

    if (!m_root || !m_root->get_tracks().get_count()) {
        delete_all_nodes();
    } else if (m_root) {
        auto new_selection = TreeViewPopulator::s_setup_tree(m_wnd_tv, TVI_ROOT, m_root, m_node_state, 0, 0);
        ranges::insert(m_selection, new_selection);
        m_cleaned_selection.reset();
    }

    if (m_node_state && (!m_library_v4.is_valid() || m_library_v4->is_initialized()))
        m_node_state.reset();

    m_populated = true;

    m_node_state.reset();

    redraw_on_reset.reset();

    restore_scroll_position();
}

void AlbumListWindow::refresh_tree(bool preserve_state)
{
    TRACK_CALL_TEXT("album_list_panel_refresh_tree");

    if (!m_wnd_tv)
        return;

    if (m_library_v4.is_valid() && !m_library_v4->is_initialized())
        return;

    pfc::hires_timer timer;
    timer.start();

    if (preserve_state && m_root)
        m_node_state = m_root->get_state(m_selection);

    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove;
    to_add.prealloc(1024);
    library_manager::get()->get_all_items(to_add);

    update_tree(to_add, to_remove, false);

    console::print("Album list panel: initialised in ", pfc::format_float(timer.query(), 0, 3), " s");
}
