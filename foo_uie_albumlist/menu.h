#pragma once

class MenuNodeSelectView : public ui_extension::menu_node_popup_t {
    pfc::list_t<ui_extension::menu_node_ptr> m_items;
    service_ptr_t<AlbumListWindow> m_window;

public:
    bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags) const override
    {
        p_out = "View";
        p_displayflags = 0;
        return true;
    }

    size_t get_children_count() const override { return m_items.get_count(); }

    void get_child(size_t p_index, uie::menu_node_ptr& p_out) const override { p_out = m_items[p_index].get_ptr(); }

    MenuNodeSelectView(AlbumListWindow* p_wnd) : m_window{p_wnd}
    {
        const auto& views = get_views();

        const auto view_count = views.get_count();
        add_item(directory_structure_view_name);

        for (size_t i = 0; i < view_count; i++) {
            add_item(views.get_name(i));
        }
    }

private:
    void add_item(const char* view_name)
    {
        pfc::string8 description;
        description << "Switches to the " << view_name << " view.";
        const uint32_t display_flags = !stricmp_utf8(view_name, m_window->get_view()) ? state_checked : 0;
        const auto set_view = [this, view_name = std::string{view_name}] { m_window->set_view(view_name.c_str()); };

        m_items.add_item(new uie::simple_command_menu_node{view_name, description, display_flags, set_view});
    }
};
