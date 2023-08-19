#include "stdafx.h"

#include "menu.h"
#include "node.h"
#include "tree_view_populator.h"
#include "actions.h"
#include "version.h"

// TODO: node name as field

DECLARE_COMPONENT_VERSION("Album list panel",

    album_list_panel::version,

    "allows you to browse through your media library\n\n"
    "based upon albumlist 3.1.0\n"
    "compiled: " COMPILATION_DATE "\n"
    "with Columns UI SDK version: " UI_EXTENSION_VERSION

)

const char* directory_structure_view_name = "by directory structure";

void album_list_window::s_update_all_fonts()
{
    if (s_font) {
        const auto count = s_instances.get_count();
        for (size_t i{0}; i < count; i++) {
            auto wnd = s_instances[i]->m_wnd_tv;
            if (wnd)
                uih::set_window_font(wnd, nullptr, false);

            wnd = s_instances[i]->m_wnd_edit;
            if (wnd)
                uih::set_window_font(wnd, nullptr, false);
        }
    }

    s_font.reset(cui::fonts::helper(album_list_font_client_id).get_font());

    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            uih::set_window_font(wnd, s_font.get());
            TreeView_SetIndent(wnd, cfg_use_custom_indentation ? cfg_custom_indentation_amount : 0);

            wnd = s_instances[i]->m_wnd_edit;
            if (wnd) {
                uih::set_window_font(wnd, s_font.get());
                s_instances[i]->on_size();
            }
        }
    }
}

album_list_window::~album_list_window()
{
    if (m_initialised) {
        s_instances.remove_item(this);
        m_initialised = false;
    }
}

void album_list_window::s_update_all_window_frames()
{
    long flags = 0;
    if (cfg_frame_style == 1)
        flags |= WS_EX_CLIENTEDGE;
    if (cfg_frame_style == 2)
        flags |= WS_EX_STATICEDGE;

    const auto count = s_instances.get_count();

    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            SetWindowLongPtr(wnd, GWL_EXSTYLE, flags);
            SetWindowPos(wnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
}

void album_list_window::s_update_all_tree_colours()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            s_instances[i]->update_tree_colours();
        }
    }
}

void album_list_window::s_update_all_tree_themes()
{
    for (auto&& window : s_instances) {
        window->update_tree_theme();
        window->update_tooltip_theme();
    }
}

void album_list_window::s_update_all_edit_themes()
{
    for (auto&& window : s_instances)
        window->update_edit_theme();
}

void album_list_window::s_update_all_edit_colours()
{
    s_filter_background_brush.reset();

    for (auto&& window : s_instances)
        window->update_edit_colours();
}

void album_list_window::s_update_all_labels()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            s_instances[i]->update_all_labels();
        }
    }
}

void album_list_window::s_update_all_showhscroll()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            uih::DisableRedrawScope disable_redrawing(s_instances[i]->get_wnd());
            s_instances[i]->recreate_tree(true);
        }
    }
}

void album_list_window::s_on_view_script_change(const char* p_view_before, const char* p_view)
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            s_instances[i]->on_view_script_change(p_view_before, p_view);
        }
    }
}

void album_list_window::s_refresh_all()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            s_instances[i]->refresh_tree();
        }
    }
}

void album_list_window::s_update_all_item_heights()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->m_wnd_tv;
        if (wnd) {
            s_instances[i]->update_item_height();
        }
    }
}

void album_list_window::s_update_all_indents()
{
    const auto count = s_instances.get_count();
    for (size_t i{0}; i < count; i++) {
        const auto wnd = s_instances[i]->get_wnd();
        if (wnd) {
            const auto indentation = cfg_use_custom_indentation ? cfg_custom_indentation_amount : 0;
            TreeView_SetIndent(s_instances[i]->m_wnd_tv, indentation);
        }
    }
}

bool album_list_window::is_bydir() const
{
    return !stricmp_utf8(m_view, directory_structure_view_name);
}

const char* album_list_window::get_hierarchy() const
{
    const auto index = cfg_views.find_item(m_view);
    if (index != pfc_infinite)
        return cfg_views.get_value(index);
    return "N/A";
}

void album_list_window::on_view_script_change(const char* p_view_before, const char* p_view)
{
    if (get_wnd()) {
        if (!stricmp_utf8(p_view_before, m_view)) {
            set_view(p_view);
        }
    }
}

void album_list_window::update_all_labels()
{
    if (m_root) {
        m_root->mark_all_labels_dirty();
        SendMessage(m_wnd_tv, WM_SETREDRAW, FALSE, 0);
        TreeViewPopulator::s_setup_tree(m_wnd_tv, TVI_ROOT, m_root, std::nullopt, 0, 0);
        SendMessage(m_wnd_tv, WM_SETREDRAW, TRUE, 0);
    }
}

void album_list_window::update_tree_theme(const cui::colours::helper& colours) const
{
    if (!m_wnd_tv)
        return;

    bool is_themed{};

    if (colours.get_bool(cui::colours::bool_dark_mode_enabled)) {
        SetWindowTheme(m_wnd_tv, L"DarkMode_Explorer", nullptr);
        is_themed = true;
    } else if (colours.get_themed()) {
        SetWindowTheme(m_wnd_tv, L"Explorer", nullptr);
        is_themed = true;
    } else {
        SetWindowTheme(m_wnd_tv, nullptr, nullptr);
    }

    auto styles = GetWindowLongPtr(m_wnd_tv, GWL_STYLE);

    if (is_themed)
        styles &= ~TVS_HASLINES;
    else
        styles |= TVS_HASLINES;

    SetWindowLongPtr(m_wnd_tv, GWL_STYLE, styles);
}

void album_list_window::update_tree_colours()
{
    SetWindowRedraw(m_wnd_tv, FALSE);
    cui::colours::helper p_colours(album_list_items_colours_client_id);
    update_tree_theme(p_colours);

    TreeView_SetBkColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_background));
    TreeView_SetLineColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_active_item_frame));
    TreeView_SetTextColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_text));
    RedrawWindow(m_wnd_tv, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
    SetWindowRedraw(m_wnd_tv, TRUE);
}

void album_list_window::update_tooltip_theme() const
{
    if (!m_wnd_tv)
        return;

    const HWND tooltip_wnd = TreeView_GetToolTips(m_wnd_tv);

    if (!tooltip_wnd)
        return;

    const auto is_dark = cui::colours::is_dark_mode_active();
    SetWindowTheme(tooltip_wnd, is_dark ? L"DarkMode_Explorer" : nullptr, nullptr);
}

void album_list_window::update_edit_theme() const
{
    if (!m_wnd_edit)
        return;

    SetWindowTheme(m_wnd_edit, cui::colours::is_dark_mode_active() ? L"DarkMode_CFD" : nullptr, nullptr);
}

void album_list_window::update_edit_colours() const
{
    if (!m_wnd_edit)
        return;

    RedrawWindow(m_wnd_edit, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE);
}

void album_list_window::update_item_height()
{
    const auto font = uih::get_window_font(m_wnd_tv);
    int font_height = -1;
    if (cfg_use_custom_vertical_item_padding) {
        font_height = uih::get_font_height(font) + cfg_custom_vertical_padding_amount;
        if (font_height < 1)
            font_height = 1;
    }
    TreeView_SetItemHeight(m_wnd_tv, font_height);
}

void album_list_window::on_task_completion(t_uint32 task, t_uint32 code)
{
    if (task == 0)
        refresh_tree();
}

void album_list_window::create_or_destroy_filter()
{
    if (m_filter)
        create_filter();
    else
        destroy_filter();
    on_size();
}

void album_list_window::create_filter()
{
    if (m_filter && !m_wnd_edit) {
        const auto flags = WS_EX_CLIENTEDGE;
        m_wnd_edit = CreateWindowEx(flags, WC_EDIT, _T(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0,
            0, 0, get_wnd(), HMENU(IDC_FILTER), core_api::get_my_instance(), nullptr);
        update_edit_theme();
        uih::set_window_font(m_wnd_edit, s_font.get(), false);
        SetFocus(m_wnd_edit);
        Edit_SetCueBannerText(m_wnd_edit, L"Search");
    }
}

void album_list_window::destroy_filter()
{
    if (m_wnd_edit) {
        const auto was_focused = GetFocus() == m_wnd_edit;
        DestroyWindow(m_wnd_edit);
        m_wnd_edit = nullptr;

        if (m_wnd_tv) {
            if (m_populated)
                refresh_tree();
            if (was_focused)
                SetFocus(m_wnd_tv);
        }
    }
    m_filter_ptr.release();
}

void album_list_window::on_size(unsigned cx, unsigned cy)
{
    HDWP dwp = BeginDeferWindowPos(2);
    const unsigned edit_height = m_wnd_edit ? uGetFontHeight(s_font.get()) + 4 : 0;
    const unsigned tv_height = edit_height < cy ? cy - edit_height : cy;

    dwp = DeferWindowPos(dwp, m_wnd_tv, nullptr, 0, 0, cx, tv_height, SWP_NOZORDER);
    if (m_wnd_edit)
        dwp = DeferWindowPos(dwp, m_wnd_edit, nullptr, 0, tv_height, cx, edit_height, SWP_NOZORDER);
    EndDeferWindowPos(dwp);
}

void album_list_window::on_size()
{
    RECT rc;
    GetClientRect(get_wnd(), &rc);
    on_size(rc.right, rc.bottom);
}

void album_list_window::create_tree()
{
    const auto wnd = get_wnd();

    auto ex_styles = 0l;
    if (cfg_frame_style == 1)
        ex_styles |= WS_EX_CLIENTEDGE;
    else if (cfg_frame_style == 2)
        ex_styles |= WS_EX_STATICEDGE;

    auto styles = TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | WS_CHILD | WS_VSCROLL
        | WS_VISIBLE | WS_TABSTOP;

    if (!cfg_show_horizontal_scroll_bar)
        styles |= TVS_NOHSCROLL;

    m_wnd_tv = CreateWindowEx(ex_styles, WC_TREEVIEW, L"Album list", styles, 0, 0, 0, 0, wnd,
        reinterpret_cast<HMENU>(IDC_TREE), core_api::get_my_instance(), nullptr);

    if (m_wnd_tv) {
        TreeView_SetExtendedStyle(m_wnd_tv, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
        update_tree_theme();
        update_tooltip_theme();

        if (s_font) {
            uih::set_window_font(m_wnd_tv, s_font.get(), false);
            TreeView_SetIndent(m_wnd_tv, cfg_use_custom_indentation ? cfg_custom_indentation_amount : 0);
        } else {
            s_update_all_fonts();
        }

        if (cfg_use_custom_vertical_item_padding)
            update_item_height();

        update_tree_colours();

        SetWindowLongPtr(m_wnd_tv, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
        m_treeproc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(m_wnd_tv, GWLP_WNDPROC, reinterpret_cast<LPARAM>(s_tree_hook_proc)));

        if (m_populated) {
            refresh_tree();
        }
    }
}

void album_list_window::destroy_tree(bool should_save_scroll_position)
{
    if (m_wnd_tv) {
        if (should_save_scroll_position)
            save_scroll_position();

        DestroyWindow(m_wnd_tv);
        m_wnd_tv = nullptr;
    }
}

void album_list_window::recreate_tree(bool save_state)
{
    if (!m_wnd_tv)
        return;

    if (m_root && save_state)
        m_node_state = m_root->get_state(m_selection);

    SetWindowRedraw(get_wnd(), FALSE);

    destroy_tree(save_state);
    create_tree();
    on_size();

    SetWindowRedraw(get_wnd(), TRUE);
    RedrawWindow(get_wnd(), nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void album_list_window::save_scroll_position() const
{
    if (!m_wnd_tv)
        return;

    m_saved_scroll_position.reset();

    SCROLLINFO si_horz{};
    si_horz.cbSize = sizeof(SCROLLINFO);
    si_horz.fMask = SIF_POS;

    SCROLLINFO si_vert{};
    si_vert.cbSize = sizeof(SCROLLINFO);
    si_vert.fMask = SIF_POS | SIF_RANGE;

    if (!GetScrollInfo(m_wnd_tv, SB_HORZ, &si_horz) || !GetScrollInfo(m_wnd_tv, SB_VERT, &si_vert))
        return;

    m_saved_scroll_position = {si_horz.nPos, si_vert.nPos, si_vert.nMax};
}

void album_list_window::restore_scroll_position()
{
    if (!m_wnd_tv || !m_saved_scroll_position)
        return;

    const auto is_initialised = !m_library_v4.is_valid() || m_library_v4->is_initialized();

    // Ensure scroll bar state has been updated; apparently the tree
    // view does this when rendering
    if (is_initialised)
        UpdateWindow(m_wnd_tv);

    SCROLLINFO si_vert_current{};
    si_vert_current.cbSize = sizeof(SCROLLINFO);
    si_vert_current.fMask = SIF_RANGE | SIF_POS;

    if (!GetScrollInfo(m_wnd_tv, SB_VERT, &si_vert_current))
        return;

    SCROLLINFO si{};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    si.nPos = m_saved_scroll_position->horizontal_position;
    const auto new_horizontal_position = SetScrollInfo(m_wnd_tv, SB_HORZ, &si, is_initialised);

    si.nPos = MulDiv(
        si_vert_current.nMax, m_saved_scroll_position->vertical_position, m_saved_scroll_position->vertical_max);
    const auto new_vertical_position = SetScrollInfo(m_wnd_tv, SB_VERT, &si, is_initialised);

    SendMessage(m_wnd_tv, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, new_horizontal_position), 0);
    SendMessage(m_wnd_tv, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, new_vertical_position), 0);

    if (is_initialised) {
        m_saved_scroll_position.reset();
    }
}

void album_list_window::get_config(stream_writer* p_writer, abort_callback& p_abort) const
{
    save_scroll_position();

    p_writer->write_string(m_view, p_abort);
    p_writer->write_lendian_t(m_filter, p_abort);
    const auto saved_scroll_position = m_saved_scroll_position.value_or(alp::SavedScrollPosition{});
    p_writer->write_lendian_t(saved_scroll_position.horizontal_position, p_abort);
    p_writer->write_lendian_t(saved_scroll_position.vertical_position, p_abort);
    p_writer->write_lendian_t(saved_scroll_position.vertical_max, p_abort);

    if (get_wnd() && m_root) {
        write_node_state(p_writer, m_root->get_state(m_selection), p_abort);
    } else if (m_node_state) {
        write_node_state(p_writer, *m_node_state, p_abort);
    }
}

void album_list_window::get_name(pfc::string_base& out) const
{
    out.set_string("Album list");
}

void album_list_window::get_category(pfc::string_base& out) const
{
    out.set_string("Panels");
}

void album_list_window::set_config(stream_reader* p_reader, t_size psize, abort_callback& p_abort)
{
    if (psize) {
        p_reader->read_string(m_view, p_abort);
        try {
            p_reader->read_lendian_t(m_filter, p_abort);
            const auto horizontal_scroll_position = p_reader->read_lendian_t<int32_t>(p_abort);
            const auto vertical_scroll_position = p_reader->read_lendian_t<int32_t>(p_abort);
            const auto vertical_scroll_max = p_reader->read_lendian_t<int32_t>(p_abort);
            m_node_state = alp::read_node_state(p_reader, p_abort);

            // Only set scroll positions if expansion state was read
            m_saved_scroll_position
                = alp::SavedScrollPosition{horizontal_scroll_position, vertical_scroll_position, vertical_scroll_max};
        } catch (exception_io_data_truncation&) {
        }
    }
}

void album_list_window::toggle_show_filter()
{
    m_filter = !m_filter;
    create_or_destroy_filter();
}

const char* album_list_window::get_view() const
{
    return m_view;
}

void album_list_window::set_view(const char* view)
{
    m_view = view;
    recreate_tree(false);
}

void album_list_window::get_menu_items(ui_extension::menu_hook_t& p_hook)
{
    const auto node_settings
        = uie::menu_node_ptr{new uie::simple_command_menu_node{"Settings", "Shows Album List panel settings", 0,
            [] { static_api_ptr_t<ui_control>()->show_preferences(album_list_panel_preferences_page_id); }}};
    const auto node_filter = uie::menu_node_ptr{new uie::simple_command_menu_node{"Filter", "Shows the filter bar",
        m_filter ? uie::menu_node_t::state_checked : 0,
        [instance = service_ptr_t<album_list_window>{this}] { instance->toggle_show_filter(); }}};

    p_hook.add_node(ui_extension::menu_node_ptr(new menu_node_select_view(this)));
    p_hook.add_node(node_filter);
    p_hook.add_node(node_settings);
}

bool album_list_window::do_click_action(ClickAction click_action)
{
    switch (click_action) {
    case ClickAction::send_to_playlist:
        do_playlist(m_selection, true);
        break;
    case ClickAction::add_to_playlist:
        do_playlist(m_selection, false);
        break;
    case ClickAction::send_to_new_playlist:
        do_playlist(m_selection, true, true);
        break;
    case ClickAction::send_to_autosend_playlist:
        do_autosend_playlist(m_selection, m_view, true);
        break;
    default:
        return false;
    }
    return true;
}

void album_list_window::collapse_other_nodes(const node_ptr& node) const
{
    auto current = node;
    auto parent = current->get_parent().lock();

    while (parent) {
        auto expanded_siblings = parent->get_children()
            | ranges::views::filter([&current](auto& sibling) { return sibling->is_expanded() && sibling != current; });

        for (const auto& sibling : expanded_siblings) {
            TreeView_Expand(m_wnd_tv, sibling->m_ti, TVE_COLLAPSE);
            sibling->set_expanded(false);
        }

        current = parent;
        parent = current->get_parent().lock();
    }
}

// {606E9CDD-45EE-4c3b-9FD5-49381CEBE8AE}
const GUID album_list_window::s_extension_guid
    = {0x606e9cdd, 0x45ee, 0x4c3b, {0x9f, 0xd5, 0x49, 0x38, 0x1c, 0xeb, 0xe8, 0xae}};

ui_extension::window_factory<album_list_window> blah;
