#pragma once

namespace alp {

class PreferencesTab {
public:
    virtual HWND create(HWND wnd) = 0;
    virtual const char* get_name() = 0;
};

class PreferencesPageInstance : public preferences_page_instance {
public:
    explicit PreferencesPageInstance(HWND parent)
    {
        const auto [_, has_dark_mode] = fbh::auto_dark_modeless_dialog_box(
            IDD_HOST, parent, [this](auto&&... args) { return handle_message(std::forward<decltype(args)>(args)...); });

        m_has_dark_mode = has_dark_mode;
    }

    t_uint32 get_state() override { return m_has_dark_mode ? preferences_state::dark_mode_supported : 0; }
    fb2k::hwnd_t get_wnd() override { return m_wnd; }
    void apply() override {}
    void reset() override {}

private:
    void make_child(HWND wnd);
    INT_PTR CALLBACK handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_wnd{};
    HWND m_child{};
    bool m_has_dark_mode{};
};

class PreferencesPage : public preferences_page_v3 {
public:
    const char* get_name() override { return "Album list panel"; }

    GUID get_guid() override { return album_list_panel_preferences_page_id; }

    GUID get_parent_guid() override { return guid_media_library; }
    preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) override
    {
        return fb2k::service_new<PreferencesPageInstance>(parent);
    }
};

} // namespace alp
