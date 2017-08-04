#include "stdafx.h"
#include "tree_view_populator.h"

template<typename String>
const char* c_str(String& string)
{
    return string.c_str();
}

template<>
const char* c_str(const char* const & string)
{
    return string;
}

struct process_bydir_entry {
    metadb_handle* m_item;
    const char* m_path;

    static bool g_is_separator(char c) { return c == '\\' || c == '|' || c == '/'; }

    static const char* g_advance(const char* p_path)
    {
        const char* ptr = p_path;
        while (*ptr && !g_is_separator(*ptr))
            ptr++;
        while (*ptr && g_is_separator(*ptr))
            ptr++;
        return ptr;
    }

    static size_t g_get_segment_length(const char* p_path)
    {
        size_t ret{0};
        while (p_path[ret] && !g_is_separator(p_path[ret]))
            ret++;
        return ret;
    }

    size_t get_segment_length() const
    {
        return g_get_segment_length(m_path);
    }

    static int g_compare_segment(const char* p_path1, const char* p_path2)
    {
        return metadb::path_compare_ex(p_path1, g_get_segment_length(p_path1), p_path2, g_get_segment_length(p_path2));
    }

    static int g_compare_segment(const process_bydir_entry& p_item1, const process_bydir_entry& p_item2)
    {
        return metadb::path_compare_ex(p_item1.m_path, p_item1.get_segment_length(), p_item2.m_path,
                                       p_item2.get_segment_length());
    }

    static int g_compare(const process_bydir_entry& p_item1, const process_bydir_entry& p_item2)
    {
        return metadb::path_compare(p_item1.m_path, p_item2.m_path);
    }

    enum { is_bydir = true };
};

template<typename String = std::string>
struct process_byformat_entry {
    metadb_handle* m_item;
    String m_path;

    static bool g_is_separator(char c) { return c == '|'; }

    static const char* g_advance(const char* p_path)
    {
        const char* ptr = p_path;
        while (*ptr && !g_is_separator(*ptr))
            ptr++;
        while (*ptr && g_is_separator(*ptr))
            ptr++;
        return ptr;
    }

    static size_t g_get_segment_length(const char* p_path)
    {
        size_t ret = 0;
        while (p_path[ret] && !g_is_separator(p_path[ret]))
            ret++;
        return ret;
    }

    size_t get_segment_length() const
    {
        return g_get_segment_length(c_str(m_path));
    }

    static int g_compare_segment(const char* p_path1, const char* p_path2)
    {
        return stricmp_utf8_ex(p_path1, g_get_segment_length(p_path1), p_path2, g_get_segment_length(p_path2));
    }

    static int g_compare_segment(const process_byformat_entry& p_item1, const process_byformat_entry& p_item2)
    {
        return stricmp_utf8_ex(c_str(p_item1.m_path), p_item1.get_segment_length(), c_str(p_item2.m_path),
                               p_item2.get_segment_length());
    }

    static int g_compare(const process_byformat_entry& p_item1, const process_byformat_entry& p_item2)
    {
        return stricmp_utf8(c_str(p_item1.m_path), c_str(p_item2.m_path));
    }

    enum { is_bydir = false };
};

template<typename t_entry>
class process_entry_list_wrapper_t : public list_base_const_t<metadb_handle_ptr> {
public:
    process_entry_list_wrapper_t(const t_entry* p_data, size_t p_count) : m_data(p_data), m_count(p_count) {}

    size_t get_count() const override
    {
        return m_count;
    }

    void get_item_ex(metadb_handle_ptr& p_out, size_t n) const override
    {
        p_out = m_data[n].m_item;
    }

private:
    const t_entry* m_data;
    size_t m_count;
};

template<typename t_entry, typename t_local_entry = t_entry>
static void process_level_recur_t(const t_entry* p_items, size_t const p_items_count, node_ptr p_parent,
                                  bool b_add_only)
{
    p_parent->set_bydir(t_entry::is_bydir);
    p_parent->set_data(process_entry_list_wrapper_t<t_entry>(p_items, p_items_count), !b_add_only);
    p_parent->m_label_dirty = cfg_show_numbers != 0;
    assert(p_items_count > 0);
    pfc::array_t<t_local_entry> items_local;
    items_local.set_size(p_items_count);
    size_t items_local_ptr{0};
    const char* last_path{nullptr};
    bool b_node_added{false};

    for (size_t i = 0; i < p_items_count; i++) {
        const char* current_path = c_str(p_items[i].m_path);
        while (*current_path && t_entry::g_is_separator(*current_path))
            current_path++;
        if (items_local_ptr > 0 && t_entry::g_compare_segment(last_path, current_path) != 0) {
            bool b_new = false;
            node_ptr p_node = p_parent->find_or_add_child(last_path, t_entry::g_get_segment_length(last_path),
                                                          !b_add_only, b_new);
            if (b_new)
                b_node_added = true;
            process_level_recur_t<>(items_local.get_ptr(), items_local_ptr, p_node, b_add_only);
            items_local_ptr = 0;
            last_path = nullptr;
        }

        if (*current_path != 0) {
            items_local[items_local_ptr].m_item = p_items[i].m_item;
            items_local[items_local_ptr].m_path = t_entry::g_advance(current_path);
            items_local_ptr++;
            last_path = current_path;
        }
    }

    if (items_local_ptr > 0) {
        bool b_new{false};
        node_ptr p_node = p_parent->find_or_add_child(last_path, t_entry::g_get_segment_length(last_path), !b_add_only,
                                                      b_new);
        if (b_new)
            b_node_added = true;
        process_level_recur_t<>(items_local.get_ptr(), items_local_ptr, p_node, b_add_only);
        items_local_ptr = 0;
        last_path = nullptr;
    }

    if (b_node_added && cfg_show_numbers2) {
        size_t count = p_parent->get_children().get_count();
        for (size_t i{0}; i < count; i++)
            p_parent->get_children()[i]->m_label_dirty = true;
    }
}

struct process_byformat_branch_segment {
    size_t m_first_choice;
    size_t m_last_choice;
    size_t m_current_choice;
};

struct process_byformat_branch_choice {
    size_t m_start;
    size_t m_end;
};

template<typename List>
size_t process_byformat_add_branches(metadb_handle* handle, const char* p_text, List& entries)
{
    const char* marker = strchr(p_text, 4);
    if (marker == nullptr) {
        entries.push_back(process_byformat_entry<>{handle, p_text});
        return 1;
    }
    size_t branch_count{1};

    list_t<process_byformat_branch_segment> segments;
    list_t<process_byformat_branch_choice> choices;

    // compute segments and branch count
    size_t ptr{0};
    while (p_text[ptr] != 0) {
        // begin choice
        if (p_text[ptr] == 4) {
            ptr++;

            if (p_text[ptr] == 4)
                return 0; // empty choice

            process_byformat_branch_segment segment;
            segment.m_first_choice = choices.get_count();
            segment.m_current_choice = segment.m_first_choice;

            process_byformat_branch_choice choice;
            choice.m_start = ptr;

            while (p_text[ptr] != 0 && p_text[ptr] != 4) {
                if (p_text[ptr] == 5) {
                    choice.m_end = ptr;
                    choices.add_item(choice);
                    ptr++;
                    choice.m_start = ptr;
                }
                else
                    ptr++;
            }

            choice.m_end = ptr;

            if (p_text[ptr] != 0)
                ptr++;

            if (choice.m_end > choice.m_start)
                choices.add_item(choice);

            segment.m_last_choice = choices.get_count();

            if (segment.m_last_choice == segment.m_first_choice)
                return 0; // only empty choices

            segments.add_item(segment);

            branch_count *= segment.m_last_choice - segment.m_first_choice;
        }
        else {
            process_byformat_branch_segment segment;
            segment.m_first_choice = choices.get_count();
            segment.m_current_choice = segment.m_first_choice;

            process_byformat_branch_choice choice;
            choice.m_start = ptr;

            while (p_text[ptr] != 0 && p_text[ptr] != 4)
                ptr++;

            choice.m_end = ptr;
            choices.add_item(choice);

            segment.m_last_choice = choices.get_count();
            segments.add_item(segment);
        }
    }

    pfc::string8_fast buffer;
    // assemble branches
    for (size_t branch_index{0}; branch_index < branch_count; branch_index++) {
        buffer.reset();
        const size_t segment_count{segments.get_count()};
        for (size_t segment_index{0}; segment_index < segment_count; segment_index++) {
            const process_byformat_branch_choice& choice = choices[segments[segment_index].m_current_choice];
            buffer.add_string(&p_text[choice.m_start], choice.m_end - choice.m_start);
        }
        entries.push_back(process_byformat_entry<>{handle, buffer});

        for (size_t segment_index{0}; segment_index < segment_count; segment_index++) {
            process_byformat_branch_segment& segment = segments[segment_count - segment_index - 1];
            segment.m_current_choice++;
            if (segment.m_current_choice >= segment.m_last_choice)
                segment.m_current_choice = segment.m_first_choice;
            else
                break;
        }
    }
    return branch_count;
}

void album_list_window::build_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& tracks, bool preserve_existing)
{
    m_filter_ptr.release();

    static_api_ptr_t<library_manager> api;
    string8 pattern;

    if (m_wnd_edit)
        uGetWindowText(m_wnd_edit, pattern);

    if (m_wnd_edit && !pattern.is_empty()) {
        const auto callback = [p_this = service_ptr_t<album_list_window>{this}](auto&& code) {
            p_this->on_task_completion(0, code);
        };
        const auto completion_notify_ptr = fb2k::makeCompletionNotify(callback);

        try {
            m_filter_ptr = static_api_ptr_t<search_filter_manager_v2>()->create_ex(
                pattern, completion_notify_ptr, NULL
            );

            pfc::array_t<bool> mask;
            mask.set_count(tracks.get_count());
            m_filter_ptr->test_multi(tracks, mask.get_ptr());
            tracks.remove_mask(bit_array_not(bit_array_table(mask.get_ptr(), mask.get_count())));
        }
        catch (const pfc::exception&) {}
    }

    if (is_bydir()) {
        const size_t count = tracks.get_count();

        if (count > 0) {
            pfc::list_t<process_bydir_entry> entries;
            entries.set_size(count);
            pfc::array_t<string8> strings;
            strings.set_size(count);

            for (size_t n{0}; n < count; n++) {
                api->get_relative_path(tracks[n], strings[n]);
                entries[n].m_path = strings[n];
                entries[n].m_item = tracks[n].get_ptr();
            }

            mmh::single_reordering_sort(entries, process_bydir_entry::g_compare, false);

            if (!preserve_existing || !m_root.is_valid())
                m_root = new node(nullptr, 0, this, 0);

            process_level_recur_t(entries.get_ptr(), count, m_root, !preserve_existing);
        }
    }
    else {
        const size_t count{tracks.get_count()};

        if (count > 0) {
            service_ptr_t<titleformat_object> script;
            static_api_ptr_t<titleformat_compiler>()->compile_safe(script, get_hierarchy());
            concurrency::concurrent_vector<process_byformat_entry<>> entries;

            concurrency::parallel_for(size_t{0}, count, [&tracks, &script, &entries](size_t n) {
                pfc::string8_fast formatted_title;
                formatted_title.prealloc(32);
                const playable_location& location = tracks[n]->get_location();
                metadb_info_container::ptr info_ptr;
                info_ptr = tracks[n]->get_info_ref();

                titleformat_hook_impl_file_info_branch tf_hook_file_info(location, &info_ptr->info());
                titleformat_text_filter_impl_reserved_chars tf_hook_text_filter("|");
                formatted_title.prealloc(32);
                tracks[n]->format_title(
                    &tf_hook_file_info, formatted_title, script, &tf_hook_text_filter
                );
                process_byformat_add_branches(tracks[n].get_ptr(), formatted_title, entries);
            });

            const size_t size = entries.size();

            pfc::list_t<process_byformat_entry<>> entries_sorted;
            entries_sorted.set_size(size);
            for (size_t i{0}; i < size; ++i)
                entries_sorted[i] = std::move(entries[i]);

            mmh::Permuation perm(size);
            mmh::sort_get_permuation(entries_sorted, perm, process_byformat_entry<>::g_compare, false, false, true);
            mmh::destructive_reorder(entries_sorted, perm);

            if (!preserve_existing || !m_root.is_valid())
                m_root = new node(nullptr, 0, this, 0);
            process_level_recur_t<process_byformat_entry<>, process_byformat_entry<const char*>>(
                entries_sorted.get_ptr(), size, m_root, !preserve_existing);
        }
    }
    if (!preserve_existing) {
        m_root->sort_children();
    }
}

void g_node_remove_tracks_recur(const node_ptr& ptr, const metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks)
{
    if (!ptr.is_valid())
        return;

    size_t count{ptr->get_entries().get_count()};

    if (!count)
        return;

    size_t index{};
    bit_array_bittable mask(count);
    bool b_found{false};

    const metadb_handle_ptr* p_entries = ptr->get_entries().get_ptr();
    const node_ptr* p_nodes = ptr->get_children().get_ptr();

    for (size_t i{0}; i < count; i++) {
        if (pfc::bsearch_simple_inline_t(p_tracks.get_ptr(), p_tracks.get_count(), p_entries[i], index)) {
            mask.set(i, true);
            b_found = true;
        }
    }

    if (b_found)
        ptr->remove_entries(mask);

    count = ptr->get_children().get_count();
    for (size_t i{0}; i < count; i++) {
        g_node_remove_tracks_recur(p_nodes[i], p_tracks);
    }
}

void album_list_window::remove_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks)
{
    g_node_remove_tracks_recur(m_root, p_tracks);
}

void album_list_window::on_items_added(const pfc::list_base_const_t<metadb_handle_ptr>& p_const_data)
{
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add = p_const_data;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove;

    update_tree(to_add, to_remove, true);
}

void album_list_window::on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr>& p_data_const)
{
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove = p_data_const;

    update_tree(to_add, to_remove, true);
}

void album_list_window::on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr>& p_const_data)
{
    metadb_handle_list_t<pfc::alloc_fast_aggressive> modified = p_const_data;

    update_tree(modified, modified, true);
}

void album_list_window::update_tree(metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_add,
    metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_remove, bool preserve_existing)
{
    if (preserve_existing && !m_populated)
        return;

    if (!preserve_existing) {
        TreeView_DeleteItem(m_wnd_tv, TVI_ROOT);
        m_selection.release();
        m_root.release();
    }

    if (preserve_existing && to_remove.get_count()) {
        mmh::in_place_sort(to_remove, pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, false);
    }

    SendMessage(m_wnd_tv, WM_SETREDRAW, FALSE, 0);

    try {
        if (preserve_existing)
            remove_nodes(to_remove);
        build_nodes(to_add, preserve_existing);
    }
    catch (pfc::exception const& e) {
        string_formatter formatter;
        popup_message::g_show(
            formatter << "Album list panel: An error occured while generating the tree (" << e << ").", "Error",
            popup_message::icon_error
        );
        m_root.release();
        m_selection.release();
        TreeView_DeleteItem(m_wnd_tv, TVI_ROOT);
    }

    if (m_root.is_valid()) {
        if (!m_root->get_entries().get_count()) {
            m_root.release();
            m_selection.release();
            TreeView_DeleteItem(m_wnd_tv, TVI_ROOT);
        }
        else {
            TreeViewPopulator::s_setup_tree(m_wnd_tv, TVI_ROOT, m_root, 0, 0, nullptr);
        }
    }

    SendMessage(m_wnd_tv, WM_SETREDRAW, TRUE, 0);
}

void album_list_window::refresh_tree()
{
    TRACK_CALL_TEXT("album_list_panel_refresh_tree");

    if (!m_wnd_tv)
        return;

    static_api_ptr_t<library_manager> api;
    if (!api->is_library_enabled())
        return;

    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_add;
    metadb_handle_list_t<pfc::alloc_fast_aggressive> to_remove;
    to_add.prealloc(1024);
    api->get_all_items(to_add);

    hires_timer timer;
    timer.start();

    update_tree(to_add, to_remove, false);

    console::formatter formatter;
    formatter << "Album list panel: initialised in " << pfc::format_float(timer.query(), 0, 3) << " s";

    m_populated = m_root.is_valid();
}
