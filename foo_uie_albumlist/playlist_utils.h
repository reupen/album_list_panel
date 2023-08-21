#pragma once

namespace alp {

void send_tracks_to_playlist(const metadb_handle_list& tracks, bool replace_contents);
void send_nodes_to_playlist(const std::vector<node_ptr>& nodes, bool replace_contents, bool create_new);
void send_nodes_to_autosend_playlist(const std::vector<node_ptr>& nodes, const char* view, bool b_play);

} // namespace alp
