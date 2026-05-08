#pragma once

namespace alp {

void create_autoplaylist(bool is_by_dir, wil::zstring_view view_titleformat,
    std::vector<std::vector<wil::zstring_view>> node_labels, std::string_view filter);

}
