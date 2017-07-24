#pragma once

class TreeViewPopulator {
public:
    void setup_tree(HWND list, HTREEITEM parent, node_ptr ptr, t_size level, t_size idx, t_size max_idx, HTREEITEM ti_after);

    static void s_setup_tree(HWND list, HTREEITEM parent, node_ptr ptr, t_size level, t_size idx, t_size max_idx, HTREEITEM ti_after = TVI_LAST);
private:
    const char* get_item_text(node_ptr ptr, t_size item_index, t_size child_count);
    string8_fast_aggressive m_text_buffer;
    pfc::stringcvt::string_wide_from_utf8_fast m_utf16_converter;
};
