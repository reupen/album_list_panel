#include "stdafx.h"
#include "tree_view_populator.h"

#include "node_formatter.h"

void TreeViewPopulator::s_setup_tree(HWND wnd_tv, HTREEITEM parent, node_ptr ptr, t_size idx, t_size max_idx)
{
    TRACK_CALL_TEXT("album_list_panel::TreeViewPopulator::s_setup_tree");
    TreeViewPopulator populater{wnd_tv, ptr->m_level};
    populater.setup_tree(parent, ptr, idx, max_idx, TVI_FIRST);
}

void TreeViewPopulator::s_setup_children(HWND wnd_tv, node_ptr ptr)
{
    TreeViewPopulator populater{wnd_tv, ptr->m_level};
    populater.setup_children(ptr);
}

void TreeViewPopulator::setup_tree(HTREEITEM parent, node_ptr ptr, t_size idx, t_size max_idx, HTREEITEM ti_after)
{
    const auto populate_children = ptr->m_children_inserted || ptr->m_level < 1 + m_initial_level;

    ptr->purge_empty_children(m_wnd_tv);

    if ((!ptr->m_ti || ptr->m_label_dirty) && (ptr->m_level > 0 || cfg_show_root_node)) {
        auto text = m_node_formatter.format(ptr, idx, max_idx);

        m_utf16_converter.convert(text.data(), text.size());
        if (ptr->m_ti) {
            TVITEM tvi{};
            tvi.hItem = ptr->m_ti;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = const_cast<WCHAR*>(m_utf16_converter.get_ptr());
            TreeView_SetItem(m_wnd_tv, &tvi);
        } else {
            TVINSERTSTRUCT is{};
            is.hParent = parent;
            is.hInsertAfter = ti_after;
            is.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
            is.item.pszText = const_cast<WCHAR*>(m_utf16_converter.get_ptr());
            is.item.lParam = reinterpret_cast<LPARAM>(ptr.get());
            is.item.state = ptr->m_level < 1 ? TVIS_EXPANDED : 0;
            is.item.stateMask = TVIS_EXPANDED;

            const auto children_count = ptr->get_children().size();
            if (!populate_children && children_count > 0) {
                is.item.mask |= TVIF_CHILDREN;
                is.item.cChildren = 1;
            }

            ptr->m_ti = TreeView_InsertItem(m_wnd_tv, &is);
        }
        ptr->m_label_dirty = false;
    }

    if (populate_children)
        setup_children(ptr);
}

void TreeViewPopulator::setup_children(node_ptr ptr)
{
    const auto& children = ptr->get_children();
    const auto children_count = children.size();

    if (ptr->m_children_inserted) {
        for (size_t i{0}; i < children_count; ++i) {
            HTREEITEM ti_aft = i ? children[i - 1]->m_ti : nullptr;

            if (ti_aft == nullptr)
                ti_aft = TVI_FIRST;

            setup_tree(ptr->m_ti, children[i], i, children_count, ti_aft);
        }
    } else {
        // If there are no existing items, use a more optimised path that inserts items in reverse
        for (auto i{children_count}; i > 0; --i) {
            const auto index = i - 1;
            setup_tree(ptr->m_ti, children[index], index, children_count, TVI_FIRST);
        }
    }

    ptr->m_children_inserted = true;
}
