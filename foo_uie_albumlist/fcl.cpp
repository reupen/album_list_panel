#include "stdafx.h"

// {3E3F5B9F-5599-417e-A867-AC690EF01A8A}
const GUID g_guid_fcl_group_album_list_views{
    0x3e3f5b9f, 0x5599, 0x417e, {0xa8, 0x67, 0xac, 0x69, 0xe, 0xf0, 0x1a, 0x8a}};

// {F45D4B85-FA35-4abd-A04B-345D06C39E38}
const GUID g_guid_fcl_dataset_album_list_views{
    0xf45d4b85, 0xfa35, 0x4abd, {0xa0, 0x4b, 0x34, 0x5d, 0x6, 0xc3, 0x9e, 0x38}};

// {E93DF451-6196-48fd-A67B-F9056705336A}
const GUID g_guid_fcl_dataset_album_list_appearance{
    0xe93df451, 0x6196, 0x48fd, {0xa6, 0x7b, 0xf9, 0x5, 0x67, 0x5, 0x33, 0x6a}};

class album_list_fcl_views : public cui::fcl::dataset {
public:
    void get_name(pfc::string_base& p_out) const override { p_out = "Album List Views"; }
    const GUID& get_guid() const override { return g_guid_fcl_dataset_album_list_views; }
    const GUID& get_group() const override { return g_guid_fcl_group_album_list_views; }

    void get_data(stream_writer* writer, t_uint32 type, cui::fcl::t_export_feedback& feedback,
        abort_callback& p_abort) const override
    {
        fbh::fcl::Writer w(writer, p_abort);
        w.write_raw(static_cast<uint32_t>(stream_version));
        const uint32_t count = pfc::downcast_guarded<uint32_t>(cfg_views.get_count());
        w.write_raw(count);

        for (uint32_t i{0}; i < count; i++) {
            w.write_raw(uint32_t{2});
            w.write_item(view_name, cfg_views.get_name(i));
            w.write_item(view_script, cfg_views.get_value(i));
        }
    }

    void set_data(stream_reader* reader, size_t size, t_uint32 type, cui::fcl::t_import_feedback& feedback,
        abort_callback& p_abort) override
    {
        fbh::fcl::Reader fcl_reader(reader, size, p_abort);
        const auto version = fcl_reader.read_raw_item<uint32_t>();

        if (version <= stream_version) {
            cfg_views.remove_all();
            const auto count = fcl_reader.read_raw_item<uint32_t>();

            for (uint32_t i{0}; i < count; i++)
                read_item(fcl_reader);
        }
        if (g_config_general.is_active())
            g_config_general.refresh_views();
        album_list_window::s_refresh_all();
    }

private:
    enum { stream_version = 0 };

    enum { view_name, view_script };

    static void read_item(fbh::fcl::Reader& fcl_reader)
    {
        auto elems = fcl_reader.read_raw_item<uint32_t>();

        pfc::string8 name;
        pfc::string8 script;

        while (elems) {
            const auto id = fcl_reader.read_raw_item<uint32_t>();
            const auto elem_size = fcl_reader.read_raw_item<uint32_t>();

            switch (id) {
            case view_name:
                fcl_reader.read_item(name, elem_size);
                break;
            case view_script:
                fcl_reader.read_item(script, elem_size);
                break;
            default:
                fcl_reader.skip(elem_size);
                break;
            }
            --elems;
        }
        cfg_views.add_item(name, script);
    }
};

class album_list_fcl_appearance : public cui::fcl::dataset {
public:
    void get_name(pfc::string_base& p_out) const override { p_out = "Album List appearance settings"; }
    const GUID& get_guid() const override { return g_guid_fcl_dataset_album_list_appearance; }
    const GUID& get_group() const override { return cui::fcl::groups::colours_and_fonts; }

    void get_data(stream_writer* writer, t_uint32 type, cui::fcl::t_export_feedback& feedback,
        abort_callback& p_abort) const override
    {
        fbh::fcl::Writer w(writer, p_abort);

        w.write_raw(static_cast<uint32_t>(stream_version));
        w.write_item(id_sub_item_counts, cfg_show_subitem_counts);
        w.write_item(id_sub_item_indices, cfg_show_item_indices);
        w.write_item(id_horizontal_scrollbar, cfg_show_horizontal_scroll_bar);
        w.write_item(id_root_node, cfg_show_root_node);
        w.write_item(id_use_item_padding, cfg_use_custom_vertical_item_padding);
        w.write_item(id_item_padding, cfg_custom_vertical_padding_amount.get_raw_value().value);
        w.write_item(id_item_padding_dpi, cfg_custom_vertical_padding_amount.get_raw_value().dpi);
        w.write_item(id_use_indentation, cfg_use_custom_indentation);
        w.write_item(id_indentation, cfg_custom_indentation_amount.get_raw_value().value);
        w.write_item(id_indentation_dpi, cfg_custom_indentation_amount.get_raw_value().dpi);
        w.write_item(id_edge_style, cfg_frame_style);
    }

    void set_data(stream_reader* reader, size_t size, t_uint32 type, cui::fcl::t_import_feedback& feedback,
        abort_callback& p_abort) override
    {
        fbh::fcl::Reader fcl_reader(reader, size, p_abort);
        const auto version = fcl_reader.read_raw_item<uint32_t>();
        if (version <= stream_version) {
            read_items(fcl_reader);
            album_list_window::s_update_all_item_heights();
            album_list_window::s_update_all_indents();
            album_list_window::s_update_all_window_frames();
        }
    }

private:
    enum { stream_version = 0 };

    enum {
        id_sub_item_counts,
        id_sub_item_indices,
        id_horizontal_scrollbar,
        id_root_node,
        id_use_item_padding,
        id_item_padding,
        id_use_indentation,
        id_indentation,
        id_edge_style,
        id_indentation_dpi,
        id_item_padding_dpi,
    };

    static void read_items(fbh::fcl::Reader& fcl_reader)
    {
        uih::IntegerAndDpi<int32_t> indentation(0, uih::get_system_dpi_cached().cx);
        uih::IntegerAndDpi<int32_t> item_padding(0, uih::get_system_dpi_cached().cx);
        std::unordered_set<uint32_t> read_element_ids;

        while (fcl_reader.get_remaining()) {
            const auto id = fcl_reader.read_raw_item<uint32_t>();
            const auto elem_size = fcl_reader.read_raw_item<uint32_t>();

            read_element_ids.emplace(id);

            switch (id) {
            case id_sub_item_counts:
                fcl_reader.read_item(cfg_show_subitem_counts);
                break;
            case id_sub_item_indices:
                fcl_reader.read_item(cfg_show_item_indices);
                break;
            case id_horizontal_scrollbar:
                fcl_reader.read_item(cfg_show_horizontal_scroll_bar);
                break;
            case id_root_node:
                fcl_reader.read_item(cfg_show_root_node);
                break;
            case id_use_item_padding:
                fcl_reader.read_item(cfg_use_custom_vertical_item_padding);
                break;
            case id_item_padding:
                fcl_reader.read_item(item_padding.value);
                break;
            case id_item_padding_dpi:
                fcl_reader.read_item(item_padding.dpi);
                break;
            case id_use_indentation:
                fcl_reader.read_item(cfg_use_custom_indentation);
                break;
            case id_indentation:
                fcl_reader.read_item(indentation.value);
                break;
            case id_indentation_dpi:
                fcl_reader.read_item(indentation.dpi);
                break;
            case id_edge_style:
                fcl_reader.read_item(cfg_frame_style);
                break;
            default:
                fcl_reader.skip(elem_size);
                break;
            }
        }

        if (read_element_ids.contains(id_indentation)) {
            cfg_custom_indentation_amount = indentation;
        }

        if (read_element_ids.contains(id_item_padding)) {
            cfg_custom_vertical_padding_amount = item_padding;
        }
    }
};

cui::fcl::group_impl_factory g_fclgroup{g_guid_fcl_group_album_list_views, "Album List Views",
    "Album List view title format scripts", cui::fcl::groups::title_scripts};

cui::fcl::dataset_factory<album_list_fcl_views> g_album_list_fcl_views;
cui::fcl::dataset_factory<album_list_fcl_appearance> g_album_list_fcl_appearance;
