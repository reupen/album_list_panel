#include "stdafx.h"
#include "tree_view_populator.h"
#include "node.h"

//TODO: node name as field

DECLARE_COMPONENT_VERSION("Album list panel",
                          
                          "0.4.0-dev",
                          
                          "allows you to browse through your media library\n\n"
                          "based upon albumlist 3.1.0\n"
                          "compiled: " __DATE__ "\n"
                          "with Columns UI SDK version: " UI_EXTENSION_VERSION
                          
                          );



const char * directory_structure_view_name = "by directory structure";

class album_list_window;

static string8_fastalloc g_formatbuf;

ptr_list_t<album_list_window> album_list_window::s_instances;
HFONT album_list_window::s_font = nullptr;

void album_list_window::g_update_all_fonts()
{
    if (s_font!=nullptr)
    {
        unsigned n, count = album_list_window::s_instances.get_count();
        for (n=0; n<count; n++)
        {
            HWND wnd = album_list_window::s_instances[n]->m_wnd_tv;
            if (wnd) uSendMessage(wnd,WM_SETFONT,(WPARAM)0,MAKELPARAM(0,0));
            wnd = album_list_window::s_instances[n]->m_wnd_edit;
            if (wnd) uSendMessage(wnd,WM_SETFONT,(WPARAM)0,MAKELPARAM(0,0));
        }
        DeleteObject(s_font);
    }

    s_font = cui::fonts::helper(g_guid_album_list_font).get_font();

    unsigned n, count = album_list_window::s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = album_list_window::s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            uSendMessage(wnd,WM_SETFONT,(WPARAM)s_font,MAKELPARAM(1,0));
            if (cfg_use_custom_indent)
                TreeView_SetIndent(wnd, cfg_indent);
            wnd = album_list_window::s_instances[n]->m_wnd_edit;
            if (wnd) 
            {
                uSendMessage(wnd,WM_SETFONT,(WPARAM)s_font,MAKELPARAM(1,0));
                album_list_window::s_instances[n]->on_size();
            }
        }
    }
}

album_list_window::~album_list_window()
{
    if (m_initialised) 
    {
        s_instances.remove_item(this);
        m_initialised = false;
    }
}

void album_list_window::update_all_window_frames()
{
    unsigned n, count = s_instances.get_count();
    long flags = 0;
    if (cfg_frame == 1) flags |= WS_EX_CLIENTEDGE;
    if (cfg_frame == 2) flags |= WS_EX_STATICEDGE;
    
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd)
        {
            uSetWindowLong(wnd, GWL_EXSTYLE, flags);
            SetWindowPos(wnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        }
        /*wnd = list_wnd[n]->wnd_edit;
        if (wnd)
        {
            uSetWindowLong(wnd, GWL_EXSTYLE, flags);
            SetWindowPos(wnd,0,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        }*/
    }
}

void album_list_window::update_all_colours()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->update_colours();
        }
    }

}

void album_list_window::g_update_all_labels()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->update_all_labels();
        }
    }

}

void album_list_window::g_update_all_showhscroll()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->destroy_tree();
            s_instances[n]->create_tree();
            s_instances[n]->on_size();
            /*
            SetWindowLongPtr(wnd, GWL_STYLE, (cfg_hscroll ? NULL : TVS_NOHSCROLL ) | (GetWindowLongPtr(wnd, GWL_STYLE) &~(TVS_NOHSCROLL )) );
            SetWindowPos(wnd, NULL, 0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
            RECT rc;
            GetClientRect(wnd, &rc);
            SendMessage(wnd, WM_SIZE, NULL, MAKELPARAM(rc.right, rc.bottom));
            */
        }
    }

}

void album_list_window::g_on_view_script_change(const char * p_view_before, const char * p_view)
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->on_view_script_change(p_view_before, p_view);
        }
    }

}

void album_list_window::g_refresh_all()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->refresh_tree();
        }
    }

}
void album_list_window::update_all_item_heights()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->m_wnd_tv;
        if (wnd) 
        {
            s_instances[n]->update_item_height();
        }
    }

}

/*void album_list_window::update_all_heights()
{
            
    unsigned n, count = list_wnd.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = list_wnd[n]->get_wnd();
        if (wnd) 
        {
            ui_extension::window_host_ptr host = list_wnd[n]->get_host();
            if (host.is_valid()) 
            {
                host->on_size_limit_change(wnd, ui_extension::SLC_MIN_HEIGHT);
            }
        }
    }
}*/

void album_list_window::update_all_indents()
{
            
    unsigned n, count = s_instances.get_count();
    for (n=0; n<count; n++)
    {
        HWND wnd = s_instances[n]->get_wnd();
        if (wnd) 
        {
            TreeView_SetIndent(s_instances[n]->m_wnd_tv, cfg_use_custom_indent ? cfg_indent : s_instances[n]->m_indent_default);
        }
    }
}

void album_list_window::on_view_script_change(const char * p_view_before, const char * p_view)
{
    if (get_wnd())
    {
        if (!stricmp_utf8(p_view_before, m_view))
        {
            m_view = p_view;
            refresh_tree();
        }
    }
}

void album_list_window::update_all_labels()
{
    if (m_root.is_valid())
    {
        m_root->mark_all_labels_dirty();
        uSendMessage(m_wnd_tv,WM_SETREDRAW,FALSE,0);
        {
            TRACK_CALL_TEXT("album_list_panel_setup_tree");
            TreeViewPopulator::s_setup_tree(m_wnd_tv,TVI_ROOT,m_root,0,0,nullptr);
        }
        uSendMessage(m_wnd_tv,WM_SETREDRAW,TRUE,0);
    }
}

void album_list_window::update_colours()
{
    cui::colours::helper p_colours(g_guid_album_list_colours);
    if (p_colours.get_themed()) uih::tree_view_set_explorer_theme(m_wnd_tv);
    else uih::tree_view_remove_explorer_theme(m_wnd_tv);

    TreeView_SetBkColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_background));
    TreeView_SetLineColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_active_item_frame));
    TreeView_SetTextColor(m_wnd_tv, p_colours.get_colour(cui::colours::colour_text));
    
}

void album_list_window::update_item_height()
{
    const auto font = reinterpret_cast<HFONT>(uSendMessage(m_wnd_tv, WM_GETFONT, 0, 0));
    int font_height = -1;
    if (cfg_custom_item_height)
    {
        font_height = uGetFontHeight(font) + cfg_item_height; 
        if (font_height < 1) font_height = 1;
    }
    TreeView_SetItemHeight(m_wnd_tv, font_height);
}

void album_list_window::on_task_completion(t_uint32 task, t_uint32 code)
{
    if (task == 0)
        refresh_tree();
}

void TreeView_CollapseOtherNodes(HWND wnd, HTREEITEM ti)
{
    HTREEITEM child = ti;
    do
    {
        HTREEITEM sibling = child;
        while (sibling = TreeView_GetPrevSibling(wnd, sibling))
        {
            //if (child != sibling)
                TreeView_Expand(wnd, sibling, TVE_COLLAPSE);
        }
        sibling = child;
        while (sibling = TreeView_GetNextSibling(wnd, sibling))
        {
            //if (child != sibling)
                TreeView_Expand(wnd, sibling, TVE_COLLAPSE);
        }
    }
    while (child = TreeView_GetParent(wnd, child));
}

void album_list_window::create_or_destroy_filter()
{
    if (m_filter)
        create_filter();
    else
        destroy_filter();
    on_size();
}

void album_list_window::create_filter()
{
    if (m_filter && !m_wnd_edit)
    {
        long flags = WS_EX_CLIENTEDGE;//0;
        /*if (cfg_frame == 1) flags |= WS_EX_CLIENTEDGE;
        else if (cfg_frame == 2) flags |= WS_EX_STATICEDGE;*/
        m_wnd_edit = CreateWindowEx(flags, WC_EDIT, _T(""),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0,
            get_wnd(), HMENU(IDC_FILTER), core_api::get_my_instance(), nullptr);
        //SetWindowTheme(wnd_edit, L"SearchBoxEdit", NULL);
        uSendMessage(m_wnd_edit,WM_SETFONT,(WPARAM)s_font,MAKELPARAM(0,0));
        SetFocus(m_wnd_edit);
        Edit_SetCueBannerText(m_wnd_edit, L"Search");
    }
}

void album_list_window::destroy_filter()
{
    if (m_wnd_edit)
    {
        bool b_was_focused = GetFocus() == m_wnd_edit;
        DestroyWindow(m_wnd_edit);
        m_wnd_edit=nullptr;
        if (m_wnd_tv)
        {
            if (m_populated) refresh_tree();
            if (b_was_focused) SetFocus(m_wnd_tv);
        }
    }
    m_filter_ptr.release();
}

void album_list_window::on_size(unsigned cx, unsigned cy)
{
    HDWP dwp = BeginDeferWindowPos(2);
    unsigned edit_height = m_wnd_edit ? uGetFontHeight(s_font) + 4: 0;
    unsigned tv_height = edit_height<cy?cy-edit_height:cy;
    unsigned edit_cx = 0;
    dwp = DeferWindowPos(dwp, m_wnd_tv, nullptr, 0, 0, cx, tv_height, SWP_NOZORDER);
    if (m_wnd_edit)
    dwp = DeferWindowPos(dwp, m_wnd_edit, nullptr, edit_cx, tv_height, max(cx-edit_cx,0), edit_height, SWP_NOZORDER);
    EndDeferWindowPos(dwp);
}

void album_list_window::on_size()
{
    RECT rc;
    GetClientRect(get_wnd(), &rc);
    on_size(rc.right, rc.bottom);
}

void album_list_window::create_tree()
{
    HWND wnd = get_wnd();

    long flags = 0;
    if (cfg_frame == 1) flags |= WS_EX_CLIENTEDGE;
    else if (cfg_frame == 2) flags |= WS_EX_STATICEDGE;

    m_wnd_tv = CreateWindowEx(flags, WC_TREEVIEW, _T("Album list"),
        TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT  | (cfg_hscroll ? 0 : TVS_NOHSCROLL ) | WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0,
        wnd, HMENU(IDC_TREE), core_api::get_my_instance(), nullptr);
    
    if (m_wnd_tv)
    {
        if (mmh::is_windows_vista_or_newer())
            TreeView_SetExtendedStyle(m_wnd_tv, TVS_EX_AUTOHSCROLL, TVS_EX_AUTOHSCROLL);
        if (cui::colours::helper(g_guid_album_list_colours).get_themed()) uih::tree_view_set_explorer_theme(m_wnd_tv);
        //SendMessage(wnd, TV_FIRST + 44, 0x0002, 0x0002);
        
        m_indent_default =  TreeView_GetIndent(m_wnd_tv);

        if (s_font)
        {
            uSendMessage(m_wnd_tv,WM_SETFONT,(WPARAM)s_font,MAKELPARAM(0,0));
            if (cfg_use_custom_indent)
                TreeView_SetIndent(wnd, cfg_indent);
        }
        else
            g_update_all_fonts();


        if (cfg_custom_item_height)
            update_item_height();

        update_colours();

        uSetWindowLong(m_wnd_tv,GWL_USERDATA,(LPARAM)(this));
        m_treeproc = (WNDPROC)uSetWindowLong(m_wnd_tv,GWL_WNDPROC,(LPARAM)(hook_proc));

        if (m_populated)
            refresh_tree();
    }
}

void album_list_window::destroy_tree()
{
    if (m_wnd_tv)
    {
        DestroyWindow(m_wnd_tv);
        m_wnd_tv=nullptr;
    }
}



void album_list_window::get_config(stream_writer * p_writer, abort_callback & p_abort) const
{
    p_writer->write_string(m_view, p_abort);
    p_writer->write_lendian_t(m_filter, p_abort);
}






void album_list_window::get_name(string_base & out)const
{
    out.set_string("Album list");
}
void album_list_window::get_category(string_base & out)const
{
    out.set_string("Panels");
}

void album_list_window::set_config(stream_reader * p_reader, t_size psize, abort_callback & p_abort)
{
    if (psize) 
    {
        p_reader->read_string(m_view, p_abort);
        try
        {
            p_reader->read_lendian_t(m_filter, p_abort);
        }
        catch (exception_io_data_truncation &) {};
    }
}


// {606E9CDD-45EE-4c3b-9FD5-49381CEBE8AE}
const GUID album_list_window::s_extension_guid = 
{ 0x606e9cdd, 0x45ee, 0x4c3b, { 0x9f, 0xd5, 0x49, 0x38, 0x1c, 0xeb, 0xe8, 0xae } };

ui_extension::window_factory<album_list_window> blah;


