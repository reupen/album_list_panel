#pragma once

class NodeFormatter {
public:
    const wchar_t* format(const node_ptr& node, size_t item_index, size_t item_count)
    {
        m_buffer.clear();

        if ((!cfg_show_item_indices || item_count == 0) && !cfg_show_subitem_counts)
            return {node->get_name_utf16()};

        if (cfg_show_item_indices && item_count > 0) {
            uint32_t pad_digits = 0;

            while (item_count > 0) {
                item_count /= 10;
                pad_digits++;
            }
            format_to(std::back_inserter(m_buffer), L"{:0{}}. ", item_index + 1, pad_digits);
        }

        format_to(std::back_inserter(m_buffer), L"{}", node->get_name_utf16());

        if (cfg_show_subitem_counts && node->get_num_children())
            format_to(std::back_inserter(m_buffer), L" ({})", node->get_num_children());

        m_buffer.push_back(0);

        return m_buffer.data();
    }

private:
    fmt::basic_memory_buffer<wchar_t> m_buffer;
};
