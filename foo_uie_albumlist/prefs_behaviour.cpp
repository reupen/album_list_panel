#include "stdafx.h"

namespace alp {
namespace {

class TabBehaviour : public PreferencesTab {
public:
    HWND create(HWND parent_window) override
    {
        const auto [wnd, _] = fbh::auto_dark_modeless_dialog_box(IDD_BEHAVIOUR_TAB, parent_window,
            [this](auto&&... args) { return handle_message(std::forward<decltype(args)>(args)...); });
        return wnd;
    }
    const char* get_name() override { return "Behaviour"; }

private:
    INT_PTR CALLBACK handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
};

INT_PTR TabBehaviour::handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        SendDlgItemMessage(wnd, IDC_POPULATE, BM_SETCHECK, cfg_populate_on_init, 0);
        SendDlgItemMessage(wnd, IDC_KEYB, BM_SETCHECK, cfg_process_keyboard_shortcuts, 0);
        SendDlgItemMessage(wnd, IDC_AUTOCOLLAPSE, BM_SETCHECK, cfg_collapse_other_nodes_on_expansion, 0);
        SendDlgItemMessage(wnd, IDC_AUTOPLAY, BM_SETCHECK, cfg_play_on_send, 0);
        SendDlgItemMessage(wnd, IDC_ADD_ITEMS_SELECT, BM_SETCHECK, cfg_add_items_select, 0);
        SendDlgItemMessage(wnd, IDC_ADD_ITEMS_USE_CORE_SORT, BM_SETCHECK, cfg_add_items_use_core_sort, 0);

        SendDlgItemMessage(wnd, IDC_AUTO_SEND, BM_SETCHECK, cfg_autosend, 0);

        const auto playlist_name_wnd = GetDlgItem(wnd, IDC_PLAYLIST_NAME);
        uSetWindowText(playlist_name_wnd, cfg_autosend_playlist_name);
        uih::enhance_edit_control(playlist_name_wnd);

        const auto double_click_action_wnd = GetDlgItem(wnd, IDC_DBLCLK);
        uSendMessageText(double_click_action_wnd, CB_ADDSTRING, 0, "Expand or collapse");
        uSendMessageText(double_click_action_wnd, CB_ADDSTRING, 0, "Send to playlist");
        uSendMessageText(double_click_action_wnd, CB_ADDSTRING, 0, "Add to playlist");
        uSendMessageText(double_click_action_wnd, CB_ADDSTRING, 0, "Send to new playlist");
        uSendMessageText(double_click_action_wnd, CB_ADDSTRING, 0, "Send to autosend playlist");
        SendMessage(double_click_action_wnd, CB_SETCURSEL, cfg_double_click_action, 0);

        const auto middle_click_action_wnd = GetDlgItem(wnd, IDC_MIDDLE);
        uSendMessageText(middle_click_action_wnd, CB_ADDSTRING, 0, "None");
        uSendMessageText(middle_click_action_wnd, CB_ADDSTRING, 0, "Send to playlist");
        uSendMessageText(middle_click_action_wnd, CB_ADDSTRING, 0, "Add to playlist");
        uSendMessageText(middle_click_action_wnd, CB_ADDSTRING, 0, "Send to new playlist");
        uSendMessageText(middle_click_action_wnd, CB_ADDSTRING, 0, "Send to autosend playlist");
        SendMessage(middle_click_action_wnd, CB_SETCURSEL, cfg_middle_click_action, 0);

        break;
    }
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
        case IDC_AUTOPLAY:
            cfg_play_on_send = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
        case IDC_KEYB:
            cfg_process_keyboard_shortcuts = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            break;
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
        }
        break;
    }
    return 0;
}

TabBehaviour behaviour_prefs_tab;

} // namespace

PreferencesTab* get_behaviour_prefs_tab()
{
    return &behaviour_prefs_tab;
}

} // namespace alp
