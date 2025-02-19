#pragma once

namespace alp {

struct SavedNodeState {
    pfc::string name;
    bool expanded{};
    bool selected{};
    std::vector<SavedNodeState> children;
};

auto find_node_state(const std::vector<SavedNodeState>& items, const char* child_name) -> std::optional<SavedNodeState>;
void write_node_state(stream_writer* writer, const SavedNodeState& state, abort_callback& aborter);
auto read_node_state(stream_reader* reader, abort_callback& aborter, std::optional<uint32_t> read_size = {})
    -> SavedNodeState;

} // namespace alp
