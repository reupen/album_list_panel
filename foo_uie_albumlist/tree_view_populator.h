#pragma once

class TreeViewPopulator {
public:
    TreeViewPopulator(HWND wnd_tv, uint16_t initial_level = 0)
        : m_wnd_tv{wnd_tv}, m_initial_level{initial_level} {}

    void setup_tree(HTREEITEM parent, node_ptr ptr, t_size idx, t_size max_idx, HTREEITEM ti_after);
    void setup_children(node_ptr ptr);

    static void s_setup_children(HWND wnd_tv, node_ptr ptr);
    static void s_setup_tree(HWND wnd_tv, HTREEITEM parent, node_ptr ptr, t_size idx, t_size max_idx, HTREEITEM ti_after = TVI_LAST);
private:
    HWND m_wnd_tv;
    uint16_t m_initial_level;

    const char* get_item_text(node_ptr ptr, t_size item_index, t_size child_count);
    string8_fast_aggressive m_text_buffer;
    pfc::stringcvt::string_wide_from_utf8_fast m_utf16_converter;
};
