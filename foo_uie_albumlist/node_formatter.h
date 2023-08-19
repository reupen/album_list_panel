#pragma once

class NodeFormatter {
public:
    const wchar_t* format(const node_ptr& node)
    {
        m_buffer.clear();

        const auto parent = node->get_parent().lock();
        auto item_count = parent ? parent->get_num_children() : 0;

        const auto item_index = node->get_display_index();

        if ((!cfg_show_item_indices || item_count == 0) && !cfg_show_subitem_counts)
            return node->get_name_utf16();

        if (cfg_show_item_indices && item_index && parent) {
            uint32_t pad_digits = 0;

            while (item_count > 0) {
                item_count /= 10;
                pad_digits++;
            }

            format_to(std::back_inserter(m_buffer), L"{:0{}}. ", *item_index + 1, pad_digits);
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
