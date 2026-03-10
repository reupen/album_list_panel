#pragma once

#include "prefs.h"

namespace alp {

class TabViews : public PreferencesTab {
public:
    bool is_active() const { return m_wnd != nullptr; }
    void refresh_views();

    HWND create(HWND parent_window) override
    {
        const auto [wnd, _] = fbh::auto_dark_modeless_dialog_box(IDD_VIEWS_TAB, parent_window,
            [this](auto&&... args) { return handle_message(std::forward<decltype(args)>(args)...); });
        return wnd;
    }
    const char* get_name() override { return "Views"; }

private:
    class UIConfigCallback : public ui_config_callback {
    public:
        explicit UIConfigCallback(uih::ListView* list_view) : m_list_view(list_view)
        {
            if (m_manager.is_valid())
                m_manager->add_callback(this);
        }

        ~UIConfigCallback()
        {
            if (m_manager.is_valid())
                m_manager->remove_callback(this);
        }

        void ui_fonts_changed() override {}
        void ui_colors_changed() override { m_list_view->set_use_dark_mode(m_manager->is_dark_mode()); }

    private:
        uih::ListView* m_list_view{};
        ui_config_manager::ptr m_manager{ui_config_manager::tryGet()};
    };

    class ViewsListView : public uih::ListView {
        void notify_update_item_data(size_t index) override;
        void render_get_colour_data(ColourData& p_out) override;
        void execute_default_action(size_t index, size_t column, bool b_keyboard, bool b_ctrl) override;
    };

    INT_PTR CALLBACK handle_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    HWND m_wnd{};
    ViewsListView m_list_view;
    std::unique_ptr<UIConfigCallback> m_ui_config_callback;
};

extern TabViews g_prefs_tab_views;

} // namespace alp
