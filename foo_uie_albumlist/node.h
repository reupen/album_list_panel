#pragma once

#include "node_state.h"

typedef std::shared_ptr<class Node> node_ptr;

class Node : public std::enable_shared_from_this<Node> {
public:
    HTREEITEM m_ti{};
    bool m_label_dirty{};
    bool m_children_inserted{};
    uint16_t m_level;

    Node(const char* name, size_t name_length, class AlbumListWindow* window, uint16_t level,
        std::weak_ptr<Node> parent = {});

    void sort_children();

    const metadb_handle_list& get_tracks() const { return m_tracks; }
    const metadb_handle_list& get_sorted_tracks()
    {
        sort_tracks();
        return m_tracks;
    }

    void set_bydir(bool p) { m_bydir = p; }

    ~Node()
    {
        m_tracks.remove_all();
        m_children.clear();
    }

    const char* get_name() const { return m_name.is_empty() ? "All music" : m_name.get_ptr(); }
    const wchar_t* get_name_utf16()
    {
        if (m_name.is_empty())
            return L"All music";

        if (m_name_utf16.is_empty())
            m_name_utf16.convert(m_name.get_ptr(), m_name.get_length());

        return m_name_utf16.get_ptr();
    }

    void add_entry(const metadb_handle_ptr& p_entry) { m_tracks.add_item(p_entry); }

    void remove_tracks(pfc::bit_array& mask);

    void set_data(const pfc::list_base_const_t<metadb_handle_ptr>& p_data, bool b_keep_existing);

    bool is_expanded() const { return m_expanded; }
    void set_expanded(bool expanded) { m_expanded = expanded; }

    alp::SavedNodeState get_state(const std::unordered_set<node_ptr>& selection);

    std::tuple<std::vector<node_ptr>::const_iterator, std::vector<node_ptr>::const_iterator> find_child(
        std::string_view name) const;
    node_ptr find_or_add_child(const char* p_value, size_t p_value_len, bool b_find, bool& b_new);

    node_ptr add_child_v2(const char* p_value, size_t p_value_len);

    node_ptr add_child_v2(const char* p_value) { return add_child_v2(p_value, strlen(p_value)); }

    void mark_all_labels_dirty();
    void mark_tracks_unsorted();

    void purge_empty_children(HWND wnd);

    const std::vector<node_ptr>& get_children() const { return m_children; }

    std::vector<node_ptr>& get_children() { return m_children; }

    size_t get_num_children() const { return m_children.size(); }

    size_t get_num_tracks() const { return m_tracks.get_count(); }

    node_ptr get_parent() const { return m_parent.lock(); }

    std::vector<node_ptr> get_parents() const;

    std::vector<node_ptr> get_hierarchy();

    void set_display_index(std::optional<size_t> display_index) { m_display_index = display_index; }

    std::optional<size_t> get_display_index() const { return m_display_index; }

private:
    void sort_tracks();
    void apply_function(std::function<void(Node&)> func)
    {
        func(*this);

        for (auto& child : m_children)
            child->apply_function(func);
    }

    std::weak_ptr<Node> m_parent;
    std::optional<size_t> m_display_index;
    pfc::string_simple m_name;
    pfc::stringcvt::string_wide_from_utf8 m_name_utf16;
    std::vector<node_ptr> m_children;
    metadb_handle_list m_tracks;
    bool m_sorted : 1 {};
    bool m_bydir : 1 {};
    bool m_expanded : 1 {};
    class AlbumListWindow* m_window{};
};
