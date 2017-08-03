#pragma once

typedef pfc::refcounted_object_ptr_t<class node> node_ptr;

class node : public pfc::refcounted_object_root {
public:
    HTREEITEM m_ti{};
    bool m_label_dirty{};
    bool m_children_inserted{};
    uint16_t m_level;

    node(const char* p_value, unsigned p_value_len, class album_list_window* window, uint16_t level);

    void sort_children();
    void sort_entries();//for contextmenu
    
    const metadb_handle_list& get_entries() const
    {
        return m_tracks;
    }

    void create_new_playlist();
    void send_to_playlist(bool replace);

    void set_bydir(bool p)
    {
        m_bydir = p;
    }

    ~node() override
    {
        m_tracks.remove_all();
        m_children.remove_all();
    }

    const char* get_val()
    {
        return m_value.is_empty() ? "All music" : m_value.get_ptr();
    }

    void add_entry(const metadb_handle_ptr& p_entry)
    {
        m_tracks.add_item(p_entry);
    }

    void remove_entries(pfc::bit_array& mask);

    void set_data(const list_base_const_t<metadb_handle_ptr>& p_data, bool b_keep_existing);

    static int g_compare_name(const node_ptr& p1, const wchar_t* str)
    {
        return StrCmpLogicalW(uT(p1->m_value), str);
    }

    node_ptr find_or_add_child(const char* p_value, unsigned p_value_len, bool b_find, bool& b_new);

    node_ptr add_child_v2(const char* p_value, unsigned p_value_len);

    node_ptr add_child_v2(const char* p_value)
    {
        return add_child_v2(p_value, strlen(p_value));
    }

    void reset()
    {
        m_children.remove_all();
        m_tracks.remove_all();
        m_sorted = false;
    }

    void mark_all_labels_dirty();

    void purge_empty_children(HWND wnd);

    const list_t<node_ptr>& get_children() const
    {
        return m_children;
    }

    list_t<node_ptr>& get_children()
    {
        return m_children;
    }

    unsigned get_num_children() const
    {
        return m_children.get_count();
    }

    unsigned get_num_entries() const
    {
        return m_tracks.get_count();
    }

private:
    string_simple m_value;
    list_t<node_ptr> m_children;
    metadb_handle_list m_tracks;
    bool m_sorted{};
    bool m_bydir{};
    class album_list_window* m_window{};
};
