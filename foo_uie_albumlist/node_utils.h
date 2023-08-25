#pragma once

namespace alp {

class NodeTracksHolder {
public:
    explicit NodeTracksHolder(const std::vector<node_ptr>& nodes)
    {
        if (nodes.size() == 1) {
            m_tracks_ref = nodes[0]->get_sorted_tracks();
        } else {
            for (auto& node_ : nodes) {
                m_tracks.add_items(node_->get_sorted_tracks());
            }
        }
    }

    const metadb_handle_list& tracks() const { return m_tracks_ref; }

private:
    metadb_handle_list m_tracks;
    std::reference_wrapper<const metadb_handle_list> m_tracks_ref{m_tracks};
};

NodeTracksHolder get_node_tracks(const std::vector<node_ptr>& nodes);

std::vector<node_ptr> clean_selection(std::unordered_set<node_ptr> selection);

} // namespace alp
