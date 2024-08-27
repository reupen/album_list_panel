#include "stdafx.h"

struct edit_view_param {
    unsigned idx;
    pfc::string8 name, value;
    bool b_new;
};

static INT_PTR CALLBACK EditViewProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp, edit_view_param& state)
{
    switch (msg) {
    case WM_INITDIALOG: {
        uSetDlgItemText(wnd, IDC_NAME, state.name);
        uSetDlgItemText(wnd, IDC_VALUE, state.value);
        break;
    }
    case WM_COMMAND:
        switch (wp) {
        case IDCANCEL:
            EndDialog(wnd, 0);
            break;
        case IDOK: {
            {
                pfc::string8 temp;
                uGetDlgItemText(wnd, IDC_NAME, temp);
                if (temp.is_empty()) {
                    uMessageBox(wnd, "Please enter a valid name.", nullptr, 0);
                    break;
                }
                size_t idx_find = get_views().find_item(temp);
                if (idx_find != -1 && (state.b_new || ((idx_find != state.idx) && (idx_find != -1)))) {
                    uMessageBox(wnd, "View of this name already exists. Please enter another one.", nullptr, 0);
                    break;
                }
                state.name = temp;
            }
            uGetDlgItemText(wnd, IDC_VALUE, state.value);
            EndDialog(wnd, 1);

            break;
        }
        }
        break;
    }
    return FALSE;
}

static bool run_edit_view(edit_view_param& param, HWND parent)
{
    return fbh::auto_dark_modal_dialog_box(IDD_EDIT_VIEW, parent, [&param](auto&&... args) {
        return EditViewProc(std::forward<decltype(args)>(args)..., param);
    }) != 0;
}

cfg_int cfg_child(GUID{0x637c25b6, 0x9166, 0xd8df, 0xae, 0x7a, 0x39, 0x75, 0x78, 0x08, 0xfa, 0xf0}, 0);

TabGeneral g_config_general;

TabAdvanced g_config_advanced;

static PreferencesTab* g_tabs[] = {
    &g_config_general,
    &g_config_advanced,
};

static preferences_page_factory_t<PreferencesPage> foo3;

void TabGeneral::refresh_views()
{
    const auto& views = get_views();

    HWND list = uGetDlgItem(m_wnd, IDC_VIEWS);
    SendMessage(list, LB_RESETCONTENT, 0, 0);
    size_t n, m = views.get_count();
    pfc::string8_fastalloc temp;
    for (n = 0; n < m; n++) {
        views.format_display(n, temp);
        uSendMessageText(list, LB_ADDSTRING, 0, temp);
    }
}

INT_PTR TabGeneral::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        m_wnd = wnd;

        refresh_views();

        SendDlgItemMessage(wnd, IDC_SHOW_NUMBERS, BM_SETCHECK, cfg_show_subitem_counts, 0);
        SendDlgItemMessage(wnd, IDC_SHOW_NUMBERS2, BM_SETCHECK, cfg_show_item_indices, 0);

        HWND list = uGetDlgItem(wnd, IDC_DBLCLK);
        uSendMessageText(list, CB_ADDSTRING, 0, "Expand/collapse (default)");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Add to playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to new playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to autosend playlist");
        SendMessage(list, CB_SETCURSEL, cfg_double_click_action, 0);

        list = uGetDlgItem(wnd, IDC_MIDDLE);
        uSendMessageText(list, CB_ADDSTRING, 0, "None");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Add to playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to new playlist");
        uSendMessageText(list, CB_ADDSTRING, 0, "Send to autosend playlist");
        SendMessage(list, CB_SETCURSEL, cfg_middle_click_action, 0);

        uSetDlgItemText(wnd, IDC_PLAYLIST_NAME, cfg_autosend_playlist_name);

        SendDlgItemMessage(wnd, IDC_AUTO_SEND, BM_SETCHECK, cfg_autosend, 0);

        m_initialised = true;

        break;
    }
    case WM_DESTROY:
        m_initialised = false;
        m_wnd = nullptr;
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDC_MIDDLE | (CBN_SELCHANGE << 16):
            cfg_middle_click_action = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            break;
        case IDC_DBLCLK | (CBN_SELCHANGE << 16):
            cfg_double_click_action = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            break;
        case IDC_AUTO_SEND:
            cfg_autosend = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case (EN_KILLFOCUS << 16) | IDC_PLAYLIST_NAME:
            cfg_autosend_playlist_name = uGetWindowText(reinterpret_cast<HWND>(lp));
            break;
        case IDC_VIEWS | (LBN_DBLCLK << 16): {
            const auto list = reinterpret_cast<HWND>(lp);
            unsigned idx = ListBox_GetCurSel(list);
            if (idx != LB_ERR) {
                auto& views = get_views();

                edit_view_param p;
                p.b_new = false;
                p.idx = idx;
                p.name = views.get_name(idx);
                p.value = views.get_value(idx);
                edit_view_param pbefore = p;
                if (run_edit_view(p, wnd)) {
                    pfc::string8 temp;
                    if (idx < views.get_count()) // modal message loop
                    {
                        views.modify_item(idx, p.name, p.value);
                        views.format_display(idx, temp);
                        SendMessage(list, LB_DELETESTRING, idx, 0);
                        uSendMessageText(list, LB_INSERTSTRING, idx, temp);
                        uSendMessageText(list, LB_SETCURSEL, idx, nullptr);
                        AlbumListWindow::s_on_view_script_change(pbefore.name, p.name);
                    }
                }
            }
            break;
        }
        case IDC_VIEW_UP: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            auto idx = ListBox_GetCurSel(list);

            if (idx != LB_ERR && idx > 0) {
                auto& views = get_views();
                SendMessage(list, LB_DELETESTRING, idx, 0);
                views.swap(idx, idx - 1);
                pfc::string8 temp;
                views.format_display(idx - 1, temp);
                uSendMessageText(list, LB_INSERTSTRING, idx - 1, temp);
                SendMessage(list, LB_SETCURSEL, idx - 1, 0);
            }
            break;
        }
        case IDC_VIEW_DOWN: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            auto& views = get_views();
            auto idx = ListBox_GetCurSel(list);

            if (idx != LB_ERR && gsl::narrow<size_t>(idx) + 1 < views.get_count()) {
                SendMessage(list, LB_DELETESTRING, idx, 0);
                views.swap(idx, idx + 1);
                pfc::string8 temp;
                views.format_display(idx + 1, temp);
                uSendMessageText(list, LB_INSERTSTRING, idx + 1, temp);
                SendMessage(list, LB_SETCURSEL, idx + 1, 0);
            }
            break;
        }
        case IDC_VIEW_DELETE: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            auto idx = ListBox_GetCurSel(list);
            if (idx != LB_ERR) {
                get_views().remove_item(idx);
                SendDlgItemMessage(wnd, IDC_VIEWS, LB_DELETESTRING, idx, 0);
            }
            break;
        }
        case IDC_VIEW_NEW: {
            edit_view_param p;
            p.b_new = true;
            p.idx = -1;
            if (run_edit_view(p, wnd)) {
                auto& views = get_views();
                HWND list = uGetDlgItem(wnd, IDC_VIEWS);
                size_t n = views.add_item(p.name, p.value);
                pfc::string8 temp;
                views.format_display(n, temp);
                uSendMessageText(list, LB_ADDSTRING, 0, temp);
                SendMessage(list, LB_SETCURSEL, n, 0);
            }
            break;
        }
        case IDC_VIEW_RESET: {
            auto& views = get_views();
            views.reset();
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            SendMessage(list, LB_RESETCONTENT, 0, 0);
            size_t n, m = views.get_count();
            pfc::string8_fastalloc temp;
            for (n = 0; n < m; n++) {
                views.format_display(n, temp);
                uSendMessageText(list, LB_ADDSTRING, 0, temp);
            }
            break;
        }
        }
        break;
    }
    return 0;
}

namespace {

class FontClient : public cui::fonts::client {
public:
    const GUID& get_client_guid() const override { return album_list_font_client_id; }

    void get_name(pfc::string_base& p_out) const override { p_out = "Album List"; }

    cui::fonts::font_type_t get_default_font_type() const override { return cui::fonts::font_type_items; }

    void on_font_changed() const override { AlbumListWindow::s_update_all_fonts(); }
};

FontClient::factory<FontClient> g_font_client_album_list;

class ItemsColoursClient : public cui::colours::client {
public:
    const GUID& get_client_guid() const override { return album_list_items_colours_client_id; }
    void get_name(pfc::string_base& p_out) const override { p_out = "Album List: Items"; }
    uint32_t get_supported_colours() const override
    {
        uint32_t flags = cui::colours::colour_flag_all
            & ~(cui::colours::colour_flag_group_foreground | cui::colours::colour_flag_group_background);

        if (cui::colours::is_dark_mode_active()) {
            flags &= ~(cui::colours::colour_flag_active_item_frame);
        }

        return flags;
    }
    uint32_t get_supported_bools() const override { return cui::colours::bool_flag_dark_mode_enabled; }
    bool get_themes_supported() const override { return true; }

    void on_colour_changed(uint32_t mask) const override { AlbumListWindow::s_update_all_tree_colours(); }
    void on_bool_changed(uint32_t mask) const override
    {
        if (mask & cui::colours::bool_flag_dark_mode_enabled)
            AlbumListWindow::s_update_all_tree_themes();
    }
};

cui::colours::client::factory<ItemsColoursClient> g_items_colours_client;

class FilterColoursClient : public cui::colours::client {
public:
    const GUID& get_client_guid() const override { return album_list_filter_colours_client_id; }
    void get_name(pfc::string_base& p_out) const override { p_out = "Album List: Filter"; }
    uint32_t get_supported_colours() const override
    {
        return cui::colours::colour_flag_background | cui::colours::colour_flag_text;
    }
    uint32_t get_supported_bools() const override { return cui::colours::bool_flag_dark_mode_enabled; }
    bool get_themes_supported() const override { return false; }

    void on_colour_changed(uint32_t mask) const override { AlbumListWindow::s_update_all_edit_colours(); }
    void on_bool_changed(uint32_t mask) const override
    {
        if (mask & cui::colours::bool_flag_dark_mode_enabled)
            AlbumListWindow::s_update_all_edit_themes();
    }
};

cui::colours::client::factory<FilterColoursClient> g_filter_colours_client;

}; // namespace

INT_PTR TabAdvanced::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        SendDlgItemMessage(wnd, IDC_SHOW_NUMBERS, BM_SETCHECK, cfg_show_subitem_counts, 0);
        SendDlgItemMessage(wnd, IDC_SHOW_NUMBERS2, BM_SETCHECK, cfg_show_item_indices, 0);
        HWND list = uGetDlgItem(wnd, IDC_FRAME);

        uSendMessageText(list, CB_ADDSTRING, 0, "None");
        uSendMessageText(list, CB_ADDSTRING, 0, "Sunken");
        uSendMessageText(list, CB_ADDSTRING, 0, "Grey");

        SendMessage(list, CB_SETCURSEL, cfg_frame_style, 0);

        SendDlgItemMessage(wnd, IDC_KEYB, BM_SETCHECK, cfg_process_keyboard_shortcuts, 0);
        SendDlgItemMessage(wnd, IDC_POPULATE, BM_SETCHECK, cfg_populate_on_init, 0);
        SendDlgItemMessage(wnd, IDC_HSCROLL, BM_SETCHECK, cfg_show_horizontal_scroll_bar, 0);
        SendDlgItemMessage(wnd, IDC_SHOW_ROOT, BM_SETCHECK, cfg_show_root_node, 0);
        SendDlgItemMessage(wnd, IDC_AUTOPLAY, BM_SETCHECK, cfg_play_on_send, 0);
        SendDlgItemMessage(wnd, IDC_ADD_ITEMS_USE_CORE_SORT, BM_SETCHECK, cfg_add_items_use_core_sort, 0);
        SendDlgItemMessage(wnd, IDC_ADD_ITEMS_SELECT, BM_SETCHECK, cfg_add_items_select, 0);
        SendDlgItemMessage(wnd, IDC_AUTOCOLLAPSE, BM_SETCHECK, cfg_collapse_other_nodes_on_expansion, 0);

        SendDlgItemMessage(wnd, IDC_USE_INDENT, BM_SETCHECK, cfg_use_custom_indentation, 0);
        HWND wnd_indent = GetDlgItem(wnd, IDC_INDENT);

        EnableWindow(wnd_indent, cfg_use_custom_indentation);
        EnableWindow(GetDlgItem(wnd, IDC_INDENT_SPIN), cfg_use_custom_indentation);
        if (cfg_use_custom_indentation)
            SetDlgItemInt(wnd, IDC_INDENT, cfg_custom_indentation_amount, TRUE);
        else
            uSendMessageText(wnd_indent, WM_SETTEXT, 0, "");

        SendDlgItemMessage(wnd, IDC_INDENT_SPIN, UDM_SETRANGE32, 1, 999);
        SendDlgItemMessage(wnd, IDC_USE_ITEM_HEIGHT, BM_SETCHECK, cfg_use_custom_vertical_item_padding, 0);

        HWND wnd_item_height = GetDlgItem(wnd, IDC_ITEM_HEIGHT);

        EnableWindow(wnd_item_height, cfg_use_custom_vertical_item_padding);
        EnableWindow(GetDlgItem(wnd, IDC_ITEM_HEIGHT_SPIN), cfg_use_custom_vertical_item_padding);

        if (cfg_use_custom_vertical_item_padding)
            SetDlgItemInt(wnd, IDC_ITEM_HEIGHT, cfg_custom_vertical_padding_amount, TRUE);
        else
            uSendMessageText(wnd_item_height, WM_SETTEXT, 0, "");

        SendDlgItemMessage(wnd, IDC_ITEM_HEIGHT_SPIN, UDM_SETRANGE32, -99, 99);

        m_initialised = true;

        break;
    }
    case WM_DESTROY:
        m_initialised = false;
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDC_AUTOCOLLAPSE:
            cfg_collapse_other_nodes_on_expansion = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_ADD_ITEMS_USE_CORE_SORT:
            cfg_add_items_use_core_sort = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_mark_tracks_unsorted();
            break;
        case IDC_ADD_ITEMS_SELECT:
            cfg_add_items_select = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_POPULATE:
            cfg_populate_on_init = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_SHOW_NUMBERS:
            cfg_show_subitem_counts = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_labels();
            break;
        case IDC_AUTO_SEND:
            cfg_autosend = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_HSCROLL:
            cfg_show_horizontal_scroll_bar = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_showhscroll();
            break;
        case IDC_SHOW_ROOT:
            cfg_show_root_node = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_refresh_all();
            break;
        case IDC_AUTOPLAY:
            cfg_play_on_send = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_KEYB:
            cfg_process_keyboard_shortcuts = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_SHOW_NUMBERS2:
            cfg_show_item_indices = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_labels();
            break;
        case (CBN_SELCHANGE << 16) | IDC_FRAME: {
            cfg_frame_style = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            AlbumListWindow::s_update_all_window_frames();
        } break;
        case (EN_CHANGE << 16) | IDC_ITEM_HEIGHT: {
            if (m_initialised && cfg_use_custom_vertical_item_padding) {
                BOOL result;
                int new_height = GetDlgItemInt(wnd, IDC_ITEM_HEIGHT, &result, TRUE);
                if (result) {
                    if (new_height > 99)
                        new_height = 99;
                    if (new_height < -99)
                        new_height = -99;
                    cfg_custom_vertical_padding_amount = new_height;
                    AlbumListWindow::s_update_all_item_heights();
                }
            }
        } break;
        case IDC_USE_ITEM_HEIGHT: {
            cfg_use_custom_vertical_item_padding = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            HWND wnd_indent = GetDlgItem(wnd, IDC_ITEM_HEIGHT);

            EnableWindow(wnd_indent, cfg_use_custom_vertical_item_padding);
            EnableWindow(GetDlgItem(wnd, IDC_ITEM_HEIGHT_SPIN), cfg_use_custom_vertical_item_padding);

            if (cfg_use_custom_vertical_item_padding)
                SetDlgItemInt(wnd, IDC_ITEM_HEIGHT, cfg_custom_vertical_padding_amount, TRUE);
            else
                uSendMessageText(wnd_indent, WM_SETTEXT, 0, "");

            AlbumListWindow::s_update_all_item_heights();
        } break;
        case IDC_USE_INDENT: {
            cfg_use_custom_indentation = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            HWND wnd_indent = GetDlgItem(wnd, IDC_INDENT);

            EnableWindow(wnd_indent, cfg_use_custom_indentation);
            EnableWindow(GetDlgItem(wnd, IDC_INDENT_SPIN), cfg_use_custom_indentation);
            if (cfg_use_custom_indentation)
                SetDlgItemInt(wnd, IDC_INDENT, cfg_custom_indentation_amount, TRUE);
            else
                uSendMessageText(wnd_indent, WM_SETTEXT, 0, "");

            AlbumListWindow::s_update_all_indents();
        } break;
        case (EN_CHANGE << 16) | IDC_INDENT: {
            if (m_initialised && cfg_use_custom_indentation) {
                BOOL result;
                unsigned new_height = GetDlgItemInt(wnd, IDC_INDENT, &result, TRUE);
                if (result) {
                    if (new_height > 999)
                        new_height = 999;
                    cfg_custom_indentation_amount = new_height;
                    AlbumListWindow::s_update_all_indents();
                }
            }
        } break;
        }
        break;
    }
    return 0;
}

void PreferencesPageInstance::make_child(HWND wnd)
{
    if (m_child) {
        ShowWindow(m_child, SW_HIDE);
        DestroyWindow(m_child);
        m_child = nullptr;
    }

    HWND wnd_tab = GetDlgItem(wnd, IDC_TAB1);

    RECT tab;

    GetWindowRect(wnd_tab, &tab);
    MapWindowPoints(HWND_DESKTOP, wnd, (LPPOINT)&tab, 2);

    TabCtrl_AdjustRect(wnd_tab, FALSE, &tab);

    unsigned count = tabsize(g_tabs);
    if ((unsigned)cfg_child >= count)
        cfg_child = 0;

    if ((unsigned)cfg_child < count && cfg_child >= 0) {
        m_child = g_tabs[cfg_child]->create(wnd);
    }

    if (m_child) {
        EnableThemeDialogTexture(m_child, ETDT_ENABLETAB);
    }

    SetWindowPos(m_child, nullptr, tab.left, tab.top, tab.right - tab.left, tab.bottom - tab.top, SWP_NOZORDER);
    SetWindowPos(wnd_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShowWindow(m_child, SW_SHOWNORMAL);
}

INT_PTR PreferencesPageInstance::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        m_wnd = wnd;
        HWND wnd_tab = GetDlgItem(wnd, IDC_TAB1);
        unsigned n, count = tabsize(g_tabs);
        for (n = 0; n < count; n++) {
            uTabCtrl_InsertItemText(wnd_tab, n, g_tabs[n]->get_name());
        }
        TabCtrl_SetCurSel(wnd_tab, cfg_child);
        make_child(wnd);
        break;
    }
    case WM_DESTROY:
        m_wnd = nullptr;
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lp)->idFrom) {
        case IDC_TAB1:
            switch (((LPNMHDR)lp)->code) {
            case TCN_SELCHANGE: {
                cfg_child = TabCtrl_GetCurSel(GetDlgItem(wnd, IDC_TAB1));
                make_child(wnd);
            } break;
            }
            break;
        }
        break;
    case WM_PARENTNOTIFY:
        switch (wp) {
        case WM_DESTROY: {
            if (m_wnd && (HWND)lp == m_wnd)
                m_wnd = nullptr;
        } break;
        }
        break;
    }
    return 0;
}
