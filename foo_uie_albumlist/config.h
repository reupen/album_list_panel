#pragma once

class CfgViewList : public cfg_var {
public:
    CfgViewList& operator=(const CfgViewList& source)
    {
        m_data = source.m_data;
        m_has_set_values = true;
        return *this;
    }

    void get_data_raw(stream_writer* out, abort_callback& p_abort) override;
    void set_data_raw(stream_reader* r, size_t psize, abort_callback& p_abort) override;

    void reset();

    const char* get_name(size_t index) const
    {
        if (index >= 0 && index < m_data.get_count())
            return m_data[index].name;
        return nullptr;
    }

    const char* get_value(size_t index) const
    {
        if (index >= 0 && index < m_data.get_count())
            return m_data[index].value;
        return nullptr;
    }

    size_t find_item(const char* name) const
    {
        const auto count = m_data.get_count();
        for (size_t i{0}; i < count; i++)
            if (!stricmp_utf8(m_data[i].name, name))
                return i;

        return (std::numeric_limits<size_t>::max)();
    }

    size_t add_item(const char* name, const char* value) { return m_data.add_item(entry{name, value}); }

    void remove_item(size_t index) { m_data.remove_by_idx(index); }

    void remove_all() { m_data.remove_all(); }

    void modify_item(size_t index, const char* name, const char* value)
    {
        auto& p_entry = m_data[index];
        p_entry.name = name;
        p_entry.value = value;
    }

    size_t get_count() const { return m_data.get_count(); }

    void swap(size_t index1, size_t index2) { m_data.swap_items(index1, index2); }

    CfgViewList(const GUID& p_guid, bool read_and_write_legacy_size_value)
        : cfg_var(p_guid)
        , m_read_and_write_legacy_size_value(read_and_write_legacy_size_value)
    {
        reset();
    }

    void format_display(size_t index, pfc::string_base& out) const
    {
        out = get_name(index);
        out += " : ";
        out += get_value(index);
    }

    bool has_read_values() const { return m_has_set_values; }

private:
    struct entry {
        pfc::string8 name;
        pfc::string8 value;
    };

    pfc::list_t<entry> m_data;
    bool m_read_and_write_legacy_size_value{};
    bool m_has_set_values{};
};

constexpr GUID album_list_font_client_id{0x6b856cc, 0x86e7, 0x4459, 0xa7, 0x5c, 0x2d, 0xab, 0x5b, 0x33, 0xb8, 0xbb};
constexpr GUID album_list_items_colours_client_id{
    0xda66e8f3, 0xd210, 0x4ad2, 0x89, 0xd4, 0x9b, 0x2c, 0xc5, 0x8d, 0x2, 0x35};
constexpr GUID album_list_filter_colours_client_id{
    0xd1530949, 0xa059, 0x454c, {0x9a, 0x42, 0x9a, 0x46, 0xe6, 0xc7, 0x5a, 0x68}};
constexpr GUID album_list_panel_preferences_page_id{
    0x53c89e50, 0x685d, 0x8ed1, 0x43, 0x25, 0x6b, 0xe8, 0x0f, 0x1b, 0xe7, 0x1f};

extern cfg_bool cfg_themed;
extern cfg_int cfg_populate_on_init;
extern cfg_int cfg_autosend;
extern cfg_int cfg_collapse_other_nodes_on_expansion;
extern cfg_int cfg_add_items_use_core_sort;
extern cfg_int cfg_add_items_select;
extern cfg_int cfg_show_subitem_counts;
extern cfg_int cfg_show_item_indices;
extern cfg_int cfg_double_click_action;
extern cfg_int cfg_process_keyboard_shortcuts;
extern cfg_int cfg_middle_click_action;
extern cfg_int cfg_frame_style;
extern cfg_int cfg_show_horizontal_scroll_bar;
extern cfg_int cfg_show_root_node;
extern cfg_int cfg_play_on_send;
extern cfg_int cfg_use_custom_indentation;
extern fbh::ConfigInt32DpiAware cfg_custom_indentation_amount;
extern fbh::ConfigInt32DpiAware cfg_custom_vertical_padding_amount;
extern cfg_int cfg_use_custom_vertical_item_padding;
extern cfg_string cfg_autosend_playlist_name;

CfgViewList& get_views();
