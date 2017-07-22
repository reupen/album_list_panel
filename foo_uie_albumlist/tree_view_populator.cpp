#include "stdafx.h"
#include "tree_view_populator.h"

void TreeViewPopulator::s_setup_tree(HWND list, HTREEITEM parent, node_ptr ptr, t_size level, t_size idx, t_size max_idx, metadb_handle_list_t<pfc::alloc_fast_aggressive>& entries, HTREEITEM ti_after)
{
    TreeViewPopulator populater;
    populater.setup_tree(list, parent, ptr, level, idx, max_idx, entries, ti_after);
}

void TreeViewPopulator::setup_tree(HWND list, HTREEITEM parent, node_ptr ptr, t_size level, t_size idx, t_size max_idx, metadb_handle_list_t<pfc::alloc_fast_aggressive>& entries, HTREEITEM ti_after)
{
    HTREEITEM item = TVI_ROOT;

    ptr->purge_empty_children(list);

    if (ptr->m_ti)
    {
        item = ptr->m_ti;
    }
    if ((!ptr->m_ti || ptr->m_label_dirty) && (level > 0 || cfg_show_root))
    {
        const char* text = get_item_text(ptr, idx, max_idx);

        m_utf16_converter.convert(text);
        if (ptr->m_ti)
        {
            TVITEM tvi{};
            tvi.hItem = ptr->m_ti;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = const_cast<WCHAR*>(m_utf16_converter.get_ptr());
            TreeView_SetItem(list, &tvi);
        }
        else
        {
            TVINSERTSTRUCT is{};
            is.hParent = parent;
            is.hInsertAfter = ti_after;
            is.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
            is.item.pszText = const_cast<WCHAR*>(m_utf16_converter.get_ptr());
            is.item.lParam = reinterpret_cast<LPARAM>(ptr.get_ptr());
            is.item.state = level < 1 ? TVIS_EXPANDED : 0;
            is.item.stateMask = TVIS_EXPANDED;
            item = TreeView_InsertItem(list, &is);

            ptr->m_ti = item;
        }
        ptr->m_label_dirty = false;
    }

    const list_t<node_ptr> & children = ptr->get_children();

    unsigned n;

    {
        for (n = 0; n < children.get_count(); n++)
        {
            HTREEITEM ti_aft = n ? children[n - 1]->m_ti : nullptr;
            if (ti_aft == nullptr) ti_aft = TVI_FIRST;
            setup_tree(list, item, children[n], level + 1, n, children.get_count(), entries, ti_aft);
        }
    }
}

const char* TreeViewPopulator::get_item_text(node_ptr ptr, t_size item_index, t_size item_count)
{
    if ((!cfg_show_numbers2 || item_count == 0) && !cfg_show_numbers)
        return ptr->get_val();

    m_text_buffer.reset();

    if (cfg_show_numbers2 && item_count > 0)
    {
        t_size pad = 0;
        while (item_count > 0)
        {
            item_count /= 10;
            pad++;
        }
        char temp1[128], temp2[128];
        sprintf_s(temp1, "%%0%uu. ", pad);
        sprintf_s(temp2, temp1, item_index + 1);
        m_text_buffer += temp2;
    }

    m_text_buffer += ptr->get_val();

    if (cfg_show_numbers)
    {
        t_size num = ptr->get_num_children();
        if (num > 0)
        {
            char blah[64];
            sprintf_s(blah, " (%u)", num);
            m_text_buffer += blah;
        }
    }
    return m_text_buffer.get_ptr();
}
