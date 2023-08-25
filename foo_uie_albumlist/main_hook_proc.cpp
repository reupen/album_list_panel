#include "stdafx.h"

#include "node.h"
#include "node_utils.h"

LRESULT WINAPI album_list_window::s_tree_hook_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto p_this = reinterpret_cast<album_list_window*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

    return p_this ? p_this->on_tree_hooked_message(wnd, msg, wp, lp) : DefWindowProc(wnd, msg, wp, lp);
}

static bool test_point_distance(POINT pt1, POINT pt2, int test)
{
    const int dx = pt1.x - pt2.x;
    const int dy = pt1.y - pt2.y;
    return dx * dx + dy * dy > test * test;
}

LRESULT WINAPI album_list_window::on_tree_hooked_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        uie::window_ptr p_this{this};

        if (get_host()->get_keyboard_shortcuts_enabled() && wp != VK_LEFT && wp != VK_RIGHT
            && cfg_process_keyboard_shortcuts && g_process_keydown_keyboard_shortcuts(wp)) {
            m_process_char = false;
            return 0;
        }

        m_process_char = true;

        switch (wp) {
        case VK_TAB:
            if (msg == WM_KEYDOWN)
                g_on_tab(wnd);
            break;
        case VK_HOME:
        case VK_DOWN:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_UP: {
            deselect_selected_nodes();
            break;
        }
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
        update_selection_holder();
        RedrawWindow(wnd, nullptr, nullptr, RDW_INVALIDATE);
        break;
    }
    case WM_KILLFOCUS: {
        m_selection_holder.release();
        RedrawWindow(wnd, nullptr, nullptr, RDW_INVALIDATE);
        break;
    }
    case WM_GETDLGCODE: {
        const auto lpmsg = reinterpret_cast<LPMSG>(lp);
        if (lpmsg && cfg_process_keyboard_shortcuts) {
            // let dialog manager handle it, otherwise to kill ping we have to process WM_CHAR to return 0 on wp == 0xd
            // and 0xa
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
    case WM_LBUTTONDOWN: {
        m_clicked = true;
        m_clickpoint = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

        TVHITTESTINFO tvhti{};
        tvhti.pt = m_clickpoint;
        TreeView_HitTest(wnd, &tvhti);

        if (!(tvhti.flags & TVHT_ONITEM))
            break;

        TVITEMEX tvi{};
        tvi.hItem = tvhti.hItem;
        tvi.mask = TVIF_PARAM;
        if (!TreeView_GetItem(wnd, &tvi))
            break;

        auto click_node = reinterpret_cast<node*>(tvi.lParam)->shared_from_this();

        if (wp & MK_CONTROL) {
            auto currently_selected = m_selection.contains(click_node);

            if (!manually_select_tree_item(tvhti.hItem, !currently_selected))
                return 0;

            m_cleaned_selection.reset();

            if (currently_selected) {
                m_selection.erase(click_node);
            } else {
                m_selection.emplace(click_node);
            }

            autosend();
            update_selection_holder();

            return 0;
        }

        deselect_selected_nodes(click_node);
        break;
    }
    case WM_MOUSEMOVE: {
        const POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

        if (!(wp & MK_LBUTTON)) {
            m_dragging = false;
            m_clicked = false;
        } else if (!m_dragging && m_clicked && test_point_distance(m_clickpoint, pt, 5)) {
            TVHITTESTINFO ti{};
            ti.pt = m_clickpoint;
            TreeView_HitTest(wnd, &ti);

            if (!(ti.flags & TVHT_ONITEM) || !ti.hItem)
                break;

            TreeView_SelectItem(wnd, ti.hItem);

            if (m_selection.empty())
                break;

            static_api_ptr_t<playlist_incoming_item_filter> incoming_api;
            auto tracks_holder = alp::get_node_tracks(get_cleaned_selection());

            m_dragging = true;
            auto data_object
                = static_api_ptr_t<playlist_incoming_item_filter>()->create_dataobject_ex(tracks_holder.tracks());
            if (data_object.is_valid()) {
                DWORD effect;
                auto colours = cui::colours::helper(album_list_items_colours_client_id);
                const auto colour_selection_background = colours.get_colour(cui::colours::colour_selection_background);
                const auto colour_selection_text = colours.get_colour(cui::colours::colour_selection_text);
                const auto selection_count = tracks_holder.tracks().get_count();
                pfc::string8 text;
                text << mmh::IntegerFormatter(selection_count) << (selection_count != 1 ? " tracks" : " track");
                SHDRAGIMAGE sdi = {0};
                LOGFONT lf = {0};
                GetObject(s_font.get(), sizeof(lf), &lf);
                uih::create_drag_image(m_wnd_tv, true, m_dd_theme, colour_selection_background, colour_selection_text,
                    nullptr, &lf, true, text, &sdi);
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
    case WM_LBUTTONDBLCLK: {
        TVHITTESTINFO ti{};
        ti.pt.x = GET_X_LPARAM(lp);
        ti.pt.y = GET_Y_LPARAM(lp);

        TreeView_HitTest(wnd, &ti);

        if (ti.flags & TVHT_ONITEM) {
            const auto click_processed = do_click_action(static_cast<ClickAction>(cfg_double_click_action.get_value()));

            if (click_processed)
                return 0;
        }
    } break;
    }
    return CallWindowProc(m_treeproc, wnd, msg, wp, lp);
}
