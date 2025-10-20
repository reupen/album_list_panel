#include "stdafx.h"

void Node::sort_children()
{
    const auto count = m_children.size();
    mmh::Permutation permutation(count);

    mmh::sort_get_permutation(
        m_children, permutation,
        [](auto& left, auto& right) { return StrCmpLogicalW(left->get_name_utf16(), right->get_name_utf16()); }, false,
        false, true);

    mmh::destructive_reorder(m_children, permutation);
    concurrency::parallel_for(size_t{0}, count, [this](size_t n) { m_children[n]->sort_children(); });
}

std::vector<node_ptr> Node::get_parents() const
{
    std::vector<node_ptr> parents;
    node_ptr parent = get_parent();

    while (parent) {
        parents.emplace_back(parent);
        parent = parent->get_parent();
    }

    return parents;
}

std::vector<node_ptr> Node::get_hierarchy()
{
    auto nodes = get_parents() | ranges::actions::reverse;
    nodes.emplace_back(shared_from_this());
    return nodes;
}

void Node::sort_tracks()
{
    if (m_sorted)
        return;

    service_ptr_t<titleformat_object> script;

    if (cfg_add_items_use_core_sort) {
        script = playlist_incoming_item_filter_v4::get()->get_incoming_item_sorter();
    } else {
        pfc::string8 tf_string;
        if (m_bydir)
            tf_string = "%path_sort%";
        else {
            tf_string = m_window->get_hierarchy();
            tf_string += "|%path_sort%";
        }
        titleformat_compiler::get()->compile(script, tf_string);
    }

    if (script.is_valid())
        fbh::sort_metadb_handle_list_by_format(m_tracks, script, nullptr);

    m_sorted = true;
}

Node::Node(std::string name, AlbumListWindow* window, uint16_t level, std::weak_ptr<Node> parent)
    : m_level(level)
    , m_parent(std::move(parent))
    , m_name(std::move(name))
    , m_window(window)
{
}

void Node::remove_tracks(pfc::bit_array& mask)
{
    m_tracks.remove_mask(mask);
}

void Node::set_data(const pfc::list_base_const_t<metadb_handle_ptr>& p_data, bool b_keep_existing)
{
    if (!b_keep_existing)
        m_tracks.remove_all();
    m_tracks.add_items(p_data);
    m_sorted = false;
}

alp::SavedNodeState Node::get_state(const std::unordered_set<node_ptr>& selection)
{
    alp::SavedNodeState state;
    state.name = m_name;
    state.expanded = m_expanded;
    state.selected = selection.contains(this->shared_from_this());

    state.children = m_children | std::ranges::views::filter([&selection](auto& child) {
        return child->is_expanded() || selection.contains(child);
    }) | std::ranges::views::transform([&selection](auto& child) { return child->get_state(selection); })
        | ranges::to_vector;

    return state;
}

std::tuple<std::vector<node_ptr>::const_iterator, std::vector<node_ptr>::const_iterator> Node::find_child(
    std::string_view name) const
{
    const auto value_utf16 = pfc::stringcvt::string_wide_from_utf8(name.data(), name.size());

    return std::ranges::equal_range(
        m_children, value_utf16.get_ptr(),
        [](const wchar_t* left, const wchar_t* right) { return StrCmpLogicalW(left, right) < 0; },
        [](auto& node) { return node->get_name_utf16(); });
}

node_ptr Node::find_or_add_child(std::string name, bool b_find, bool& b_new)
{
    if (!b_find)
        return add_child_v2(std::move(name));

    auto [start, end] = find_child(name);

    if (start != end) {
        b_new = false;
        return *start;
    }

    b_new = true;

    return *m_children.insert(start, std::make_shared<Node>(name, m_window, m_level + 1, this->shared_from_this()));
}

node_ptr Node::add_child_v2(std::string name)
{
    node_ptr temp = std::make_shared<Node>(std::move(name), m_window, m_level + 1, this->shared_from_this());
    m_children.emplace_back(temp);
    return temp;
}

void Node::mark_tracks_unsorted()
{
    apply_function([](auto& node) { node.m_sorted = false; });
}

void Node::purge_empty_children(HWND wnd)
{
    for (auto iter = m_children.begin(); iter != m_children.end();) {
        auto& child = *iter;

        if (child->get_tracks().get_count()) {
            ++iter;
            continue;
        }

        if (m_window && m_window->m_selection.contains(child)) {
            m_window->m_selection.erase(child);
            m_window->m_cleaned_selection.reset();
        }

        if (child->m_ti)
            TreeView_DeleteItem(wnd, child->m_ti);

        iter = m_children.erase(iter);
    }
}
