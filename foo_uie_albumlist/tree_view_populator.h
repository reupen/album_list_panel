#pragma once

#include "node_formatter.h"

class TreeViewPopulator {
public:
    TreeViewPopulator(HWND wnd_tv, uint16_t initial_level = 0)
        : m_wnd_tv{wnd_tv}
        , m_has_selection(TreeView_GetSelection(wnd_tv) != nullptr)
        , m_initial_level{initial_level}
    {
    }

    static void s_setup_children(HWND wnd_tv, node_ptr ptr);
    static std::unordered_set<node_ptr> s_setup_tree(HWND wnd_tv, HTREEITEM parent, node_ptr ptr,
        std::optional<alp::SavedNodeState> node_state, t_size idx, t_size max_idx);

private:
    void setup_tree(HTREEITEM parent, node_ptr ptr, std::optional<alp::SavedNodeState> node_state, t_size idx,
        t_size max_idx, HTREEITEM ti_after);
    void setup_children(node_ptr ptr, std::optional<alp::SavedNodeState> node_state);

    HWND m_wnd_tv;
    bool m_has_selection{};
    std::unordered_set<node_ptr> m_new_selection;
    uint16_t m_initial_level;

    NodeFormatter m_node_formatter;
    pfc::stringcvt::string_wide_from_utf8_fast m_utf16_converter;
};
