#include "stdafx.h"

namespace alp {

auto find_node_state(const std::vector<SavedNodeState>& items, std::string_view child_name)
    -> std::optional<SavedNodeState>
{
    const auto child_name_utf16 = mmh::to_utf16(child_name);

    const auto [start, end] = std::ranges::equal_range(
        items, child_name_utf16,
        [](auto&& left, auto&& right) { return StrCmpLogicalW(left.c_str(), right.c_str()) < 0; },
        [](auto& child) { return mmh::to_utf16(child.name); });

    if (start != end)
        return *start;

    return {};
}

void write_node_state(stream_writer* writer, const SavedNodeState& state, abort_callback& aborter)
{
    stream_writer_memblock temp_writer;
    temp_writer.write_string(state.name, aborter);
    temp_writer.write_lendian_t(state.expanded, aborter);

    temp_writer.write_lendian_t(gsl::narrow<uint32_t>(state.children.size()), aborter);

    for (const auto& child : state.children) {
        stream_writer_memblock child_writer;

        write_node_state(&child_writer, child, aborter);

        temp_writer.write(child_writer.m_data.get_ptr(), child_writer.m_data.get_size(), aborter);
    }

    temp_writer.write_lendian_t(state.selected, aborter);

    writer->write_lendian_t(gsl::narrow<uint32_t>(temp_writer.m_data.get_size()), aborter);
    writer->write(temp_writer.m_data.get_ptr(), temp_writer.m_data.get_size(), aborter);
}

auto read_node_state(stream_reader* reader, abort_callback& aborter, std::optional<uint32_t> read_size)
    -> SavedNodeState
{
    const auto size = read_size ? *read_size : reader->read_lendian_t<uint32_t>(aborter);

    stream_reader_limited_ref limited_reader(reader, size);

    SavedNodeState state;
    state.name = limited_reader.read_string(aborter);
    state.expanded = limited_reader.read_lendian_t<bool>(aborter);
    auto child_count = limited_reader.read_lendian_t<uint32_t>(aborter);

    state.children = std::ranges::views::iota(0u, child_count)
        | std::ranges::views::transform(
            [&limited_reader, &aborter](auto) { return read_node_state(&limited_reader, aborter); })
        | ranges::to_vector;

    // Behaviour of StrCmpLogicalW can change e.g. with Windows updates, so re-sort after deserialising
    std::ranges::sort(
        state.children, [](auto&& left, auto&& right) { return StrCmpLogicalW(left, right) < 0; },
        [](auto& item) { return pfc::stringcvt::string_wide_from_utf8(item.name.data(), item.name.size()); });

    state.selected = limited_reader.read_lendian_t<bool>(aborter);
    limited_reader.flush_remaining(aborter);

    return state;
}

} // namespace alp
