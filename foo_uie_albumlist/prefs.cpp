#include "stdafx.h"

const GUID g_guid_preferences_album_list_panel{
    0x53c89e50, 0x685d, 0x8ed1, 0x43, 0x25, 0x6b, 0xe8, 0x0f, 0x1b, 0xe7, 0x1f
};

struct edit_view_param {
    unsigned idx;
    pfc::string8 name, value;
    bool b_new;
};

static BOOL CALLBACK EditViewProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(wnd, DWLP_USER, lp); {
            const auto ptr = reinterpret_cast<edit_view_param*>(lp);
            uSetDlgItemText(wnd, IDC_NAME, ptr->name);
            uSetDlgItemText(wnd, IDC_VALUE, ptr->value);
        }
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDCANCEL:
            EndDialog(wnd, 0);
            break;
        case IDOK: {
            auto ptr = reinterpret_cast<edit_view_param*>(GetWindowLongPtr(wnd, DWLP_USER)); {
                pfc::string8 temp;
                uGetDlgItemText(wnd, IDC_NAME, temp);
                if (temp.is_empty()) {
                    uMessageBox(wnd, "Please enter a valid name.", nullptr, 0);
                    break;
                }
                unsigned idx_find = cfg_views.find_item(temp);
                if (idx_find != -1 && (ptr->b_new || ((idx_find != ptr->idx) && (idx_find != -1)))) {
                    uMessageBox(wnd, "View of this name already exists. Please enter another one.", nullptr, 0);
                    break;
                }
                ptr->name = temp;
            }
            uGetDlgItemText(wnd, IDC_VALUE, ptr->value);
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
    return DialogBoxParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_EDIT_VIEW), parent, EditViewProc, reinterpret_cast<LPARAM>(&param)) != 0;
}

cfg_int cfg_child(GUID{0x637c25b6, 0x9166, 0xd8df, 0xae, 0x7a, 0x39, 0x75, 0x78, 0x08, 0xfa, 0xf0}, 0);

tab_general g_config_general;

tab_advanced g_config_advanced;

static preferences_tab* g_tabs[] =
{
    &g_config_general,
    &g_config_advanced,
};


HWND config_albumlist::child = nullptr;

static preferences_page_factory_t<config_albumlist> foo3;

bool tab_advanced::initialised = false;

void tab_general::refresh_views()
{ {
        HWND list = uGetDlgItem(m_wnd, IDC_VIEWS);
        SendMessage(list, LB_RESETCONTENT, 0, 0);
        unsigned n, m = cfg_views.get_count();
        pfc::string8_fastalloc temp;
        for (n = 0; n < m; n++) {
            cfg_views.format_display(n, temp);
            uSendMessageText(list, LB_ADDSTRING, 0, temp);
        }
    }
}

BOOL tab_general::g_on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    tab_general* p_data = nullptr;
    if (msg == WM_INITDIALOG) {
        p_data = reinterpret_cast<tab_general*>(lp);
        SetWindowLongPtr(wnd, DWL_USER, lp);
    }
    else
        p_data = reinterpret_cast<tab_general*>(GetWindowLongPtr(wnd, DWL_USER));
    return p_data ? p_data->on_message(wnd, msg, wp, lp) : FALSE;
}

BOOL tab_general::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
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
    case WM_COMMAND:
        switch (wp) {
        case IDC_MIDDLE | (CBN_SELCHANGE << 16):
            cfg_middle_click_action = SendMessage((HWND)lp, CB_GETCURSEL, 0, 0);
            break;
        case IDC_DBLCLK | (CBN_SELCHANGE << 16):
            cfg_double_click_action = SendMessage((HWND)lp, CB_GETCURSEL, 0, 0);
            break;
        case IDC_AUTO_SEND:
            cfg_autosend = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case (EN_KILLFOCUS << 16) | IDC_PLAYLIST_NAME:
            cfg_autosend_playlist_name = string_utf8_from_window((HWND)lp);
            break;
        case IDC_VIEWS | (LBN_DBLCLK << 16): {
            const auto list = reinterpret_cast<HWND>(lp);
            unsigned idx = SendMessage(list, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR) {
                edit_view_param p;
                p.b_new = false;
                p.idx = idx;
                p.name = cfg_views.get_name(idx);
                p.value = cfg_views.get_value(idx);
                edit_view_param pbefore = p;
                if (run_edit_view(p, wnd)) {
                    pfc::string8 temp;
                    if (idx < cfg_views.get_count()) //modal message loop
                    {
                        cfg_views.modify_item(idx, p.name, p.value);
                        cfg_views.format_display(idx, temp);
                        SendMessage(list, LB_DELETESTRING, idx, 0);
                        uSendMessageText(list, LB_INSERTSTRING, idx, temp);
                        uSendMessageText(list, LB_SETCURSEL, idx, nullptr);
                        album_list_window::s_on_view_script_change(pbefore.name, p.name);
                    }
                }
            }
            break;
        }
        case IDC_VIEW_UP: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            unsigned idx = SendMessage(list, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR && idx > 0) {
                SendMessage(list, LB_DELETESTRING, idx, 0);
                cfg_views.swap(idx, idx - 1);
                pfc::string8 temp;
                cfg_views.format_display(idx - 1, temp);
                uSendMessageText(list, LB_INSERTSTRING, idx - 1, temp);
                SendMessage(list, LB_SETCURSEL, idx - 1, 0);
            }
            break;
        }
        case IDC_VIEW_DOWN: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            unsigned idx = SendMessage(list, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR && idx + 1 < cfg_views.get_count()) {
                SendMessage(list, LB_DELETESTRING, idx, 0);
                cfg_views.swap(idx, idx + 1);
                pfc::string8 temp;
                cfg_views.format_display(idx + 1, temp);
                uSendMessageText(list, LB_INSERTSTRING, idx + 1, temp);
                SendMessage(list, LB_SETCURSEL, idx + 1, 0);
            }
            break;
        }
        case IDC_VIEW_DELETE: {
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            unsigned idx = SendMessage(list, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR) {
                cfg_views.remove_item(idx);
                SendDlgItemMessage(wnd, IDC_VIEWS, LB_DELETESTRING, idx, 0);
            }
            break;
        }
        case IDC_VIEW_NEW: {
            edit_view_param p;
            p.b_new = true;
            p.idx = -1;
            if (run_edit_view(p, wnd)) {
                HWND list = uGetDlgItem(wnd, IDC_VIEWS);
                unsigned n = cfg_views.add_item(p.name, p.value);
                pfc::string8 temp;
                cfg_views.format_display(n, temp);
                uSendMessageText(list, LB_ADDSTRING, 0, temp);
                SendMessage(list, LB_SETCURSEL, n, 0);
            }
            break;
        }
        case IDC_VIEW_RESET: {
            cfg_views.reset();
            HWND list = uGetDlgItem(wnd, IDC_VIEWS);
            SendMessage(list, LB_RESETCONTENT, 0, 0);
            unsigned n, m = cfg_views.get_count();
            pfc::string8_fastalloc temp;
            for (n = 0; n < m; n++) {
                cfg_views.format_display(n, temp);
                uSendMessageText(list, LB_ADDSTRING, 0, temp);
            }
            break;
        }
        }
        break;
    case WM_DESTROY:
        //history_sort.add_item(cfg_sort_order);
        m_initialised = false;
        m_wnd = nullptr;
        break;
    }
    return 0;
}

class font_client_album_list : public cui::fonts::client {
public:
    const GUID& get_client_guid() const override
    {
        return g_guid_album_list_font;
    }

    void get_name(pfc::string_base& p_out) const override
    {
        p_out = "Album List";
    }

    cui::fonts::font_type_t get_default_font_type() const override
    {
        return cui::fonts::font_type_items;
    }

    void on_font_changed() const override
    {
        album_list_window::s_update_all_fonts();
    }
};

font_client_album_list::factory<font_client_album_list> g_font_client_album_list;

class appearance_client_filter_impl : public cui::colours::client {
public:
    const GUID& get_client_guid() const override { return g_guid_album_list_colours; };
    void get_name(pfc::string_base& p_out) const override { p_out = "Album List"; };
    t_size get_supported_bools() const override { return 0; }; //bit-mask
    bool get_themes_supported() const override { return true; };

    void on_colour_changed(t_size mask) const override
    {
        album_list_window::s_update_all_colours();
    };
    void on_bool_changed(t_size mask) const override {};
};

namespace {
    cui::colours::client::factory<appearance_client_filter_impl> g_appearance_client_impl;
};

BOOL tab_advanced::ConfigProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
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

        SendDlgItemMessage(wnd, IDC_INDENT_SPIN, UDM_SETRANGE32, 0, 999);
        SendDlgItemMessage(wnd, IDC_USE_ITEM_HEIGHT, BM_SETCHECK, cfg_use_custom_vertical_item_padding, 0);

        HWND wnd_item_height = GetDlgItem(wnd, IDC_ITEM_HEIGHT);

        EnableWindow(wnd_item_height, cfg_use_custom_vertical_item_padding);
        EnableWindow(GetDlgItem(wnd, IDC_ITEM_HEIGHT_SPIN), cfg_use_custom_vertical_item_padding);

        if (cfg_use_custom_vertical_item_padding)
            SetDlgItemInt(wnd, IDC_ITEM_HEIGHT, cfg_custom_vertical_padding_amount, TRUE);
        else
            uSendMessageText(wnd_item_height, WM_SETTEXT, 0, "");

        SendDlgItemMessage(wnd, IDC_ITEM_HEIGHT_SPIN, UDM_SETRANGE32, -99, 99);

        initialised = true;

        break;
    }
    case WM_COMMAND:
        switch (wp) {
        case IDC_AUTOCOLLAPSE:
            cfg_collapse_other_nodes_on_expansion = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_ADD_ITEMS_USE_CORE_SORT:
            cfg_add_items_use_core_sort = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_ADD_ITEMS_SELECT:
            cfg_add_items_select = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_POPULATE:
            cfg_populate_on_init = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_SHOW_NUMBERS:
            cfg_show_subitem_counts = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            album_list_window::s_update_all_labels();
            break;
        case IDC_AUTO_SEND:
            cfg_autosend = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_HSCROLL:
            cfg_show_horizontal_scroll_bar = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            album_list_window::s_update_all_showhscroll();
            break;
        case IDC_SHOW_ROOT:
            cfg_show_root_node = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            album_list_window::s_refresh_all();
            break;
        case IDC_AUTOPLAY:
            cfg_play_on_send = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_KEYB:
            cfg_process_keyboard_shortcuts = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            break;
        case IDC_SHOW_NUMBERS2:
            cfg_show_item_indices = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            album_list_window::s_update_all_labels();
            break;
        case (CBN_SELCHANGE << 16) | IDC_FRAME: {
            cfg_frame_style = SendMessage((HWND)lp, CB_GETCURSEL, 0, 0);
            album_list_window::s_update_all_window_frames();
        }
            break;
        case (EN_CHANGE << 16) | IDC_ITEM_HEIGHT: {
            if (initialised && cfg_use_custom_vertical_item_padding) {
                BOOL result;
                int new_height = GetDlgItemInt(wnd, IDC_ITEM_HEIGHT, &result, TRUE);
                if (result) {
                    if (new_height > 99)
                        new_height = 99;
                    if (new_height < -99)
                        new_height = -99;
                    cfg_custom_vertical_padding_amount = new_height;
                    album_list_window::s_update_all_item_heights();
                }
            }
        }
            break;
        case IDC_USE_ITEM_HEIGHT: {
            cfg_use_custom_vertical_item_padding = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            HWND wnd_indent = GetDlgItem(wnd, IDC_ITEM_HEIGHT);

            EnableWindow(wnd_indent, cfg_use_custom_vertical_item_padding);
            EnableWindow(GetDlgItem(wnd, IDC_ITEM_HEIGHT_SPIN), cfg_use_custom_vertical_item_padding);

            if (cfg_use_custom_vertical_item_padding)
                SetDlgItemInt(wnd, IDC_ITEM_HEIGHT, cfg_custom_vertical_padding_amount, TRUE);
            else
                uSendMessageText(wnd_indent, WM_SETTEXT, 0, "");

            album_list_window::s_update_all_item_heights();
        }
            break;
        case IDC_USE_INDENT: {
            cfg_use_custom_indentation = SendMessage((HWND)lp, BM_GETCHECK, 0, 0);
            HWND wnd_indent = GetDlgItem(wnd, IDC_INDENT);

            EnableWindow(wnd_indent, cfg_use_custom_indentation);
            EnableWindow(GetDlgItem(wnd, IDC_INDENT_SPIN), cfg_use_custom_indentation);
            if (cfg_use_custom_indentation)
                SetDlgItemInt(wnd, IDC_INDENT, cfg_custom_indentation_amount, TRUE);
            else
                uSendMessageText(wnd_indent, WM_SETTEXT, 0, "");

            album_list_window::s_update_all_indents();
        }
            break;
        case (EN_CHANGE << 16) | IDC_INDENT: {
            if (initialised && cfg_use_custom_indentation) {
                BOOL result;
                unsigned new_height = GetDlgItemInt(wnd, IDC_INDENT, &result, TRUE);
                if (result) {
                    if (new_height > 999)
                        new_height = 999;
                    cfg_custom_indentation_amount = new_height;
                    album_list_window::s_update_all_indents();
                }
            }
        }
            break;
        }
        break;
    case WM_DESTROY:
        initialised = false;
        break;
    }
    return 0;
}

void config_albumlist::make_child(HWND wnd)
{
    if (child) {
        ShowWindow(child, SW_HIDE);
        DestroyWindow(child);
        child = nullptr;
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
        child = g_tabs[cfg_child]->create(wnd);
    }

    if (child) { {
            EnableThemeDialogTexture(child, ETDT_ENABLETAB);
        }
    }

    SetWindowPos(child, nullptr, tab.left, tab.top, tab.right - tab.left, tab.bottom - tab.top, SWP_NOZORDER);
    SetWindowPos(wnd_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShowWindow(child, SW_SHOWNORMAL);
}

BOOL config_albumlist::ConfigProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        HWND wnd_tab = GetDlgItem(wnd, IDC_TAB1);
        unsigned n, count = tabsize(g_tabs);
        for (n = 0; n < count; n++) {
            uTabCtrl_InsertItemText(wnd_tab, n, g_tabs[n]->get_name());
        }
        TabCtrl_SetCurSel(wnd_tab, cfg_child);
        make_child(wnd);
    }
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lp)->idFrom) {
        case IDC_TAB1:
            switch (((LPNMHDR)lp)->code) {
            case TCN_SELCHANGE: {
                cfg_child = TabCtrl_GetCurSel(GetDlgItem(wnd, IDC_TAB1));
                make_child(wnd);
            }
                break;
            }
            break;
        }
        break;
    case WM_PARENTNOTIFY:
        switch (wp) {
        case WM_DESTROY: {
            if (child && (HWND)lp == child)
                child = nullptr;
        }
            break;
        }
        break;
    }
    return 0;
}
