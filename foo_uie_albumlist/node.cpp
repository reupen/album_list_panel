#include "stdafx.h"

void node::sort_children()
{
    const auto count = m_children.size();
    mmh::Permutation permutation(count);
    pfc::array_staticsize_t<pfc::stringcvt::string_wide_from_utf8_fast> sortdata(count);

    for (size_t n = 0; n < count; n++)
        sortdata[n].convert(m_children[n]->m_value);
    mmh::sort_get_permutation(sortdata, permutation, StrCmpLogicalW, false, false, true);

    mmh::destructive_reorder(m_children, permutation);
    concurrency::parallel_for(size_t{0}, count, [this](size_t n) { m_children[n]->sort_children(); });
}

void node::sort_entries() // for contextmenu
{
    if (!m_sorted) {
        pfc::string8 tf_string;
        if (m_bydir)
            tf_string = "%path_sort%";
        else {
            tf_string = m_window->get_hierarchy();
            tf_string += "|%path_sort%";
        }
        service_ptr_t<titleformat_object> script;
        if (static_api_ptr_t<titleformat_compiler>()->compile(script, tf_string))
            fbh::sort_metadb_handle_list_by_format(m_tracks, script, nullptr);
        m_sorted = true;
    }
}

void node::create_new_playlist()
{
    static_api_ptr_t<playlist_manager> api;
    pfc::string8 name = m_value.get_ptr();
    if (name.is_empty())
        name = "All music";
    const size_t index = api->create_playlist(name, pfc_infinite, pfc_infinite);
    if (index != pfc_infinite) {
        api->set_active_playlist(index);
        send_to_playlist(true);
    }
}

void node::send_to_playlist(bool replace)
{
    static_api_ptr_t<playlist_manager> api;
    const bool select = !!cfg_add_items_select;
    api->activeplaylist_undo_backup();
    if (replace)
        api->activeplaylist_clear();
    else if (select)
        api->activeplaylist_clear_selection();
    if (cfg_add_items_use_core_sort)
        api->activeplaylist_add_items_filter(m_tracks, select);
    else {
        sort_entries();
        api->activeplaylist_add_items(m_tracks, bit_array_val(select));
    }

    if (select && !replace) {
        const size_t num = api->activeplaylist_get_item_count();
        if (num > 0) {
            api->activeplaylist_set_focus_item(num - 1);
        }
    }
}

node::node(const char* p_value, size_t p_value_len, album_list_window* window, uint16_t level)
    : m_level(level)
    , m_window(window)
{
    if (p_value && p_value_len > 0) {
        m_value.set_string(p_value, p_value_len);
    }
    m_sorted = false;
}

void node::remove_entries(pfc::bit_array& mask)
{
    m_tracks.remove_mask(mask);
}

void node::set_data(const pfc::list_base_const_t<metadb_handle_ptr>& p_data, bool b_keep_existing)
{
    if (!b_keep_existing)
        m_tracks.remove_all();
    m_tracks.add_items(p_data);
    m_sorted = false;
}

alp::SavedNodeState node::get_state(const node_ptr& selection)
{
    alp::SavedNodeState state;
    state.name = m_value;
    state.expanded = m_expanded;
    state.selected = selection.get() == this;

    state.children = m_children
        | std::ranges::views::filter([&selection](auto& child) { return child->is_expanded() || selection == child; })
        | std::ranges::views::transform([&selection](auto& child) { return child->get_state(selection); })
        | ranges::to_vector;

    return state;
}

std::tuple<std::vector<node_ptr>::const_iterator, std::vector<node_ptr>::const_iterator> node::find_child(
    std::string_view name) const
{
    auto normalised_name = name.empty() ? "?"sv : name;
    const auto value_utf16 = pfc::stringcvt::string_wide_from_utf8(normalised_name.data(), normalised_name.size());

    return std::ranges::equal_range(
        m_children, value_utf16.get_ptr(),
        [](const wchar_t* left, const wchar_t* right) { return StrCmpLogicalW(left, right) < 0; },
        [](auto& node) { return pfc::stringcvt::string_wide_from_utf8(node->m_value); });
}

node_ptr node::find_or_add_child(const char* p_value, size_t p_value_len, bool b_find, bool& b_new)
{
    if (!b_find)
        return add_child_v2(p_value, p_value_len);

    auto [start, end] = find_child({p_value, p_value_len});

    if (start != end) {
        b_new = false;
        return *start;
    }

    b_new = true;

    return *m_children.insert(start, std::make_shared<node>(p_value, p_value_len, m_window, m_level + 1));
}

node_ptr node::add_child_v2(const char* p_value, size_t p_value_len)
{
    if (p_value_len == 0 || *p_value == 0) {
        p_value = "?";
        p_value_len = 1;
    }
    node_ptr temp = std::make_shared<node>(p_value, p_value_len, m_window, m_level + 1);
    m_children.emplace_back(temp);
    return temp;
}

void node::mark_all_labels_dirty()
{
    m_label_dirty = true;
    t_size i = m_children.size();
    for (; i; i--) {
        m_children[i - 1]->mark_all_labels_dirty();
    }
}

void node::purge_empty_children(HWND wnd)
{
    size_t index_first_removed = pfc_infinite;

    bool was_something_removed{};
    for (auto iter = m_children.begin(); iter != m_children.end();) {
        auto& child = *iter;

        if (was_something_removed && cfg_show_item_indices) {
            child->m_label_dirty = true;
        }

        if (child->get_entries().get_count()) {
            ++iter;
            continue;
        }

        if (m_window && m_window->m_selection == child)
            m_window->m_selection = nullptr;

        if (child->m_ti)
            TreeView_DeleteItem(wnd, child->m_ti);

        iter = m_children.erase(iter);

        was_something_removed = true;

        if (cfg_show_subitem_counts)
            m_label_dirty = true;
    }
}
