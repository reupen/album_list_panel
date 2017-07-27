#pragma once

#define IDC_TREE 1000
#define IDC_FILTER 1001
#define EDIT_TIMER_ID 2001

#define USE_TIMER

extern const char * directory_structure_view_name;

class album_list_window : public ui_extension::container_ui_extension, public library_callback_dynamic
{
    friend class font_notify;
    friend class node;
public:
    static void s_update_all_colours();
    static void s_update_all_item_heights();
    static void s_update_all_indents();
    static void s_update_all_labels();
    static void s_update_all_showhscroll();
    static void s_update_all_fonts();
    static void s_refresh_all();
    static void s_on_view_script_change(const char * p_view_before, const char * p_view);
    static void s_update_all_window_frames();

    void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) override;
    void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) override;
    void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & p_data) override;

    bool is_bydir()
    {
        return !stricmp_utf8(m_view, directory_structure_view_name);
    }

    const char * get_hierarchy()
    {
        unsigned idx = cfg_view_list.find_item(m_view);
        if (idx != (unsigned)(-1)) return cfg_view_list.get_value(idx);
        return "N/A";
    }

    LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) override;
    LRESULT WINAPI on_hook(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    void create_or_destroy_filter();
    void create_filter();
    void destroy_filter();
    void create_tree();
    void destroy_tree();
    void on_task_completion(t_uint32 task, t_uint32 code);
    void on_size(unsigned cx, unsigned cy);
    void on_size();

    void refresh_tree();
    void rebuild_nodes();
    void build_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& tracks, bool preserve_existing = false);
    void remove_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks);
    void update_all_labels();
    void update_colours();
    void update_item_height();
    void on_view_script_change(const char * p_view_before, const char * p_view);

    ~album_list_window();

    const GUID & get_extension_guid() const override
    {
        return s_extension_guid;
    }

    void get_name(string_base & out)const override;
    void get_category(string_base & out)const override;

    void set_config(stream_reader * p_reader, t_size size, abort_callback & p_abort) override;
    void get_config(stream_writer * p_writer, abort_callback & p_abort)const override;

    unsigned get_type() const override { return ui_extension::type_panel; }

    class_data & get_class_data()const override
    {
        __implement_get_class_data(_T("{606E9CDD-45EE-4c3b-9FD5-49381CEBE8AE}"), false);
    }

    class menu_node_settings : public ui_extension::menu_node_command_t
    {
    public:
        bool get_display_data(pfc::string_base & p_out, unsigned & p_displayflags) const override
        {
            p_out = "Settings";
            p_displayflags = 0;
            return true;
        }
        bool get_description(pfc::string_base & p_out) const override
        {
            return false;
        }
        void execute() override
        {
            static_api_ptr_t<ui_control>()->show_preferences(g_guid_preferences_album_list_panel);
        }
        menu_node_settings() {};
    };
    class menu_node_filter : public ui_extension::menu_node_command_t
    {
        service_ptr_t<album_list_window> p_this;
    public:
        bool get_display_data(pfc::string_base & p_out, unsigned & p_displayflags) const override
        {
            p_out = "Filter";
            p_displayflags = p_this->m_filter ? uie::menu_node_t::state_checked : 0;
            return true;
        }
        bool get_description(pfc::string_base & p_out) const override
        {
            return false;
        }
        void execute() override
        {
            p_this->m_filter = !p_this->m_filter;
            p_this->create_or_destroy_filter();
        }
        menu_node_filter(album_list_window * p_wnd) : p_this(p_wnd) {};
    };

    class menu_node_view : public ui_extension::menu_node_command_t
    {
        service_ptr_t<album_list_window> p_this;
        string_simple view;
    public:
        bool get_display_data(string_base & p_out, unsigned & p_displayflags)const override
        {
            p_out = view;
            p_displayflags = (!stricmp_utf8(view, p_this->m_view) ? ui_extension::menu_node_t::state_checked : 0);
            return true;
        }
        bool get_description(string_base & p_out)const override
        {
            return false;
        }
        void execute() override
        {
            p_this->m_view = view;
            p_this->refresh_tree();
        }
        menu_node_view(album_list_window * p_wnd, const char * p_value) : p_this(p_wnd), view(p_value) {};
    };

    class menu_node_select_view : public ui_extension::menu_node_popup_t
    {
        list_t<ui_extension::menu_node_ptr> m_items;
    public:
        bool get_display_data(string_base & p_out, unsigned & p_displayflags)const override
        {
            p_out = "View";
            p_displayflags = 0;
            return true;
        }
        unsigned get_children_count()const override { return m_items.get_count(); }
        void get_child(unsigned p_index, uie::menu_node_ptr & p_out)const override { p_out = m_items[p_index].get_ptr(); }
        menu_node_select_view(album_list_window * p_wnd)
        {
            unsigned n, m = cfg_view_list.get_count();
            string8_fastalloc temp;
            temp.prealloc(32);

            m_items.add_item(new menu_node_view(p_wnd, directory_structure_view_name));

            for (n = 0; n<m; n++)
            {
                m_items.add_item(new menu_node_view(p_wnd, cfg_view_list.get_name(n)));
            };
        };
    };

    void get_menu_items(ui_extension::menu_hook_t & p_hook) override
    {
        p_hook.add_node(ui_extension::menu_node_ptr(new menu_node_select_view(this)));
        p_hook.add_node(ui_extension::menu_node_ptr(new menu_node_filter(this)));
        p_hook.add_node(ui_extension::menu_node_ptr(new menu_node_settings()));
    }

private:
    static LRESULT WINAPI s_hook_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    static ptr_list_t<album_list_window> s_instances;
    static const GUID s_extension_guid;
    static const char* s_class_name;
    static HFONT s_font;

    HWND m_wnd_tv{nullptr};
    HWND m_wnd_edit{nullptr};
    HTHEME m_dd_theme{nullptr};
    WNDPROC m_treeproc{nullptr};
    bool m_initialised{false};
    bool m_populated{false};
    bool m_dragging{false};
    bool m_clicked{false};
    bool m_filter{false};
    bool m_timer{false};
    DWORD m_clickpoint{0};
    int m_indent_default{0};
    string8 m_view{"by artist/album"};
    node_ptr m_root;
    node_ptr m_selection;
    search_filter::ptr m_filter_ptr;
    ui_selection_holder::ptr m_selection_holder;
};

void TreeView_CollapseOtherNodes(HWND wnd, HTREEITEM ti);
void do_playlist(node_ptr const & src, bool replace, bool b_new = false);
void do_autosend_playlist(node_ptr const & src, string_base & view, bool b_play = false);
