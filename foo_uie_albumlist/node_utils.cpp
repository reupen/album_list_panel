#include "stdafx.h"

namespace alp {

NodeTracksHolder get_node_tracks(const std::vector<node_ptr>& nodes)
{
    return NodeTracksHolder(nodes);
}

std::vector<node_ptr> clean_selection(std::unordered_set<node_ptr> selection)
{
    std::vector<node_ptr> cleaned_selection;

    for (auto& node_ : selection) {
        if (!ranges::any_of(node_->get_parents(), [&selection](auto& parent) { return selection.contains(parent); }))
            cleaned_selection.emplace_back(node_);
    }

    ranges::sort(cleaned_selection, [](auto& left, auto& right) {
        if (left == right)
            return false;

        auto left_hierarchy = left->get_hierarchy();
        auto right_hierarchy = right->get_hierarchy();

        auto transformer = [](auto& node_) { return node_->get_display_index().value_or(0); };
        auto left_indices = left_hierarchy | ranges::views::transform(transformer);
        auto right_indices = right_hierarchy | ranges::views::transform(transformer);

        for (auto [index_left, index_right] : ranges::views::zip(left_indices, right_indices)) {
            if (index_left != index_right) {
                return index_left < index_right;
            }
        }

        return left_indices.size() < right_indices.size();
    });

    return cleaned_selection;
}

} // namespace alp
