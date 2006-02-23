#include "main.h"

int idMenuQuit = wxNewId();
int idMenuAbout = wxNewId();
int idMenuTokenizer = wxNewId();

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(idMenuQuit, MyFrame::OnQuit)
    EVT_MENU(idMenuAbout, MyFrame::OnAbout)
    EVT_MENU(idMenuTokenizer, MyFrame::OnTokenizer)
END_EVENT_TABLE()

MyFrame::MyFrame(wxFrame *frame, const wxString& title)
        : wxFrame(frame, -1, title)
{
#if wxUSE_MENUS
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tESC"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu* testitMenu = new wxMenu(_T(""));
    testitMenu->Append(idMenuTokenizer, _("&Tokenizer\tT"), _(""));
    mbar->Append(testitMenu, _("Test&It"));

    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuAbout, _("&About\tF1"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    SetMenuBar(mbar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar with some information about the used wxWidgets version
    CreateStatusBar(1);
    SetStatusText(_("Hello Code::Blocks user !"),0);
#endif // wxUSE_STATUSBAR
}

MyFrame::~MyFrame()
{}

void MyFrame::OnQuit(wxCommandEvent& event)
{
    Close();
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxString msg = _T("wxWidgets project to test several small code fragments");
    wxMessageBox(msg, _("Welcome to..."));
}

void MyFrame::OnTokenizer(wxCommandEvent& event)
{
    wxString msg;
    wxStringTokenizer tkz(wxT("1 2 3 4 5"), wxT(" "));
    while ( tkz.HasMoreTokens() )
    {
        wxString strNumber = tkz.GetNextToken();

        // convert token to number
        long nNumber;
        strNumber.ToLong(&nNumber);

        // You can also use wxAtoi
        nNumber = wxAtoi(strNumber);
        msg.Printf(_T("%s%03d,"),
                    msg.c_str(),
                    nNumber);
    }
    wxMessageBox(msg, _("Tokenizer..."));
}


//wxApplication
IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame* frame = new MyFrame(0L, _("wx_TestIt"));
    frame->Show();
    return true;
}
