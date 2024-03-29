#pragma once

class MetaBranchTitleformatHook : public titleformat_hook_impl_file_info {
public:
    typedef titleformat_hook_impl_file_info baseClass;
    MetaBranchTitleformatHook(const playable_location& p_location, const file_info* p_info)
        : titleformat_hook_impl_file_info(p_location, p_info)
    {
    }
    bool process_field(
        titleformat_text_out* p_out, const char* p_name, t_size p_name_length, bool& p_found_flag) override;
    bool process_function(titleformat_text_out* p_out, const char* p_name, t_size p_name_length,
        titleformat_hook_function_params* p_params, bool& p_found_flag) override;

protected:
    bool process_meta_branch(titleformat_text_out* p_out, t_size p_index);
};
