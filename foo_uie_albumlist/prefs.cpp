#include "stdafx.h"

#include "prefs_appearance.h"
#include "prefs_behaviour.h"
#include "prefs_views.h"

namespace alp {

cfg_int cfg_child(GUID{0x637c25b6, 0x9166, 0xd8df, 0xae, 0x7a, 0x39, 0x75, 0x78, 0x08, 0xfa, 0xf0}, 0);

static PreferencesTab* g_tabs[] = {
    &g_prefs_tab_views,
    get_appearance_prefs_tab(),
    get_behaviour_prefs_tab(),
};

namespace {

preferences_page_factory_t<PreferencesPage> _preferences_page;

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

INT_PTR PreferencesPageInstance::handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
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

} // namespace alp
