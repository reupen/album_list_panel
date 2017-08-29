#pragma once

class cfg_view_list_t : public cfg_var {
public:
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

    size_t add_item(const char* name, const char* value)
    {
        return m_data.add_item(entry{name, value});
    }

    void remove_item(size_t index)
    {
        m_data.remove_by_idx(index);
    }

    void remove_all()
    {
        m_data.remove_all();
    }

    void modify_item(size_t index, const char* name, const char* value)
    {
        auto& p_entry = m_data[index];
        p_entry.name = name;
        p_entry.value = value;
    }

    size_t get_count() const
    {
        return m_data.get_count();
    }

    void swap(size_t index1, size_t index2)
    {
        m_data.swap_items(index1, index2);
    }

    cfg_view_list_t(const GUID& p_guid) : cfg_var(p_guid)
    {
        reset();
    }

    void format_display(size_t index, string_base& out) const
    {
        out = get_name(index);
        out += " : ";
        out += get_value(index);
    }

private:
    struct entry {
        pfc::string8 name;
        pfc::string8 value;
    };

    pfc::list_t<entry> m_data;
};

extern const GUID g_guid_album_list_font,
    g_guid_album_list_colours,
    g_guid_preferences_album_list_panel;

extern cfg_view_list_t cfg_views;
extern cfg_bool cfg_themed;
extern cfg_int cfg_populate_on_init,
    cfg_autosend,
    cfg_collapse_other_nodes_on_expansion,
    cfg_add_items_use_core_sort,
    cfg_add_items_select,
    cfg_show_subitem_counts,
    cfg_show_item_indices,
    cfg_double_click_action,
    cfg_process_keyboard_shortcuts,
    cfg_middle_click_action,
    cfg_frame_style,
    cfg_show_horizontal_scroll_bar,
    cfg_show_root_node,
    cfg_play_on_send,
    cfg_use_custom_indentation,
    cfg_custom_indentation_amount,
    cfg_custom_vertical_padding_amount,
    cfg_use_custom_vertical_item_padding;
extern cfg_string cfg_autosend_playlist_name;
