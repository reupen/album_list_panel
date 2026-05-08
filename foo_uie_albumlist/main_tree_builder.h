#pragma once

namespace alp {

constexpr auto branch_marker = '\4';
constexpr auto branch_delimiter = '\5';

constexpr auto vertical_bar = "|"sv;
constexpr auto vertical_bar_replacement = "\uEFA0"sv;

constexpr auto path_separators = "\\|/"sv;

struct BranchSegment {
    size_t m_choices_begin{};
    size_t m_choices_end{};
    size_t m_current_choice{};
};

struct BranchChoice {
    size_t m_start{};
    size_t m_end{};
};

struct Branches {
    size_t branch_count{1};
    std::vector<BranchSegment> segments;
    std::vector<BranchChoice> choices;
};

class VerticalBarTitleformatTextFilter : public titleformat_text_filter {
public:
    void write(const GUID& p_inputtype, pfc::string_receiver& p_out, const char* p_data, t_size p_data_length) override
    {
        // titleformat_text_filter_impl_reserved_chars only filters for titleformat_inputtypes::meta
        if (p_inputtype != titleformat_inputtypes::meta) {
            p_out.add_string(p_data, p_data_length);
            return;
        }

        const size_t real_length = strnlen(p_data, p_data_length);

        const std::string_view input{p_data, real_length};
        size_t start{};

        while (start < real_length) {
            const size_t end = input.find(vertical_bar, start);

            if (end == std::string_view::npos) {
                p_out.add_string(p_data + start, p_data_length - start);
                break;
            }

            p_out.add_string(p_data + start, end - start);
            p_out.add_string(vertical_bar_replacement.data(), vertical_bar_replacement.size());
            start = end + 1;
        }
    }
};

void unescape_vertical_bar(std::string_view text, std::string& result);
std::string unescape_vertical_bar(std::string_view text);
Branches get_branches(std::string_view text);

} // namespace alp
