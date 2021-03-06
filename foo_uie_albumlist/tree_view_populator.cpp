#include "stdafx.h"
#include "tree_view_populator.h"

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
        const char* text = get_item_text(ptr, idx, max_idx);

        m_utf16_converter.convert(text);
        if (ptr->m_ti) {
            TVITEM tvi{};
            tvi.hItem = ptr->m_ti;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = const_cast<WCHAR*>(m_utf16_converter.get_ptr());
            TreeView_SetItem(m_wnd_tv, &tvi);
        }
        else {
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
        for (auto i{ children_count }; i > 0; --i) {
            const auto index = i - 1;
            setup_tree(ptr->m_ti, children[index], index, children_count, TVI_FIRST);
        }
    }

    ptr->m_children_inserted = true;
}

const char* TreeViewPopulator::get_item_text(node_ptr ptr, t_size item_index, t_size item_count)
{
    if ((!cfg_show_item_indices || item_count == 0) && !cfg_show_subitem_counts)
        return ptr->get_val();

    m_text_buffer.reset();

    if (cfg_show_item_indices && item_count > 0) {
        t_size pad = 0;
        while (item_count > 0) {
            item_count /= 10;
            pad++;
        }
        char temp1[128], temp2[128];
        sprintf_s(temp1, "%%0%uu. ", pad);
        sprintf_s(temp2, temp1, item_index + 1);
        m_text_buffer += temp2;
    }

    m_text_buffer += ptr->get_val();

    if (cfg_show_subitem_counts) {
        t_size num = ptr->get_num_children();
        if (num > 0) {
            char blah[64];
            sprintf_s(blah, " (%u)", num);
            m_text_buffer += blah;
        }
    }
    return m_text_buffer.get_ptr();
}
