#pragma once

#include "node_formatter.h"

#define IDC_TREE 1000
#define IDC_FILTER 1001
#define EDIT_TIMER_ID 2001

namespace alp {

struct SavedScrollPosition {
    int horizontal_position{};
    int vertical_position{};
    int vertical_max{};
};

} // namespace alp

extern const char* directory_structure_view_name;

enum class ClickAction {
    none_or_default,
    send_to_playlist,
    add_to_playlist,
    send_to_new_playlist,
    send_to_autosend_playlist
};

class album_list_window
    : public uie::container_uie_window_v3
    , public library_callback_dynamic
    , public library_callback_v2_dynamic {
    friend class font_notify;
    friend class node;

public:
    static void s_update_all_tree_colours();
    static void s_update_all_tree_themes();
    static void s_update_all_edit_themes();
    static void s_update_all_edit_colours();
    static void s_update_all_item_heights();
    static void s_update_all_indents();
    static void s_update_all_labels();
    static void s_update_all_showhscroll();
    static void s_update_all_fonts();
    static void s_refresh_all();
    static void s_on_view_script_change(const char* p_view_before, const char* p_view);
    static void s_update_all_window_frames();

    ~album_list_window();

    bool is_bydir() const;
    const char* get_hierarchy() const;
    const char* get_view() const;
    void set_view(const char* view);
    void toggle_show_filter();

    void create_or_destroy_filter();
    void create_filter();
    void destroy_filter();
    void create_tree();
    void destroy_tree(bool should_save_scroll_position);
    void recreate_tree(bool save_state);

    void save_scroll_position() const;
    void restore_scroll_position();
    void on_size(unsigned cx, unsigned cy);
    void on_size();

    void refresh_tree();
    void update_tree(metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_add,
        metadb_handle_list_t<pfc::alloc_fast_aggressive>& to_remove, bool preserve_existing);
    void build_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& tracks, bool preserve_existing = false);
    void remove_nodes(metadb_handle_list_t<pfc::alloc_fast_aggressive>& p_tracks);

    void update_all_labels();
    void update_tree_theme(
        const cui::colours::helper& colours = cui::colours::helper(album_list_items_colours_client_id)) const;
    void update_tree_colours();
    void update_tooltip_theme() const;
    void update_edit_theme() const;
    void update_edit_colours() const;
    void update_item_height();
    void on_view_script_change(const char* p_view_before, const char* p_view);

    const GUID& get_extension_guid() const override { return s_extension_guid; }

    void get_name(pfc::string_base& out) const override;
    void get_category(pfc::string_base& out) const override;

    void set_config(stream_reader* p_reader, t_size size, abort_callback& p_abort) override;
    void get_config(stream_writer* p_writer, abort_callback& p_abort) const override;

    unsigned get_type() const override { return ui_extension::type_panel; }

    uie::container_window_v3_config get_window_config() override { return {L"{606E9CDD-45EE-4c3b-9FD5-49381CEBE8AE}"}; }

    void on_task_completion(t_uint32 task, t_uint32 code);
    void on_items_added(const pfc::list_base_const_t<metadb_handle_ptr>& p_data) override;
    void on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr>& p_data) override;
    void on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr>& p_data) override;
    void on_items_modified_v2(metadb_handle_list_cref items, metadb_io_callback_v2_data& data) override;
    void on_library_initialized() override;

    LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) override;
    LRESULT on_wm_contextmenu(POINT pt);
    std::optional<LRESULT> on_tree_view_wm_notify(LPNMHDR hdr);
    void get_menu_items(ui_extension::menu_hook_t& p_hook) override;

private:
    static LRESULT WINAPI s_tree_hook_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    LRESULT WINAPI on_tree_hooked_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

    static inline pfc::ptr_list_t<album_list_window> s_instances;
    static const GUID s_extension_guid;
    static const char* s_class_name;
    static inline wil::unique_hfont s_font;
    static inline wil::unique_hbrush s_filter_background_brush;

    bool do_click_action(ClickAction click_action);
    void collapse_other_nodes(const node_ptr& node) const;

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
    bool m_process_char{true};
    POINT m_clickpoint{};
    mutable std::optional<alp::SavedScrollPosition> m_saved_scroll_position{};
    pfc::string8 m_view{"by artist/album"};
    node_ptr m_root;
    node_ptr m_selection;
    std::optional<alp::SavedNodeState> m_node_state;
    search_filter::ptr m_filter_ptr;
    ui_selection_holder::ptr m_selection_holder;
    NodeFormatter m_node_formatter;
    library_manager_v4::ptr m_library_v4;
    library_manager_v3::ptr m_library_v3;
};
