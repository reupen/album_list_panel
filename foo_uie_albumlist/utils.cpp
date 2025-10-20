#include "stdafx.h"

#include "utils.h"

namespace alp::utils {

std::string replace_substring(std::string_view input, std::string_view search, std::string_view replacement)
{
    std::string result;
    size_t fragment_start{};

    while (true) {
        const auto fragment_end = input.find(search, fragment_start);
        if (fragment_end == std::string_view::npos) {
            result.append(input.substr(fragment_start));
            break;
        }

        result.append(input.substr(fragment_start, fragment_end - fragment_start));
        result.append(replacement);

        fragment_start = fragment_end + search.length();
    }

    return result;
}

} // namespace alp::utils
