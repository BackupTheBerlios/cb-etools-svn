#include "main.h"

//wxApplication
IMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame* frame = new MyFrame(0L, _("wx_TestIt"));
    frame->Show();
    return true;
}

//wxFrame
int idMenuQuit = wxNewId();
int idMenuAbout = wxNewId();
int idMenuTokenizer = wxNewId();
int idMenuScreenshot = wxNewId();

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(idMenuQuit, MyFrame::OnQuit)
    EVT_MENU(idMenuAbout, MyFrame::OnAbout)
    EVT_MENU(idMenuTokenizer, MyFrame::OnTokenizer)
    EVT_MENU(idMenuScreenshot, MyFrame::OnScreenshot)
END_EVENT_TABLE()

MyFrame::MyFrame(wxFrame *frame, const wxString& title)
        : wxFrame(frame, -1, title)
{
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tESC"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu* testitMenu = new wxMenu(_T(""));
    testitMenu->Append(idMenuTokenizer, _("Tokenizer\tT"), _(""));
    testitMenu->Append(idMenuScreenshot, _("Screenshot\tS"), _(""));
    mbar->Append(testitMenu, _("Test&It"));

    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuAbout, _("&About\tF1"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    SetMenuBar(mbar);

    // create a status bar with some information about the used wxWidgets version
    CreateStatusBar(1);
    SetStatusText(_("Hello Code::Blocks user !"),0);
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

void MyFrame::OnScreenshot(wxCommandEvent& event)
{
    int width =  1280+1024;
    int height = 1024;

    wxBitmap bitmap(width, height, -1);
    assert(bitmap.Ok());

    wxMemoryDC memDC;
    memDC.SelectObject(bitmap);

    int x = 0;
    int y = 0;
    //this->GetPosition(&x, &y);

    wxScreenDC screenDC;
    memDC.Blit(0, 0, width, height, &screenDC, x, y);

    wxString bmpFilename = _T("screenshot.bmp");
    bool succBmpSave = bitmap.SaveFile(bmpFilename, wxBITMAP_TYPE_BMP);
    assert(succBmpSave);

    wxString msg = _T("");
    msg += _T("Screenshot saved to file \"");
    msg += bmpFilename;
    msg += _T("\"");
    wxMessageBox(msg, _("Screenshot..."));

}

