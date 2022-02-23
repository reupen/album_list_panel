#include "stdafx.h"
#include "actions.h"

LRESULT WINAPI album_list_window::s_hook_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto p_this = reinterpret_cast<album_list_window*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

    return p_this ? p_this->on_hook(wnd, msg, wp, lp) : DefWindowProc(wnd, msg, wp, lp);
}

static bool test_point_distance(POINT pt1, POINT pt2, int test)
{
    const int dx = pt1.x - pt2.x;
    const int dy = pt1.y - pt2.y;
    return dx * dx + dy * dy > test * test;
}

LRESULT WINAPI album_list_window::on_hook(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_NOTIFY: {
        const auto hdr = reinterpret_cast<LPNMHDR>(lp);

        if (hdr->code != TTN_SHOW)
            break;

        std::array<wchar_t, 128> class_name{};
        GetClassName(hdr->hwndFrom, class_name.data(), class_name.size());

        if (wcsncmp(class_name.data(), TOOLTIPS_CLASSW, class_name.size()) != 0)
            break;

        const auto is_dark = cui::colours::is_dark_mode_active();
        SetWindowTheme(hdr->hwndFrom, is_dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        uie::window_ptr p_this{this};
        bool processed = false;

        if (get_host()->get_keyboard_shortcuts_enabled() && wp != VK_LEFT && wp != VK_RIGHT && cfg_process_keyboard_shortcuts)
            processed = g_process_keydown_keyboard_shortcuts(wp);

        m_process_char = !processed;

        if (!processed && msg == WM_KEYDOWN && wp == VK_TAB) {
            g_on_tab(wnd);
        }
        break;
    }
    case WM_CHAR:
        if (cfg_process_keyboard_shortcuts && !m_process_char) {
            m_process_char = true;
            return 0;
        }
        break;
    case WM_SETFOCUS: {
        m_selection_holder = static_api_ptr_t<ui_selection_manager>()->acquire();
        if (m_selection)
            m_selection_holder->set_selection(m_selection->get_entries());
        break;
    }
    case WM_KILLFOCUS: {
        m_selection_holder.release();
        break;
    }
    case WM_GETDLGCODE: {
        const auto lpmsg = reinterpret_cast<LPMSG>(lp);
        if (lpmsg && cfg_process_keyboard_shortcuts) {
            // let dialog manager handle it, otherwise to kill ping we have to process WM_CHAR to return 0 on wp == 0xd and 0xa
            const auto is_keydown_or_up = lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_KEYUP;
            const auto is_return_or_tab = lpmsg->wParam == VK_RETURN || lpmsg->wParam == VK_TAB;

            if (!(is_keydown_or_up && is_return_or_tab))
                return DLGC_WANTMESSAGE;
        }
        break;
    }
    case WM_LBUTTONUP:
        m_clicked = false;
        break;
    case WM_LBUTTONDOWN:
        m_clicked = true;
        m_clickpoint = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        break;
    case WM_MOUSEMOVE: {
        const POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

        if (!(wp & MK_LBUTTON)) {
            m_dragging = false;
            m_clicked = false;
        }
        else if (!m_dragging && m_clicked && test_point_distance(m_clickpoint, pt, 5)) {
            TVHITTESTINFO ti{};
            ti.pt = m_clickpoint;
            TreeView_HitTest(wnd, &ti);

            if (!(ti.flags & TVHT_ONITEM) || !ti.hItem)
                break;

            TreeView_SelectItem(wnd, ti.hItem);

            if (!m_selection)
                break;

            static_api_ptr_t<playlist_incoming_item_filter> incoming_api;
            metadb_handle_list_t<pfc::alloc_fast_aggressive> items;

            if (cfg_add_items_use_core_sort) {
                incoming_api->filter_items(m_selection->get_entries(), items);
            }
            else {
                m_selection->sort_entries();
                items = m_selection->get_entries();
            }

            m_dragging = true;
            auto data_object = static_api_ptr_t<playlist_incoming_item_filter>()->create_dataobject_ex(items);
            if (data_object.is_valid()) {
                DWORD effect;
                auto colours = cui::colours::helper(g_guid_album_list_colours);
                const auto colour_selection_background = colours.get_colour(cui::colours::colour_selection_background);
                const auto colour_selection_text = colours.get_colour(cui::colours::colour_selection_text);
                const auto selection_count = items.get_count();
                pfc::string8 text;
                text << mmh::IntegerFormatter(selection_count) << (selection_count != 1 ? " tracks" : " track");
                SHDRAGIMAGE sdi = {0};
                LOGFONT lf = {0};
                GetObject(s_font, sizeof(lf), &lf);
                uih::create_drag_image(m_wnd_tv, true, m_dd_theme, colour_selection_background,
                                        colour_selection_text, nullptr, &lf, true, text, &sdi);
                uih::ole::do_drag_drop(m_wnd_tv, wp, data_object.get_ptr(), DROPEFFECT_COPY | DROPEFFECT_MOVE,
                                        DROPEFFECT_COPY, &effect, &sdi);
            }
            m_dragging = false;
            m_clicked = false;
        }
        break;
    }
    case WM_MBUTTONDOWN: {
        TVHITTESTINFO ti{};

        ti.pt.x = GET_X_LPARAM(lp);
        ti.pt.y = GET_Y_LPARAM(lp);
        TreeView_HitTest(wnd, &ti);

        if (ti.flags & TVHT_ONITEM && ti.hItem) {
            if (cfg_middle_click_action) {
                TreeView_SelectItem(wnd, ti.hItem);

                do_click_action(static_cast<ClickAction>(cfg_middle_click_action.get_value()));
            }
        }
        break;
    }
    case WM_LBUTTONDBLCLK:
        if (m_selection && m_selection->get_num_entries() > 0) {
            TVHITTESTINFO ti{};
            ti.pt.x = GET_X_LPARAM(lp);
            ti.pt.y = GET_Y_LPARAM(lp);

            TreeView_HitTest(wnd, &ti);

            if (ti.flags & TVHT_ONITEM) {
                const auto click_processed = do_click_action(static_cast<ClickAction>(cfg_double_click_action.get_value()));

                if (click_processed)
                    return 0;
            }
        }
        break;
    }
    return CallWindowProc(m_treeproc, wnd, msg, wp, lp);
}
