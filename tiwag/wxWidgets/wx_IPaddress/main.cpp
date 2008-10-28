#include "main.h"

//wxApplication
IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame* frame = new MyFrame(0L, _("wxWidgets Application Template"));
    frame->Show();
    return true;
}

int idMenuQuit = wxNewId();
int idMenuAbout = wxNewId();
int idMenuMyStuff = wxNewId();

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(idMenuQuit, MyFrame::OnQuit)
    EVT_MENU(idMenuAbout, MyFrame::OnAbout)
    EVT_MENU(idMenuMyStuff, MyFrame::OnMyIP)
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

    wxMenu* mystuffMenu = new wxMenu(_T(""));
    mystuffMenu->Append(idMenuMyStuff, _("&IP address\tI"), _("Get Hostanmes and IP addresses ..."));
    mbar->Append(mystuffMenu, _("&My Stuff"));

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
{
}

void MyFrame::OnQuit(wxCommandEvent& event)
{
    Close();
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxString title = _T("Welcome to...");
    wxString msg = _T("... this wonderful program !");
    wxMessageBox(msg,title);
}

void MyFrame::OnMyIP(wxCommandEvent& event)
{
    wxIPV4address addr;
    wxString myhn = addr.Hostname();    // get my Hostname
    wxString addr2hn;
    bool res;
    res = addr.Hostname(myhn);          // set addr to my Hostname
    addr2hn = (res)? _T("true")         // check result of assignment
                   : _T("false");
    wxString myip = addr.IPAddress();   // get IP-address of my Hostname

    wxString othn = _T("www.chauvin-arnoux.com");   // get another Hostname
    res = addr.Hostname(othn);          // set addr to other Hostname
    wxString otip;
    otip = (res)? addr.IPAddress()      // get IP-address of other Hostname
                : _T("Host not found");

    wxString msg = _T("\n");
    msg += _T("get my Hostname         \t myhn    \t = ") + myhn    + _T("\t\n");
    msg += _T("set addr to my Hostname \t addr2hn \t = ") + addr2hn + _T("\t\n");
    msg += _T("get my IP address       \t myip    \t = ") + myip    + _T("\t\n");
    msg += _T("\n");
    msg += _T("get other Hostname      \t othn    \t = ") + othn    + _T("\t\n");
    msg += _T("get other IP address    \t otip    \t = ") + otip    + _T("\t\n");

    wxMessageBox(msg, _T("IPaddress"));
}

