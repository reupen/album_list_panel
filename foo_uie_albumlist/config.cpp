#include "stdafx.h"

struct {
    const char *name, *value;
} cfg_view_list_defaults[] = {
    {"by album", "%album%[ '['%album artist%']']|[[%discnumber%.]%tracknumber%. ][%track artist% - ]%title%"},
    {"by artist", "%<artist>%|%album%|[[%discnumber%.]%tracknumber%. ][%track artist% - ]%title%"},
    {"by genre", "%<genre>%|[%album artist% - ]%album%|[[%discnumber%.]%tracknumber%. ][%track artist% - ]%title%"},
    {"by year", "%<date>%|[%album artist% - ]%album%|[[%discnumber%.]%tracknumber%. ][%track artist% - ]%title%"},
    {"by artist/album",
        "[%album artist% - ]['['%date%']' ]%album%|[[%discnumber%.]%tracknumber%. ][%track artist% - ]%title%"},
};

CfgViewList cfg_views_v1(GUID{0xc584d488, 0x53dd, 0x4d21, 0xa8, 0x5b, 0x8e, 0xc7, 0xcc, 0xb3, 0x82, 0x16}, true);
CfgViewList cfg_views_v2(GUID{0xa7398829, 0x046f, 0x43cb, {0xb6, 0x82, 0x25, 0x6a, 0x2f, 0xd6, 0x94, 0x1b}}, false);

cfg_string cfg_autosend_playlist_name(
    GUID{0x4beb593f, 0xa010, 0x8b3a, 0x7b, 0x2a, 0x63, 0xad, 0xf3, 0x9f, 0xc6, 0xb4}, "Library view");

cfg_int cfg_show_subitem_counts(GUID{0xdd6f150e, 0xc995, 0xf018, 0x28, 0x58, 0x24, 0x85, 0x3d, 0xed, 0xf3, 0x52}, 0);
cfg_int cfg_show_item_indices(GUID{0x6a35739b, 0xbf63, 0x541a, 0x10, 0x5f, 0xdd, 0x41, 0x56, 0x67, 0xcd, 0xa0}, 0);
cfg_int cfg_double_click_action(GUID{0x3c8a9131, 0x8326, 0xc47a, 0x55, 0x07, 0x63, 0xef, 0xe5, 0x89, 0xfb, 0xb2}, 0);
cfg_int cfg_middle_click_action(GUID{0xb35b99b8, 0xdb22, 0xc748, 0x98, 0xaa, 0x84, 0xa2, 0x08, 0x36, 0x3d, 0x99}, 3);
cfg_int cfg_autosend(GUID{0x0ab307e0, 0xc615, 0x8446, 0x09, 0x40, 0x37, 0x2b, 0xf9, 0x41, 0xd2, 0x87}, 1);
cfg_int cfg_process_keyboard_shortcuts(
    GUID{0xcb1163d6, 0x42b9, 0xcb55, 0x35, 0x5c, 0xef, 0x80, 0xbd, 0x26, 0x9d, 0xe4}, 1);
cfg_int cfg_play_on_send(GUID{0xe03def63, 0x2645, 0x6103, 0xb3, 0xb9, 0xe7, 0x97, 0xe4, 0x6c, 0xf8, 0x56}, 1);
cfg_int cfg_populate_on_init(GUID{0x57cbf474, 0xff1a, 0xb8a1, 0x2d, 0x1a, 0xad, 0x8b, 0x49, 0x62, 0xa2, 0x02}, 1);
cfg_int cfg_show_horizontal_scroll_bar(
    GUID{0x0d9662bf, 0x3b8f, 0x410b, 0xb6, 0x7d, 0x94, 0xd4, 0x35, 0x50, 0x6d, 0x73}, 0);
cfg_int cfg_frame_style(GUID{0xa7bfe9a9, 0xbb1e, 0x0e80, 0xc9, 0x52, 0xbb, 0x23, 0x90, 0xa7, 0xe6, 0x03}, 0);
cfg_int cfg_show_root_node(GUID{0x2be2f1ea, 0x0bc5, 0x7a67, 0xdc, 0x22, 0xed, 0xf5, 0x6f, 0x51, 0x13, 0x35}, 1);
cfg_int cfg_use_custom_indentation(GUID{0x8cd7d26a, 0x524a, 0xf53f, 0xb3, 0x07, 0x73, 0x4e, 0x96, 0x0d, 0xe0, 0xa1}, 0);
fbh::ConfigInt32DpiAware cfg_custom_indentation_amount(
    GUID{0xa45b79c4, 0x14ff, 0x6118, 0x08, 0xd4, 0x7b, 0xa9, 0xfd, 0x97, 0xdc, 0x36}, 19);
cfg_int cfg_use_custom_vertical_item_padding(
    GUID{0x8e06049e, 0xfcdc, 0xb446, 0xf8, 0xcb, 0xbf, 0x11, 0xc2, 0xf0, 0x34, 0x1b}, 0);
fbh::ConfigInt32DpiAware cfg_custom_vertical_padding_amount(
    GUID{0xc3e0c661, 0xcb3f, 0x80cf, 0x91, 0x05, 0xb4, 0x44, 0x5a, 0xf4, 0x9f, 0xb2}, 4);
cfg_int cfg_add_items_use_core_sort(
    GUID{0x62bdcc52, 0xd067, 0xb238, 0xbe, 0x2a, 0xc9, 0xbd, 0x26, 0x13, 0xd6, 0xc8}, 0);
cfg_int cfg_add_items_select(GUID{0x3918ae65, 0xfdfb, 0xe23e, 0xfc, 0x3b, 0xee, 0xce, 0xca, 0xc1, 0xe9, 0x83}, 1);
cfg_int cfg_collapse_other_nodes_on_expansion(
    GUID{0x2a9d24a2, 0x2705, 0xb35e, 0xdb, 0x20, 0x86, 0xc6, 0x90, 0xc6, 0xe9, 0x4c}, 0);

void CfgViewList::get_data_raw(stream_writer* out, abort_callback& p_abort)
{
    if (m_read_and_write_legacy_size_value) {
        if (cfg_views_v2.has_read_values())
            *this = cfg_views_v2;
    }
    const auto item_count = m_data.get_count();

    if (m_read_and_write_legacy_size_value)
        out->write_lendian_t(item_count, p_abort);
    else
        out->write_lendian_t(gsl::narrow<uint32_t>(item_count), p_abort);

    for (size_t i{0}; i < item_count; ++i) {
        out->write_string(m_data[i].name, p_abort);
        out->write_string(m_data[i].value, p_abort);
    }
}

void CfgViewList::set_data_raw(stream_reader* r, size_t psize, abort_callback& p_abort)
{
    m_data.remove_all();

    const size_t item_count = m_read_and_write_legacy_size_value ? r->read_lendian_t<size_t>(p_abort)
                                                                 : r->read_lendian_t<uint32_t>(p_abort);

    for (size_t i{0}; i < item_count; ++i) {
        entry item;
        r->read_string(item.name, p_abort);
        r->read_string(item.value, p_abort);
        m_data.add_item(item);
    }

    m_has_set_values = true;
}

void CfgViewList::reset()
{
    m_data.remove_all();
    for (size_t i{0}; i < tabsize(cfg_view_list_defaults); ++i) {
        m_data.add_item(entry{cfg_view_list_defaults[i].name, cfg_view_list_defaults[i].value});
    }
}

CfgViewList& get_views()
{
    if (cfg_views_v1.has_read_values() && !cfg_views_v2.has_read_values()) {
        cfg_views_v2 = cfg_views_v1;
    }

    return cfg_views_v2;
}
