#include "stdafx.h"

#include "prefs_views.h"

namespace alp {

namespace {

struct EditViewState {
    std::optional<size_t> index;
    pfc::string8 name;
    pfc::string8 title_format;
};

INT_PTR CALLBACK EditViewProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp, EditViewState& state)
{
    switch (msg) {
    case WM_INITDIALOG: {
        if (!state.index)
            SetWindowText(wnd, L"New view");

        const auto name_edit_wnd = GetDlgItem(wnd, IDC_NAME);
        uSetWindowText(name_edit_wnd, state.name);
        uih::enhance_edit_control(name_edit_wnd);

        const auto value_edit_wnd = GetDlgItem(wnd, IDC_TITLE_FORMAT);
        uSetWindowText(value_edit_wnd, state.title_format);
        uih::enhance_edit_control(value_edit_wnd);
        break;
    }
    case WM_COMMAND:
        switch (wp) {
        case IDCANCEL:
            EndDialog(wnd, 0);
            break;
        case IDOK: {
            pfc::string8 name;
            uGetDlgItemText(wnd, IDC_NAME, name);

            if (name.is_empty()) {
                MessageBox(wnd, L"Enter a name.", nullptr, 0);
                break;
            }

            const auto found_index = get_views().find_item(name);
            const auto was_found = found_index != std::numeric_limits<size_t>::max();

            if (was_found && (!state.index || state.index != found_index)) {
                MessageBox(wnd, L"A view with this name already exists. Enter a different name.", nullptr, 0);
                break;
            }

            state.name = name;

            uGetDlgItemText(wnd, IDC_TITLE_FORMAT, state.title_format);
            EndDialog(wnd, 1);

            break;
        }
        }
        break;
    }
    return FALSE;
}

bool open_edit_view_dialog(EditViewState& param, HWND parent)
{
    return fbh::auto_dark_modal_dialog_box(IDD_EDIT_VIEW, parent, [&param](auto&&... args) {
        return EditViewProc(std::forward<decltype(args)>(args)..., param);
    }) != 0;
}

} // namespace

void TabViews::refresh_views()
{
    m_list_view.remove_all_items();

    const std::vector<uih::ListView::InsertItem> insert_items{get_views().get_count()};
    m_list_view.insert_items(0, insert_items.size(), insert_items.data());
}

void TabViews::ViewsListView::notify_update_item_data(size_t index)
{
    auto& sub_items = get_item_subitems(index);
    sub_items.resize(2);

    const auto& views = get_views();
    sub_items[0] = views.get_name(index);
    sub_items[1] = views.get_title_format(index);
}

void TabViews::ViewsListView::render_get_colour_data(ColourData& p_out)
{
    if (!ui_config_manager::g_is_dark_mode()) {
        ListView::render_get_colour_data(p_out);
        return;
    }

    p_out.m_themed = true;
    p_out.m_use_custom_active_item_frame = false;
    p_out.m_text = RGB(255, 255, 255);
    p_out.m_selection_text = RGB(255, 255, 255);
    p_out.m_background = mmh::check_windows_10_build(22'000) ? RGB(25, 25, 25) : RGB(32, 32, 32);
    p_out.m_selection_background = RGB(98, 98, 98);
    p_out.m_inactive_selection_text = RGB(255, 255, 255);
    p_out.m_inactive_selection_background = RGB(51, 51, 51);
    p_out.m_active_item_frame = RGB(119, 119, 119);
    p_out.m_group_text = get_group_text_colour_default();
    p_out.m_group_background = p_out.m_background;
}

void TabViews::ViewsListView::execute_default_action(size_t index, size_t column, bool b_keyboard, bool b_ctrl)
{
    auto& views = get_views();

    assert(index < views.get_count());

    EditViewState state;
    state.index = index;
    state.name = views.get_name(index);
    state.title_format = views.get_title_format(index);

    const auto old_name = state.name;

    if (!open_edit_view_dialog(state, GetAncestor(get_wnd(), GA_ROOT)))
        return;

    // Ensure the index is still in range following the modal message loop
    if (index < views.get_count()) {
        views.modify_item(index, state.name, state.title_format);
        update_items(index, 1);
        AlbumListWindow::s_on_view_script_change(old_name, state.name);
    }
}

INT_PTR TabViews::handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_INITDIALOG: {
        m_wnd = wnd;

        m_list_view.set_selection_mode(uih::ListView::SelectionMode::SingleRelaxed);
        m_list_view.set_columns(
            {uih::ListView::Column{"Name", 100_spx}, uih::ListView::Column{"Title format", 300_spx}});
        m_list_view.set_use_dark_mode(ui_config_manager::g_is_dark_mode());
        m_ui_config_callback = std::make_unique<UIConfigCallback>(&m_list_view);

        m_list_view.create(wnd, {7, 7, 314, 237}, true);

        refresh_views();

        ShowWindow(m_list_view.get_wnd(), SW_SHOWNORMAL);

        break;
    }
    case WM_DESTROY:
        m_ui_config_callback.reset();
        m_wnd = nullptr;
        break;
    case WM_COMMAND:
        switch (wp) {
        case IDC_VIEW_UP: {
            const auto index = m_list_view.get_selected_item_single();

            if (index > 0 && index != std::numeric_limits<size_t>::max()) {
                auto& views = get_views();
                views.swap(index, index - 1);
                m_list_view.update_items(index - 1, 2);
                m_list_view.set_item_selected_single(index - 1);
            }
            break;
        }
        case IDC_VIEW_DOWN: {
            const auto index = m_list_view.get_selected_item_single();
            auto& views = get_views();

            if (index != std::numeric_limits<size_t>::max() && index + 1 < views.get_count()) {
                views.swap(index, index + 1);
                m_list_view.update_items(index, 2);
                m_list_view.set_item_selected_single(index + 1);
            }
            break;
        }
        case IDC_VIEW_DELETE: {
            const auto index = m_list_view.get_selected_item_single();

            if (index != std::numeric_limits<size_t>::max()) {
                auto& views = get_views();
                const std::string name = views.get_name(index);
                views.remove_item(index);
                m_list_view.remove_item(index);
                AlbumListWindow::s_on_view_script_change(name.c_str(), name.c_str());
            }
            break;
        }
        case IDC_VIEW_NEW: {
            EditViewState state;

            if (open_edit_view_dialog(state, wnd)) {
                auto& views = get_views();
                views.add_item(state.name, state.title_format);
                uih::ListView::InsertItem insert_item;
                m_list_view.insert_items(m_list_view.get_item_count(), 1, &insert_item);
            }
            break;
        }
        case IDC_VIEW_RESET:
            get_views().reset();
            refresh_views();
            AlbumListWindow::s_refresh_all();
            break;
        }
        break;
    }
    return 0;
}

TabViews g_prefs_tab_views;

} // namespace alp
