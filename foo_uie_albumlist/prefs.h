#pragma once

class preferences_tab {
public:
    virtual HWND create(HWND wnd) = 0;
    virtual const char* get_name() = 0;
};

class tab_general : public preferences_tab {
    bool m_initialised{false};
    HWND m_wnd{nullptr};
public:
    bool is_active()
    {
        return m_wnd != nullptr;
    }
    void refresh_views();
    static BOOL CALLBACK g_on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
    BOOL CALLBACK on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND create(HWND wnd) override
    {
        return CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_CONFIG), wnd, g_on_message, (LPARAM)this);
    }
    const char* get_name() override
    {
        return "General";
    }

    tab_general() {}
};

class tab_advanced : public preferences_tab {
    static bool initialised;

    static BOOL CALLBACK ConfigProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
public:
    HWND create(HWND wnd) override
    {
        return CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_ADVANCED), wnd, ConfigProc, NULL);
    }
    const char* get_name() override
    {
        return "Advanced";
    }

};

class config_albumlist : public preferences_page {

    static HWND child;

    static void make_child(HWND wnd);

    static BOOL CALLBACK ConfigProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
public:
    HWND create(HWND parent) override
    {
        return CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_HOST), parent, ConfigProc, NULL);
    }

    const char* get_name() override
    {
        return "Album List Panel";
    }

    GUID get_guid() override { return album_list_panel_preferences_page_id; }

    GUID get_parent_guid() override
    {
        return guid_media_library;
    }

    bool reset_query() override
    {
        return false;
    }

    void reset() override { }
};

extern tab_general g_config_general;
