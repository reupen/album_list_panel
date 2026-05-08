#pragma once

namespace alp::utils {

void replace_substring(
    std::string_view input, std::string_view search, std::string_view replacement, std::string& result);
std::string replace_substring(std::string_view input, std::string_view search, std::string_view replacement);

} // namespace alp::utils
