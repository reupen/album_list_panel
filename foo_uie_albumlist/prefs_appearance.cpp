#include "stdafx.h"

namespace alp {
namespace {

class TabAppearance : public PreferencesTab {
public:
    HWND create(HWND parent_window) override
    {
        const auto [wnd, _] = fbh::auto_dark_modeless_dialog_box(IDD_APPEARANCE_TAB, parent_window,
            [this](auto&&... args) { return handle_message(std::forward<decltype(args)>(args)...); });
        return wnd;
    }
    const char* get_name() override { return "Appearance"; }

private:
    INT_PTR CALLBACK handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    void update_show_root_node_expand_button()
    {
        const auto show_root_node_expand_button_wnd = GetDlgItem(m_wnd, IDC_SHOW_ROOT_EXPAND_BUTTON);
        EnableWindow(show_root_node_expand_button_wnd, cfg_show_root_node);
        Button_SetCheck(show_root_node_expand_button_wnd,
            cfg_show_root_node && cfg_show_root_node_expand_button ? BST_CHECKED : BST_UNCHECKED);
    }

    bool m_initialised{};
    HWND m_wnd{};
};

INT_PTR TabAppearance::handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        m_wnd = wnd;
        SendDlgItemMessage(wnd, IDC_SHOW_ROOT, BM_SETCHECK, cfg_show_root_node, 0);
        update_show_root_node_expand_button();

        SendDlgItemMessage(wnd, IDC_SHOW_INDICES, BM_SETCHECK, cfg_show_item_indices, 0);
        SendDlgItemMessage(wnd, IDC_SHOW_COUNTS, BM_SETCHECK, cfg_show_subitem_counts, 0);
        SendDlgItemMessage(wnd, IDC_HSCROLL, BM_SETCHECK, cfg_show_horizontal_scroll_bar, 0);

        SendDlgItemMessage(wnd, IDC_USE_ITEM_HEIGHT, BM_SETCHECK, cfg_use_custom_vertical_item_padding, 0);

        const auto item_height_wnd = GetDlgItem(wnd, IDC_ITEM_HEIGHT);

        EnableWindow(item_height_wnd, cfg_use_custom_vertical_item_padding);
        EnableWindow(GetDlgItem(wnd, IDC_ITEM_HEIGHT_SPIN), cfg_use_custom_vertical_item_padding);

        if (cfg_use_custom_vertical_item_padding)
            SetDlgItemInt(wnd, IDC_ITEM_HEIGHT, cfg_custom_vertical_padding_amount, TRUE);
        else
            uSendMessageText(item_height_wnd, WM_SETTEXT, 0, "");

        uih::enhance_edit_control(item_height_wnd);

        SendDlgItemMessage(wnd, IDC_ITEM_HEIGHT_SPIN, UDM_SETRANGE32, -99, 99);

        SendDlgItemMessage(wnd, IDC_USE_INDENT, BM_SETCHECK, cfg_use_custom_indentation, 0);
        const auto indent_wnd = GetDlgItem(wnd, IDC_INDENT);

        EnableWindow(indent_wnd, cfg_use_custom_indentation);
        EnableWindow(GetDlgItem(wnd, IDC_INDENT_SPIN), cfg_use_custom_indentation);
        if (cfg_use_custom_indentation)
            SetDlgItemInt(wnd, IDC_INDENT, cfg_custom_indentation_amount, TRUE);
        else
            uSendMessageText(indent_wnd, WM_SETTEXT, 0, "");

        uih::enhance_edit_control(indent_wnd);

        SendDlgItemMessage(wnd, IDC_INDENT_SPIN, UDM_SETRANGE32, 1, 999);
        const auto edge_style_wnd = GetDlgItem(wnd, IDC_FRAME);

        uSendMessageText(edge_style_wnd, CB_ADDSTRING, 0, "None");
        uSendMessageText(edge_style_wnd, CB_ADDSTRING, 0, "Sunken");
        uSendMessageText(edge_style_wnd, CB_ADDSTRING, 0, "Grey");

        SendMessage(edge_style_wnd, CB_SETCURSEL, cfg_frame_style, 0);

        m_initialised = true;

        break;
    }
    case WM_DESTROY:
        m_initialised = false;
        m_wnd = nullptr;
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDC_HSCROLL:
            cfg_show_horizontal_scroll_bar = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_showhscroll();
            break;
        case IDC_SHOW_ROOT:
            cfg_show_root_node = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            update_show_root_node_expand_button();
            AlbumListWindow::s_update_all_show_root_expand_button();
            AlbumListWindow::s_refresh_all();
            break;
        case IDC_SHOW_ROOT_EXPAND_BUTTON:
            cfg_show_root_node_expand_button = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_show_root_expand_button();
            break;
        case IDC_SHOW_COUNTS:
            cfg_show_subitem_counts = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_labels();
            break;
        case IDC_SHOW_INDICES:
            cfg_show_item_indices = Button_GetCheck(reinterpret_cast<HWND>(lp)) != BST_UNCHECKED;
            AlbumListWindow::s_update_all_labels();
            break;
        case (CBN_SELCHANGE << 16) | IDC_FRAME:
            cfg_frame_style = ComboBox_GetCurSel(reinterpret_cast<HWND>(lp));
            AlbumListWindow::s_update_all_window_frames();
            break;
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
            break;
        }
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
            break;
        }
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
            break;
        }
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
            break;
        }
        }
        break;
    }
    return 0;
}

TabAppearance appearance_prefs_tab;

} // namespace

PreferencesTab* get_appearance_prefs_tab()
{
    return &appearance_prefs_tab;
}

} // namespace alp
