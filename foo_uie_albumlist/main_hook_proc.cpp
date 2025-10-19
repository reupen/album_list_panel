#include "stdafx.h"

#include "node.h"
#include "node_utils.h"

LRESULT WINAPI AlbumListWindow::s_tree_hook_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto p_this = reinterpret_cast<AlbumListWindow*>(GetWindowLongPtr(wnd, GWLP_USERDATA));

    return p_this ? p_this->on_tree_hooked_message(wnd, msg, wp, lp) : DefWindowProc(wnd, msg, wp, lp);
}

static bool test_point_distance(POINT pt1, POINT pt2, int test)
{
    const int dx = pt1.x - pt2.x;
    const int dy = pt1.y - pt2.y;
    return dx * dx + dy * dy > test * test;
}

LRESULT WINAPI AlbumListWindow::on_tree_hooked_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
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
        case VK_SHIFT:
            if (!(HIWORD(lp) & KF_REPEAT))
                update_shift_start_node();
            break;
        case VK_HOME:
        case VK_DOWN:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_UP: {
            const auto is_shift_down = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            const auto is_ctrl_down = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            auto shift_node = m_shift_start.lock();
            const auto old_caret_item = TreeView_GetSelection(wnd);
            const auto old_caret_node = get_node_for_tree_item(old_caret_item);

            if (!is_shift_down || !shift_node) {
                // If we're at the start or the end of the list, pressing Up or Down
                // respectively won't move the caret, but any other selected items
                // should still be deselected
                if (!is_ctrl_down && m_selection != std::unordered_set{old_caret_node}) {
                    CallWindowProc(m_treeproc, wnd, msg, wp, lp);

                    const auto new_caret_item = TreeView_GetSelection(wnd);

                    if (old_caret_item != new_caret_item)
                        return 0;

                    deselect_selected_nodes(old_caret_node);

                    if (!m_selection.contains(old_caret_node))
                        manually_select_tree_item(old_caret_item, true);

                    autosend();
                    update_selection_holder();
                    return 0;
                }
                break;
            }

            {
                pfc::vartoggle_t _(m_processing_multiselect, true);

                // Hack: By default, the tree view will scroll when Ctrl is down
                std::array<BYTE, 256> old_key_state;
                bool should_restore_key_state{};
                if (is_ctrl_down && GetKeyboardState(old_key_state.data())) {
                    std::array temp_key_state{old_key_state};
                    temp_key_state[VK_CONTROL] = 0;
                    SetKeyboardState(temp_key_state.data());
                    should_restore_key_state = true;
                }

                CallWindowProc(m_treeproc, wnd, msg, wp, lp);

                if (should_restore_key_state) {
                    SetKeyboardState(old_key_state.data());
                }
            }

            const auto caret_item = TreeView_GetSelection(wnd);

            if (old_caret_item == caret_item)
                return 0;

            const auto caret_node = get_node_for_tree_item(caret_item);

            select_range(shift_node, caret_node, is_ctrl_down);
            autosend();
            update_selection_holder();
            return 0;
        }
        }
        break;
    }
    case WM_SYSKEYUP:
    case WM_KEYUP:
        if (wp == VK_SHIFT)
            update_shift_start_node();
        break;
    case WM_CHAR:
        if (cfg_process_keyboard_shortcuts && !m_process_char) {
            m_process_char = true;
            return 0;
        }

        switch (wp) {
        // Ctrl+A
        case 1: {
            if ((HIWORD(lp) & KF_REPEAT) || (GetKeyState(VK_CONTROL) & 0x8000) == 0)
                return 0;

            auto tree_item = TreeView_GetRoot(wnd);
            bool any_selected{};

            do {
                if (auto node = get_node_for_tree_item(tree_item)) {
                    if (!m_selection.contains(node)) {
                        manually_select_tree_item(tree_item, true);
                        any_selected = true;
                    }
                }
            } while ((tree_item = TreeView_GetNextVisible(m_wnd_tv, tree_item)));

            if (any_selected) {
                autosend();
                update_selection_holder();
            }
            return 0;
        }
        }
        break;
    case WM_SETFOCUS: {
        update_shift_start_node();

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
    case WM_CAPTURECHANGED:
        m_delayed_click_node.reset();
        break;
    case WM_LBUTTONUP:
        if (const auto result = on_tree_lbuttonup(wnd, msg, wp, lp))
            return *result;
        break;
    case WM_LBUTTONDOWN:
        if (const auto result = on_tree_lbuttondown(wnd, msg, wp, lp))
            return *result;
        break;
    case WM_RBUTTONDOWN:
        on_tree_rbuttondown(wnd, msg, wp, lp);

        // The tree view uses a modal loop to process clicks. So supress the default handling
        // so we get a WM_RBUTTONUP message.
        return 0;
    case WM_RBUTTONUP:
        if (const auto result = on_tree_rbuttonup(wnd, msg, wp, lp))
            return *result;
        break;
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

            auto drag_node = get_node_for_tree_item(ti.hItem);

            if (!drag_node)
                break;

            if (!m_selection.contains(drag_node))
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
                text << mmh::format_integer(selection_count).c_str() << (selection_count != 1 ? " tracks" : " track");
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

        if (!(ti.flags & TVHT_ONITEM && ti.hItem && cfg_middle_click_action))
            break;

        if (TreeView_GetSelection(wnd) != ti.hItem) {
            TreeView_SelectItem(wnd, ti.hItem);
        } else {
            const auto node = get_node_for_tree_item(ti.hItem);
            deselect_selected_nodes(node);

            if (!m_selection.contains(node))
                manually_select_tree_item(ti.hItem, true);
        }

        do_click_action(static_cast<ClickAction>(cfg_middle_click_action.get_value()));
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
        break;
    }
    }
    return CallWindowProc(m_treeproc, wnd, msg, wp, lp);
}

std::optional<LRESULT> AlbumListWindow::on_tree_lbuttondown(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    m_clicked = true;
    m_clickpoint = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

    const auto is_shift_down = (wp & MK_SHIFT) != 0;
    const auto is_ctrl_down = (wp & MK_CONTROL) != 0;

    TVHITTESTINFO tvhti{};
    tvhti.pt = m_clickpoint;
    TreeView_HitTest(wnd, &tvhti);

    if (!(tvhti.flags & TVHT_ONITEM))
        return {};

    const auto click_item = tvhti.hItem;
    const auto click_node = get_node_for_tree_item(tvhti.hItem);

    if (!click_node)
        return {};

    if (is_shift_down || is_ctrl_down)
        SetFocus(wnd);

    if (is_shift_down) {
        const auto shift_node = m_shift_start.lock();
        const auto old_selection = m_selection;

        if (!shift_node)
            return 0;

        select_range(shift_node, click_node, is_ctrl_down);

        const auto caret = TreeView_GetSelection(wnd);

        if (caret != click_item) {
            pfc::vartoggle_t _(m_processing_multiselect, true);
            CallWindowProc(m_treeproc, wnd, msg, wp, lp);
        } else {
            manually_select_tree_item(click_item, true);
        }

        if (old_selection != m_selection) {
            autosend();
            update_selection_holder();
        }

        return 0;
    }

    if (is_ctrl_down) {
        const auto currently_selected = m_selection.contains(click_node);
        const auto caret = TreeView_GetSelection(wnd);

        if (caret != click_item) {
            pfc::vartoggle_t _(m_processing_multiselect, true);
            CallWindowProc(m_treeproc, wnd, msg, wp, lp);
        }

        if (!manually_select_tree_item(tvhti.hItem, !currently_selected))
            return 0;

        autosend();
        update_selection_holder();
        return 0;
    }

    // If the caret is on the clicked item, the tree view won't do anything itself.
    if (click_item == TreeView_GetSelection(wnd) && m_selection != std::unordered_set{click_node}) {
        SetCapture(wnd);
        m_delayed_click_node = click_node;

        if (!m_selection.contains(click_node)) {
            deselect_selected_nodes(click_node);
            manually_select_tree_item(click_node->m_ti, true);
        }

        // Tree view uses a modal loop to process clicks. Don't pass the message on
        // so we receive mouse up.
        return 0;
    }

    return {};
}

std::optional<LRESULT> AlbumListWindow::on_tree_lbuttonup(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    m_clicked = false;

    auto _ = gsl::finally([wnd] {
        if (GetCapture() == wnd)
            ReleaseCapture();
    });

    if (const auto click_node = m_delayed_click_node.lock()) {
        deselect_selected_nodes(click_node);

        autosend();
        update_selection_holder();
        return 0;
    }

    return {};
}

void AlbumListWindow::on_tree_rbuttondown(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    SetFocus(wnd);

    TVHITTESTINFO tvhti{};
    tvhti.pt = POINT{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    TreeView_HitTest(wnd, &tvhti);

    if (!(tvhti.hItem && tvhti.flags & TVHT_ONITEM))
        return;

    TVITEMEX tvi{};
    tvi.hItem = tvhti.hItem;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
    TreeView_GetItem(m_wnd_tv, &tvi);

    if (!tvi.lParam)
        return;

    const auto click_node = reinterpret_cast<Node*>(tvi.lParam)->shared_from_this();

    if (!click_node || m_selection.contains(click_node))
        return;

    SetCapture(wnd);

    m_previous_selection.emplace();
    ranges::copy(m_selection, std::back_inserter(*m_previous_selection));

    deselect_selected_nodes();
    manually_select_tree_item(tvhti.hItem, true);
}

std::optional<LRESULT> AlbumListWindow::on_tree_rbuttonup(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (GetCapture() == wnd)
        ReleaseCapture();

    const auto result = CallWindowProc(m_treeproc, wnd, msg, wp, lp);

    if (m_previous_selection) {
        deselect_selected_nodes();

        for (const auto& node : *m_previous_selection)
            if (auto locked_node = node.lock())
                manually_select_tree_item(locked_node->m_ti, true);
    }

    m_previous_selection.reset();

    return result;
}
