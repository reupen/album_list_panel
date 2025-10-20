#include "stdafx.h"
#include "playlist_utils.h"

namespace alp {

namespace {

class TitleformatHookView : public titleformat_hook {
public:
    bool process_field(
        titleformat_text_out* p_out, const char* p_name, size_t p_name_length, bool& p_found_flag) override
    {
        p_found_flag = false;
        if (m_view) {
            if (!stricmp_utf8_ex(p_name, p_name_length, "_view", 5)) {
                p_out->write(titleformat_inputtypes::meta, m_view, pfc_infinite);
                p_found_flag = true;
                return true;
            }
        }
        return false;
    }

    bool process_function(titleformat_text_out* p_out, const char* p_name, size_t p_name_length,
        titleformat_hook_function_params* p_params, bool& p_found_flag) override
    {
        p_found_flag = false;
        return false;
    }

    explicit TitleformatHookView(const char* p_view) : m_view{p_view} {}

private:
    const char* m_view;
};

} // namespace

void send_tracks_to_playlist(const metadb_handle_list& tracks, bool replace_contents)
{
    auto api = playlist_manager::get();
    const bool select = !!cfg_add_items_select;
    api->activeplaylist_undo_backup();

    if (replace_contents)
        api->activeplaylist_clear();
    else if (select)
        api->activeplaylist_clear_selection();

    api->activeplaylist_add_items(tracks, bit_array_val(select));

    if (select && !replace_contents) {
        const size_t num = api->activeplaylist_get_item_count();
        if (num > 0) {
            api->activeplaylist_set_focus_item(num - 1);
        }
    }
}

void send_nodes_to_playlist(const std::vector<node_ptr>& nodes, bool replace_contents, bool create_new)
{
    const auto tracks = get_node_tracks(nodes);

    if (tracks.tracks().size() == 0)
        return;

    if (create_new) {
        auto node_names = nodes | ranges::views::transform([](auto& node) { return node->get_display_name(); })
            | ranges::views::take(3);
        auto playlist_name = mmh::join<decltype(node_names)&, std::string_view, std::string>(node_names, ", ");

        if (nodes.size() > 3)
            playlist_name += u8", …"_pcc;

        const auto api = playlist_manager::get();
        if (const auto index
            = fbh::as_optional(api->create_playlist(playlist_name.data(), playlist_name.size(), pfc_infinite))) {
            api->set_active_playlist(*index);
            send_tracks_to_playlist(tracks.tracks(), true);
        }
    } else
        send_tracks_to_playlist(tracks.tracks(), replace_contents);

    if (replace_contents && cfg_play_on_send) {
        playlist_manager::get()->reset_playing_playlist();
        play_control::get()->play_start();
    }
}

void send_nodes_to_autosend_playlist(const std::vector<node_ptr>& nodes, const char* view, bool b_play)
{
    const auto tracks = get_node_tracks(nodes);

    if (tracks.tracks().size() == 0)
        return;

    const auto api = playlist_manager::get();
    pfc::string8 playlist_name;

    TitleformatHookView tf_hook{view};
    titleformat_compiler::get()->run(&tf_hook, playlist_name, cfg_autosend_playlist_name);
    const auto index = api->find_or_create_playlist(playlist_name, pfc_infinite);

    api->playlist_undo_backup(index);
    api->playlist_clear(index);
    api->playlist_add_items(index, tracks.tracks(), bit_array_val(cfg_add_items_select != 0));

    api->set_active_playlist(index);

    if (b_play && cfg_play_on_send) {
        api->reset_playing_playlist();
        play_control::get()->play_start();
    }
}
} // namespace alp
