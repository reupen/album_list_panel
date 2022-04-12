#include "stdafx.h"
#include "actions.h"
#include "tree_view_populator.h"

LRESULT album_list_window::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        s_instances.add_item(this);
        m_initialised = true;
        modeless_dialog_manager::g_add(wnd);

        m_dd_theme = IsThemeActive() && IsAppThemed() ? OpenThemeData(wnd, VSCLASS_DRAGDROP) : nullptr;

        create_tree();
        create_filter();

        if (cfg_populate_on_init) {
            refresh_tree();
            restore_scroll_position();
        }

        static_api_ptr_t<library_manager_v3>()->register_callback(this);
        break;
    }
    case WM_THEMECHANGED: {
        if (m_dd_theme)
            CloseThemeData(m_dd_theme);
        m_dd_theme = IsThemeActive() && IsAppThemed() ? OpenThemeData(wnd, VSCLASS_DRAGDROP) : nullptr;
        break;
    }
    case WM_CTLCOLOREDIT: {
        cui::colours::helper colours(album_list_filter_colours_client_id);
        const auto dc = reinterpret_cast<HDC>(wp);
        SetTextColor(dc, colours.get_colour(cui::colours::colour_text));
        SetBkMode(dc, TRANSPARENT);

        if (!s_filter_background_brush)
            s_filter_background_brush.reset(CreateSolidBrush(colours.get_colour(cui::colours::colour_background)));

        return reinterpret_cast<LPARAM>(s_filter_background_brush.get());
    }
    case WM_SIZE:
        on_size(LOWORD(lp), HIWORD(lp));
        break;
    case WM_TIMER:
        if (wp == EDIT_TIMER_ID) {
            refresh_tree();
            KillTimer(wnd, wp);
            m_timer = false;
        }
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDC_FILTER | (EN_CHANGE << 16):
            if (m_timer)
                KillTimer(m_wnd_edit, 500);
            m_timer = SetTimer(wnd, EDIT_TIMER_ID, 500, nullptr) != 0;
            return TRUE;
        case IDOK:
            if (GetKeyState(VK_SHIFT) & KF_UP)
                do_playlist(m_selection, false);
            else if (GetKeyState(VK_CONTROL) & KF_UP)
                do_playlist(m_selection, true, true);
            else
                do_playlist(m_selection, true);
            return 0;
        }
        break;
    case WM_CONTEXTMENU:
        return on_wm_contextmenu({GET_X_LPARAM(lp), GET_Y_LPARAM(lp)});
    case WM_NOTIFY: {
        auto hdr = reinterpret_cast<LPNMHDR>(lp);

        switch (hdr->idFrom) {
        case IDC_TREE: {
            const auto res = on_tree_view_wm_notify(hdr);
            if (res)
                return res.value();
            break;
        }
        }
        break;
    }
    case WM_DESTROY:
        static_api_ptr_t<library_manager_v3>()->unregister_callback(this);
        modeless_dialog_manager::g_remove(wnd);
        destroy_tree();
        destroy_filter();
        m_selection_holder.release();
        m_root.reset();
        m_selection.reset();
        if (m_dd_theme) {
            CloseThemeData(m_dd_theme);
            m_dd_theme = nullptr;
        }

        s_instances.remove_item(this);

        if (s_instances.get_count() == 0) {
            s_filter_background_brush.reset();
            s_font.reset();
        }

        m_initialised = false;
        break;
    }
    return DefWindowProc(wnd, msg, wp, lp);
}

LRESULT album_list_window::on_wm_contextmenu(POINT pt)
{
    enum { ID_SEND = 1, ID_ADD, ID_NEW, ID_AUTOSEND, ID_REFRESH, ID_FILT, ID_CONF, ID_VIEW_BASE };

    const HMENU menu{CreatePopupMenu()};
    service_ptr_t<contextmenu_manager> p_menu_manager;
    HTREEITEM treeitem{nullptr};
    TVHITTESTINFO ti{};

    if (pt.x != -1 && pt.y != -1) {
        ti.pt = pt;
        ScreenToClient(m_wnd_tv, &ti.pt);
        TreeView_HitTest(m_wnd_tv, &ti);
        if (ti.hItem && (ti.flags & TVHT_ONITEM)) {
            treeitem = ti.hItem;
        }
    } else {
        treeitem = TreeView_GetSelection(m_wnd_tv);
        RECT rc;
        if (treeitem && TreeView_GetItemRect(m_wnd_tv, treeitem, &rc, TRUE)) {
            MapWindowPoints(m_wnd_tv, HWND_DESKTOP, reinterpret_cast<LPPOINT>(&rc), 2);

            pt.x = rc.left;
            pt.y = rc.top + (rc.bottom - rc.top) / 2;
        } else {
            GetMessagePos(&pt);
        }
    }

    TreeView_Select(m_wnd_tv, treeitem, TVGN_DROPHILITE);

    HMENU menu_view = CreatePopupMenu();
    const size_t view_count = cfg_views.get_count();

    uAppendMenu(menu_view, MF_STRING | (!stricmp_utf8(directory_structure_view_name, m_view) ? MF_CHECKED : 0),
        ID_VIEW_BASE + 0, directory_structure_view_name);

    std::vector<std::string> view_names;
    view_names.emplace_back(directory_structure_view_name);

    for (size_t i = 0; i < view_count; i++) {
        auto view_name = cfg_views.get_name(i);
        view_names.emplace_back(view_name);
        uAppendMenu(menu_view, MF_STRING | (!stricmp_utf8(view_name, m_view) ? MF_CHECKED : 0), ID_VIEW_BASE + i + 1,
            view_name);
    }

    const int IDM_MANAGER_BASE = ID_VIEW_BASE + gsl::narrow<int>(view_names.size());

    uAppendMenu(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(menu_view), "View");

    if (!m_populated && !cfg_populate_on_init)
        uAppendMenu(menu, MF_STRING, ID_REFRESH, "Populate");
    uAppendMenu(menu, MF_STRING | (m_filter ? MF_CHECKED : 0), ID_FILT, "Filter");
    uAppendMenu(menu, MF_STRING, ID_CONF, "Settings");

    bool show_shortcuts = standard_config_objects::query_show_keyboard_shortcuts_in_menus();

    TVITEMEX tvi{};
    tvi.hItem = treeitem;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
    TreeView_GetItem(m_wnd_tv, &tvi);
    auto p_node = treeitem && tvi.lParam ? reinterpret_cast<node*>(tvi.lParam)->shared_from_this() : nullptr;

    if (treeitem && p_node) {
        uAppendMenu(menu, MF_SEPARATOR, 0, "");
        uAppendMenu(menu, MF_STRING, ID_SEND, (show_shortcuts ? "&Send to playlist\tEnter" : "&Send to playlist"));
        uAppendMenu(menu, MF_STRING, ID_ADD, show_shortcuts ? "&Add to playlist\tShift+Enter" : "&Add to playlist");
        uAppendMenu(
            menu, MF_STRING, ID_NEW, show_shortcuts ? "Send to &new playlist\tCtrl+Enter" : "Send to &new playlist");
        uAppendMenu(menu, MF_STRING, ID_AUTOSEND, "Send to &autosend playlist");
        uAppendMenu(menu, MF_SEPARATOR, 0, "");

        contextmenu_manager::g_create(p_menu_manager);
        p_node->sort_entries();

        if (p_menu_manager.is_valid()) {
            p_menu_manager->init_context(p_node->get_entries(), 0);
            p_menu_manager->win32_build_menu(menu, IDM_MANAGER_BASE, -1);
            menu_helpers::win32_auto_mnemonics(menu);
        }
    }

    const int cmd
        = TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, get_wnd(), nullptr);
    DestroyMenu(menu);

    TreeView_Select(m_wnd_tv, NULL, TVGN_DROPHILITE);

    if (cmd > 0) {
        if (p_menu_manager.is_valid() && cmd >= IDM_MANAGER_BASE) {
            p_menu_manager->execute_by_id(cmd - IDM_MANAGER_BASE);
        } else if (cmd >= ID_VIEW_BASE) {
            const unsigned view_index = cmd - ID_VIEW_BASE;
            if (view_index < view_names.size()) {
                m_view = view_names[view_index].c_str();
                refresh_tree();
            }
        } else if (cmd < ID_VIEW_BASE) {
            switch (cmd) {
            case ID_NEW:
                do_playlist(p_node, true, true);
                break;
            case ID_SEND:
                do_playlist(p_node, true);
                break;
            case ID_ADD:
                do_playlist(p_node, false);
                break;
            case ID_AUTOSEND:
                do_autosend_playlist(p_node, m_view, true);
                break;
            case ID_CONF:
                static_api_ptr_t<ui_control>()->show_preferences(album_list_panel_preferences_page_id);
                break;
            case ID_FILT:
                m_filter = !m_filter;
                create_or_destroy_filter();
                break;
            case ID_REFRESH:
                if (!m_populated && !cfg_populate_on_init)
                    refresh_tree();
                break;
            }
        }
    }

    p_menu_manager.release();
    return 0;
}

std::optional<LRESULT> album_list_window::on_tree_view_wm_notify(LPNMHDR hdr)
{
    switch (hdr->code) {
    case TVN_ITEMEXPANDING: {
        auto param = reinterpret_cast<LPNMTREEVIEW>(hdr);
        node_ptr p_node = reinterpret_cast<node*>(param->itemNew.lParam)->shared_from_this();

        if (!p_node->m_children_inserted) {
            TreeViewPopulator::s_setup_children(m_wnd_tv, p_node);
        }

        if (cfg_collapse_other_nodes_on_expansion && param->action == TVE_EXPAND) {
            uih::tree_view_collapse_other_nodes(param->hdr.hwndFrom, param->itemNew.hItem);
        }
        break;
    }
    case TVN_SELCHANGED: {
        auto param = reinterpret_cast<LPNMTREEVIEW>(hdr);
        m_selection
            = param->itemNew.hItem ? reinterpret_cast<node*>(param->itemNew.lParam)->shared_from_this() : nullptr;

        if (param->action == TVC_BYMOUSE || param->action == TVC_BYKEYBOARD) {
            if (cfg_autosend)
                do_autosend_playlist(m_selection, m_view);
        }
        if (m_selection_holder.is_valid()) {
            m_selection_holder->set_selection(m_selection ? m_selection->get_entries() : metadb_handle_list());
        }
        break;
    }
    case NM_CUSTOMDRAW: {
        const auto nmtvcd = (LPNMTVCUSTOMDRAW)(hdr);

        switch (nmtvcd->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            if (cui::colours::helper(album_list_items_colours_client_id).get_themed())
                return CDRF_DODEFAULT;
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT: {
            const auto is_window_focused = GetFocus() == hdr->hwndFrom;
            const auto is_selected = (nmtvcd->nmcd.uItemState & CDIS_SELECTED) != 0;
            const auto is_drop_highlighted
                = (TreeView_GetItemState(hdr->hwndFrom, nmtvcd->nmcd.dwItemSpec, TVIS_DROPHILITED) & TVIS_DROPHILITED)
                != 0;

            cui::colours::helper colour_client(album_list_items_colours_client_id);

            if (is_selected || is_drop_highlighted) {
                nmtvcd->clrText = is_window_focused
                    ? colour_client.get_colour(cui::colours::colour_selection_text)
                    : colour_client.get_colour(cui::colours::colour_inactive_selection_text);
                nmtvcd->clrTextBk = is_window_focused
                    ? colour_client.get_colour(cui::colours::colour_selection_background)
                    : colour_client.get_colour(cui::colours::colour_inactive_selection_background);
            }
            return CDRF_DODEFAULT;
        }
        }

        break;
    }
    }
    return {};
}
