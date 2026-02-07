/**
 * @file MainFrame.cpp
 * @brief Main frame window implementation
 *
 * Creates the primary IDM interface with toolbar, status bar,
 * splitter pane, category tree, and download list.
 */

#include "stdafx.h"
#include "MainFrame.h"
#include "DownloadListView.h"
#include "CategoryTreeView.h"
#include "AddUrlDialog.h"
#include "OptionsDialog.h"
#include "SchedulerDialog.h"
#include "BatchDownloadDialog.h"
#include "ProgressDialog.h"
#include "../core/DownloadEngine.h"
#include "../core/SpeedLimiter.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"
#include "../util/Registry.h"
#include "resource.h"

namespace idm {

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

// ─── Message Map ───────────────────────────────────────────────────────────
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_GETMINMAXINFO()
    
    // Custom messages
    ON_MESSAGE(WM_APP_TRAY_NOTIFY, &CMainFrame::OnTrayNotify)
    ON_MESSAGE(WM_APP_DOWNLOAD_PROGRESS, &CMainFrame::OnDownloadProgressMsg)
    ON_MESSAGE(WM_APP_CLIPBOARD_URL, &CMainFrame::OnClipboardUrl)
    ON_MESSAGE(WM_APP_THEME_CHANGED, &CMainFrame::OnThemeChanged)
    ON_MESSAGE(WM_APP_REFRESH_LIST, &CMainFrame::OnRefreshList)
    
    // Tasks menu
    ON_COMMAND(ID_TASKS_ADD_URL, &CMainFrame::OnTasksAddUrl)
    ON_COMMAND(ID_TASKS_ADD_BATCH, &CMainFrame::OnTasksAddBatch)
    ON_COMMAND(ID_TASKS_EXPORT, &CMainFrame::OnTasksExport)
    ON_COMMAND(ID_TASKS_IMPORT, &CMainFrame::OnTasksImport)
    ON_COMMAND(ID_TASKS_SITE_GRABBER, &CMainFrame::OnTasksSiteGrabber)
    ON_COMMAND(ID_TASKS_EXIT, &CMainFrame::OnFileExit)
    ON_COMMAND(ID_FILE_EXIT, &CMainFrame::OnFileExit)
    
    // Downloads menu
    ON_COMMAND(ID_DL_RESUME, &CMainFrame::OnDlResume)
    ON_COMMAND(ID_DL_RESUME_ALL, &CMainFrame::OnDlResumeAll)
    ON_COMMAND(ID_DL_STOP, &CMainFrame::OnDlStop)
    ON_COMMAND(ID_DL_STOP_ALL, &CMainFrame::OnDlStopAll)
    ON_COMMAND(ID_DL_DELETE, &CMainFrame::OnDlDelete)
    ON_COMMAND(ID_DL_DELETE_COMPLETED, &CMainFrame::OnDlDeleteCompleted)
    ON_COMMAND(ID_DL_DELETE_WITH_FILE, &CMainFrame::OnDlDeleteWithFile)
    ON_COMMAND(ID_DL_OPEN_FILE, &CMainFrame::OnDlOpenFile)
    ON_COMMAND(ID_DL_OPEN_FOLDER, &CMainFrame::OnDlOpenFolder)
    ON_COMMAND(ID_DL_PROPERTIES, &CMainFrame::OnDlProperties)
    ON_COMMAND(ID_DL_SELECT_ALL, &CMainFrame::OnDlSelectAll)
    
    // Queues menu
    ON_COMMAND(ID_QUEUE_SCHEDULER, &CMainFrame::OnQueueScheduler)
    ON_COMMAND(ID_QUEUE_START, &CMainFrame::OnQueueStart)
    ON_COMMAND(ID_QUEUE_STOP, &CMainFrame::OnQueueStop)
    
    // Options menu
    ON_COMMAND(ID_OPTIONS_SETTINGS, &CMainFrame::OnOptionsSettings)
    ON_COMMAND(ID_OPTIONS_SPEED_LIMITER, &CMainFrame::OnOptionsSpeedLimiter)
    
    // View menu
    ON_COMMAND(ID_VIEW_CATEGORIES, &CMainFrame::OnViewCategories)
    ON_COMMAND(ID_VIEW_TOOLBAR, &CMainFrame::OnViewToolbar)
    ON_COMMAND(ID_VIEW_DARK_MODE, &CMainFrame::OnViewDarkMode)
    
    // Help menu
    ON_COMMAND(ID_HELP_ABOUT, &CMainFrame::OnHelpAbout)
    
    // Tray commands
    ON_COMMAND(ID_TRAY_OPEN, &CMainFrame::OnTrayOpen)
    ON_COMMAND(ID_TRAY_ADD_URL, &CMainFrame::OnTasksAddUrl)
    ON_COMMAND(ID_TRAY_STOP_ALL, &CMainFrame::OnDlStopAll)
    ON_COMMAND(ID_TRAY_START_QUEUE, &CMainFrame::OnQueueStart)
    ON_COMMAND(ID_TRAY_STOP_QUEUE, &CMainFrame::OnQueueStop)
    ON_COMMAND(ID_TRAY_SCHEDULER, &CMainFrame::OnQueueScheduler)
    ON_COMMAND(ID_TRAY_EXIT, &CMainFrame::OnTrayExit)
    
    // Update handlers
    ON_UPDATE_COMMAND_UI(ID_DL_RESUME, &CMainFrame::OnUpdateDlResume)
    ON_UPDATE_COMMAND_UI(ID_DL_STOP, &CMainFrame::OnUpdateDlStop)
    ON_UPDATE_COMMAND_UI(ID_DL_DELETE, &CMainFrame::OnUpdateDlDelete)
END_MESSAGE_MAP()

// Status bar indicators
static UINT indicators[] = {
    ID_SEPARATOR,       // Status text
    ID_SEPARATOR,       // Speed
    ID_SEPARATOR,       // Active count
};

// ─── Construction ──────────────────────────────────────────────────────────
CMainFrame::CMainFrame() {
    auto settings = Registry::Instance().LoadSettings();
    m_darkMode = settings.darkMode;
}

CMainFrame::~CMainFrame() = default;

// ─── PreCreateWindow ───────────────────────────────────────────────────────
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
    if (!CFrameWnd::PreCreateWindow(cs)) return FALSE;
    
    // Set initial window size and style
    cs.cx = 900;
    cs.cy = 600;
    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    
    // Register custom window class
    WNDCLASS wc;
    if (::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wc)) {
        wc.lpszClassName = constants::APP_CLASS_NAME;
        wc.hIcon = AfxGetApp()->LoadIcon(IDI_APP_ICON);
        AfxRegisterClass(&wc);
        cs.lpszClass = constants::APP_CLASS_NAME;
    }
    
    return TRUE;
}

// ─── OnCreate ──────────────────────────────────────────────────────────────
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;
    
    // Set window title
    SetWindowText(constants::APP_TITLE);
    
    // Create UI components
    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar();
    
    // Set up system tray
    SetupSystemTray();
    
    // Set up timers
    SetupTimers();
    
    // Register as download engine observer
    DownloadEngine::Instance().AddObserver(this);
    
    // Enable drag & drop
    DragAcceptFiles(TRUE);
    
    // Apply dark mode if configured
    if (m_darkMode) {
        ApplyDarkMode();
    }
    
    // Set status bar initial text
    m_wndStatusBar.SetPaneText(0, L"Ready");
    m_wndStatusBar.SetPaneText(1, L"Speed: 0 B/s");
    m_wndStatusBar.SetPaneText(2, L"Active: 0");
    
    LOG_INFO(L"MainFrame: created successfully");
    return 0;
}

// ─── OnDestroy ─────────────────────────────────────────────────────────────
void CMainFrame::OnDestroy() {
    // Unregister from engine
    DownloadEngine::Instance().RemoveObserver(this);
    
    // Remove system tray icon
    RemoveSystemTray();
    
    // Kill timers
    KillTimer(TIMER_UI_REFRESH);
    KillTimer(TIMER_CLIPBOARD);
    KillTimer(TIMER_TRAY_ANIM);
    
    CFrameWnd::OnDestroy();
}

// ─── OnClose ───────────────────────────────────────────────────────────────
void CMainFrame::OnClose() {
    // Minimize to tray instead of closing (IDM behavior)
    if (DownloadEngine::Instance().GetActiveCount() > 0) {
        ShowWindow(SW_HIDE);
        ShowBalloonNotification(L"IDM Clone", 
            L"Download manager is still running in the system tray.");
        return;
    }
    
    // No active downloads - actually close
    CFrameWnd::OnClose();
}

// ─── OnSize ────────────────────────────────────────────────────────────────
void CMainFrame::OnSize(UINT nType, int cx, int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    
    if (nType == SIZE_MINIMIZED) {
        // Minimize to system tray
        ShowWindow(SW_HIDE);
        return;
    }
}

// ─── OnGetMinMaxInfo ───────────────────────────────────────────────────────
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI) {
    lpMMI->ptMinTrackSize.x = 640;
    lpMMI->ptMinTrackSize.y = 400;
    CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

// ─── Timer Handler ─────────────────────────────────────────────────────────
void CMainFrame::OnTimer(UINT_PTR nIDEvent) {
    switch (nIDEvent) {
        case TIMER_UI_REFRESH:
            // Refresh the download list view
            if (m_pListView) {
                m_pListView->Invalidate(FALSE);
            }
            break;
            
        case TIMER_CLIPBOARD: {
            // Check clipboard for URLs (IDM-style clipboard monitoring)
            if (::IsClipboardFormatAvailable(CF_UNICODETEXT)) {
                if (::OpenClipboard(GetSafeHwnd())) {
                    HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);
                    if (hData) {
                        LPCWSTR text = static_cast<LPCWSTR>(::GlobalLock(hData));
                        if (text) {
                            String clipText = text;
                            ::GlobalUnlock(hData);
                            
                            // Check if it looks like a URL
                            if (Unicode::IsHttpUrl(clipText) || 
                                Unicode::IsHttpsUrl(clipText) ||
                                Unicode::IsFtpUrl(clipText)) {
                                PostMessage(WM_APP_CLIPBOARD_URL, 0, 0);
                            }
                        }
                    }
                    ::CloseClipboard();
                }
            }
            break;
        }
    }
    
    CFrameWnd::OnTimer(nIDEvent);
}

// ─── Create Menu Bar ───────────────────────────────────────────────────────
void CMainFrame::CreateMenuBar() {
    // Create the main menu programmatically (matching IDM's menu structure)
    CMenu mainMenu;
    mainMenu.CreateMenu();
    
    // Tasks menu
    CMenu tasksMenu;
    tasksMenu.CreatePopupMenu();
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_ADD_URL, L"&Add URL...\tInsert");
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_ADD_BATCH, L"Add &Batch Download...");
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_SITE_GRABBER, L"&Site Grabber...");
    tasksMenu.AppendMenu(MF_SEPARATOR);
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_EXPORT, L"&Export...");
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_IMPORT, L"&Import...");
    tasksMenu.AppendMenu(MF_SEPARATOR);
    tasksMenu.AppendMenu(MF_STRING, ID_TASKS_EXIT, L"E&xit\tAlt+F4");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)tasksMenu.Detach(), L"&Tasks");
    
    // Downloads menu
    CMenu dlMenu;
    dlMenu.CreatePopupMenu();
    dlMenu.AppendMenu(MF_STRING, ID_DL_RESUME, L"&Resume\tEnter");
    dlMenu.AppendMenu(MF_STRING, ID_DL_RESUME_ALL, L"Resume &All");
    dlMenu.AppendMenu(MF_SEPARATOR);
    dlMenu.AppendMenu(MF_STRING, ID_DL_STOP, L"&Stop");
    dlMenu.AppendMenu(MF_STRING, ID_DL_STOP_ALL, L"Stop A&ll");
    dlMenu.AppendMenu(MF_SEPARATOR);
    dlMenu.AppendMenu(MF_STRING, ID_DL_DELETE, L"&Delete\tDel");
    dlMenu.AppendMenu(MF_STRING, ID_DL_DELETE_COMPLETED, L"Delete &Completed");
    dlMenu.AppendMenu(MF_STRING, ID_DL_DELETE_WITH_FILE, L"Delete with &File\tShift+Del");
    dlMenu.AppendMenu(MF_SEPARATOR);
    dlMenu.AppendMenu(MF_STRING, ID_DL_REDOWNLOAD, L"Re&download");
    dlMenu.AppendMenu(MF_SEPARATOR);
    dlMenu.AppendMenu(MF_STRING, ID_DL_OPEN_FILE, L"&Open File");
    dlMenu.AppendMenu(MF_STRING, ID_DL_OPEN_FOLDER, L"Open &Folder");
    dlMenu.AppendMenu(MF_SEPARATOR);
    dlMenu.AppendMenu(MF_STRING, ID_DL_SELECT_ALL, L"Select &All\tCtrl+A");
    dlMenu.AppendMenu(MF_STRING, ID_DL_PROPERTIES, L"&Properties...");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)dlMenu.Detach(), L"&Downloads");
    
    // Queues menu
    CMenu queuesMenu;
    queuesMenu.CreatePopupMenu();
    queuesMenu.AppendMenu(MF_STRING, ID_QUEUE_SCHEDULER, L"&Scheduler...");
    queuesMenu.AppendMenu(MF_SEPARATOR);
    queuesMenu.AppendMenu(MF_STRING, ID_QUEUE_START, L"S&tart Queue");
    queuesMenu.AppendMenu(MF_STRING, ID_QUEUE_STOP, L"Sto&p Queue");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)queuesMenu.Detach(), L"&Queues");
    
    // Options menu
    CMenu optMenu;
    optMenu.CreatePopupMenu();
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_SETTINGS, L"&Options...");
    optMenu.AppendMenu(MF_SEPARATOR);
    optMenu.AppendMenu(MF_STRING, ID_OPTIONS_SPEED_LIMITER, L"&Speed Limiter...");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)optMenu.Detach(), L"&Options");
    
    // View menu
    CMenu viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_CATEGORIES, L"&Categories Panel");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, L"&Toolbar");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING | (m_darkMode ? MF_CHECKED : 0), 
                        ID_VIEW_DARK_MODE, L"&Dark Mode");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.Detach(), L"&View");
    
    // Help menu
    CMenu helpMenu;
    helpMenu.CreatePopupMenu();
    helpMenu.AppendMenu(MF_STRING, ID_HELP_ABOUT, L"&About IDM Clone...");
    helpMenu.AppendMenu(MF_STRING, ID_HELP_CHECK_UPDATES, L"Check for &Updates...");
    mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)helpMenu.Detach(), L"&Help");
    
    SetMenu(&mainMenu);
    mainMenu.Detach();
}

// ─── Create Toolbar ────────────────────────────────────────────────────────
void CMainFrame::CreateToolBar() {
    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
            WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS)) {
        LOG_ERROR(L"Failed to create toolbar");
        return;
    }
    
    // Add toolbar buttons programmatically
    m_wndToolBar.SetButtons(nullptr, 11);
    
    m_wndToolBar.SetButtonInfo(0, ID_TASKS_ADD_URL, TBBS_BUTTON, 0);
    m_wndToolBar.SetButtonText(0, L"Add URL");
    
    m_wndToolBar.SetButtonInfo(1, ID_DL_RESUME, TBBS_BUTTON, 1);
    m_wndToolBar.SetButtonText(1, L"Resume");
    
    m_wndToolBar.SetButtonInfo(2, ID_DL_STOP, TBBS_BUTTON, 2);
    m_wndToolBar.SetButtonText(2, L"Stop");
    
    m_wndToolBar.SetButtonInfo(3, ID_DL_STOP_ALL, TBBS_BUTTON, 3);
    m_wndToolBar.SetButtonText(3, L"Stop All");
    
    m_wndToolBar.SetButtonInfo(4, ID_SEPARATOR, TBBS_SEPARATOR, 0);
    
    m_wndToolBar.SetButtonInfo(5, ID_DL_DELETE, TBBS_BUTTON, 4);
    m_wndToolBar.SetButtonText(5, L"Delete");
    
    m_wndToolBar.SetButtonInfo(6, ID_DL_DELETE_COMPLETED, TBBS_BUTTON, 5);
    m_wndToolBar.SetButtonText(6, L"Delete Finished");
    
    m_wndToolBar.SetButtonInfo(7, ID_SEPARATOR, TBBS_SEPARATOR, 0);
    
    m_wndToolBar.SetButtonInfo(8, ID_OPTIONS_SETTINGS, TBBS_BUTTON, 6);
    m_wndToolBar.SetButtonText(8, L"Options");
    
    m_wndToolBar.SetButtonInfo(9, ID_QUEUE_START, TBBS_BUTTON, 7);
    m_wndToolBar.SetButtonText(9, L"Start Queue");
    
    m_wndToolBar.SetButtonInfo(10, ID_QUEUE_STOP, TBBS_BUTTON, 8);
    m_wndToolBar.SetButtonText(10, L"Stop Queue");
    
    // Enable text labels on toolbar
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    
    // Set button sizes for text
    CSize btnSize(70, 40);
    CSize imgSize(16, 16);
    m_wndToolBar.SetSizes(btnSize, imgSize);
}

// ─── Create Status Bar ─────────────────────────────────────────────────────
void CMainFrame::CreateStatusBar() {
    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators, _countof(indicators))) {
        LOG_ERROR(L"Failed to create status bar");
        return;
    }
    
    m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);
    m_wndStatusBar.SetPaneInfo(1, ID_SEPARATOR, SBPS_NORMAL, 150);
    m_wndStatusBar.SetPaneInfo(2, ID_SEPARATOR, SBPS_NORMAL, 100);
}

// ─── Create Splitter Layout ────────────────────────────────────────────────
void CMainFrame::CreateSplitterLayout() {
    // This would be called from OnCreateClient in a full implementation
    // The splitter divides the client area into category tree (left) and 
    // download list (right)
}

// ─── System Tray ───────────────────────────────────────────────────────────
void CMainFrame::SetupSystemTray() {
    memset(&m_trayIconData, 0, sizeof(m_trayIconData));
    m_trayIconData.cbSize = sizeof(NOTIFYICONDATAW);
    m_trayIconData.hWnd = GetSafeHwnd();
    m_trayIconData.uID = 1;
    m_trayIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_trayIconData.uCallbackMessage = WM_APP_TRAY_NOTIFY;
    m_trayIconData.hIcon = AfxGetApp()->LoadIcon(IDI_APP_ICON);
    wcscpy_s(m_trayIconData.szTip, constants::APP_TITLE);
    
    ::Shell_NotifyIconW(NIM_ADD, &m_trayIconData);
    m_trayIconCreated = true;
}

void CMainFrame::RemoveSystemTray() {
    if (m_trayIconCreated) {
        ::Shell_NotifyIconW(NIM_DELETE, &m_trayIconData);
        m_trayIconCreated = false;
    }
}

void CMainFrame::ShowBalloonNotification(const String& title, const String& text) {
    m_trayIconData.uFlags |= NIF_INFO;
    wcscpy_s(m_trayIconData.szInfoTitle, title.c_str());
    wcscpy_s(m_trayIconData.szInfo, text.c_str());
    m_trayIconData.dwInfoFlags = NIIF_INFO;
    ::Shell_NotifyIconW(NIM_MODIFY, &m_trayIconData);
}

void CMainFrame::SetupTimers() {
    SetTimer(TIMER_UI_REFRESH, constants::UI_UPDATE_INTERVAL_MS, nullptr);
}

void CMainFrame::SetupClipboardMonitor() {
    auto settings = Registry::Instance().LoadSettings();
    if (settings.clipboardMonitoring) {
        SetTimer(TIMER_CLIPBOARD, 500, nullptr);
    }
}

// ─── Tray Notification Handler ─────────────────────────────────────────────
LRESULT CMainFrame::OnTrayNotify(WPARAM /*wParam*/, LPARAM lParam) {
    switch (lParam) {
        case WM_LBUTTONDBLCLK:
            OnTrayOpen();
            break;
            
        case WM_RBUTTONUP: {
            CPoint pt;
            ::GetCursorPos(&pt);
            
            CMenu trayMenu;
            trayMenu.CreatePopupMenu();
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_OPEN, L"&Open IDM Clone");
            trayMenu.AppendMenu(MF_SEPARATOR);
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_ADD_URL, L"&Add URL...");
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_STOP_ALL, L"&Stop All Downloads");
            trayMenu.AppendMenu(MF_SEPARATOR);
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_START_QUEUE, L"Start &Queue");
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_STOP_QUEUE, L"S&top Queue");
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_SCHEDULER, L"&Scheduler...");
            trayMenu.AppendMenu(MF_SEPARATOR);
            trayMenu.AppendMenu(MF_STRING, ID_TRAY_EXIT, L"E&xit");
            
            ::SetForegroundWindow(GetSafeHwnd());
            trayMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                                     pt.x, pt.y, this);
            PostMessage(WM_NULL, 0, 0);
            break;
        }
    }
    return 0;
}

// ─── Command Handlers ──────────────────────────────────────────────────────
void CMainFrame::OnTasksAddUrl() {
    CAddUrlDialog dlg(this);
    if (dlg.DoModal() == IDOK) {
        RefreshDownloadList();
    }
}

void CMainFrame::OnTasksAddBatch() {
    CBatchDownloadDialog dlg(this);
    dlg.DoModal();
}

void CMainFrame::OnTasksExport() {
    CFileDialog fileDlg(FALSE, L"txt", L"downloads.txt",
        OFN_OVERWRITEPROMPT, L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
    
    if (fileDlg.DoModal() == IDOK) {
        auto downloads = DownloadEngine::Instance().GetAllDownloads();
        std::wofstream file(fileDlg.GetPathName().GetString());
        for (const auto& dl : downloads) {
            file << dl.url << L"\n";
        }
        LOG_INFO(L"Exported %zu URLs to %s", downloads.size(), 
                 fileDlg.GetPathName().GetString());
    }
}

void CMainFrame::OnTasksImport() {
    CFileDialog fileDlg(TRUE, L"txt", nullptr, OFN_FILEMUSTEXIST,
        L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
    
    if (fileDlg.DoModal() == IDOK) {
        std::wifstream file(fileDlg.GetPathName().GetString());
        String line;
        int count = 0;
        while (std::getline(file, line)) {
            line = Unicode::Trim(line);
            if (Unicode::IsHttpUrl(line) || Unicode::IsHttpsUrl(line) || Unicode::IsFtpUrl(line)) {
                DownloadEngine::Instance().AddDownload(line, L"", L"", L"", L"", false);
                count++;
            }
        }
        LOG_INFO(L"Imported %d URLs", count);
        RefreshDownloadList();
    }
}

void CMainFrame::OnTasksSiteGrabber() {
    AfxMessageBox(L"Site Grabber will be available in Phase 5.", MB_ICONINFORMATION);
}

void CMainFrame::OnFileExit() {
    DownloadEngine::Instance().StopAll();
    DestroyWindow();
}

void CMainFrame::OnDlResume() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    for (const auto& id : selectedIds) {
        DownloadEngine::Instance().StartDownload(id);
    }
}

void CMainFrame::OnDlResumeAll() {
    DownloadEngine::Instance().ResumeAll();
}

void CMainFrame::OnDlStop() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    for (const auto& id : selectedIds) {
        DownloadEngine::Instance().PauseDownload(id);
    }
}

void CMainFrame::OnDlStopAll() {
    DownloadEngine::Instance().StopAll();
}

void CMainFrame::OnDlDelete() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    if (selectedIds.empty()) return;
    
    CString msg;
    msg.Format(L"Delete %d selected download(s)?", static_cast<int>(selectedIds.size()));
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) == IDYES) {
        for (const auto& id : selectedIds) {
            DownloadEngine::Instance().RemoveDownload(id, false);
        }
        RefreshDownloadList();
    }
}

void CMainFrame::OnDlDeleteCompleted() {
    int count = DownloadEngine::Instance().RemoveCompleted();
    RefreshDownloadList();
    LOG_INFO(L"Removed %d completed downloads", count);
}

void CMainFrame::OnDlDeleteWithFile() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    if (selectedIds.empty()) return;
    
    if (AfxMessageBox(L"Delete selected download(s) and their files?",
                      MB_YESNO | MB_ICONWARNING) == IDYES) {
        for (const auto& id : selectedIds) {
            DownloadEngine::Instance().RemoveDownload(id, true);
        }
        RefreshDownloadList();
    }
}

void CMainFrame::OnDlOpenFile() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    if (selectedIds.empty()) return;
    
    auto entry = DownloadEngine::Instance().GetDownload(selectedIds[0]);
    if (entry.has_value() && entry->status == DownloadStatus::Complete) {
        ::ShellExecuteW(GetSafeHwnd(), L"open", entry->FullPath().c_str(),
                        nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void CMainFrame::OnDlOpenFolder() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    if (selectedIds.empty()) return;
    
    auto entry = DownloadEngine::Instance().GetDownload(selectedIds[0]);
    if (entry.has_value()) {
        String path = entry->FullPath();
        String param = L"/select,\"" + path + L"\"";
        ::ShellExecuteW(GetSafeHwnd(), nullptr, L"explorer.exe",
                        param.c_str(), nullptr, SW_SHOWNORMAL);
    }
}

void CMainFrame::OnDlProperties() {
    if (!m_pListView) return;
    auto selectedIds = m_pListView->GetSelectedDownloadIds();
    if (!selectedIds.empty()) {
        auto entry = DownloadEngine::Instance().GetDownload(selectedIds[0]);
        if (entry.has_value()) {
            // Show progress/info dialog for the download
            CProgressDialog dlg(entry.value(), this);
            dlg.DoModal();
        }
    }
}

void CMainFrame::OnDlSelectAll() {
    if (m_pListView) {
        m_pListView->SelectAll();
    }
}

void CMainFrame::OnQueueScheduler() {
    CSchedulerDialog dlg(this);
    dlg.DoModal();
}

void CMainFrame::OnQueueStart() {
    // Start the main download queue
    auto entries = DownloadEngine::Instance().GetAllDownloads();
    for (const auto& entry : entries) {
        if (entry.status == DownloadStatus::Queued) {
            DownloadEngine::Instance().StartDownload(entry.id);
        }
    }
}

void CMainFrame::OnQueueStop() {
    DownloadEngine::Instance().StopAll();
}

void CMainFrame::OnOptionsSettings() {
    COptionsDialog dlg(this);
    dlg.DoModal();
}

void CMainFrame::OnOptionsSpeedLimiter() {
    // Simple speed limiter dialog
    auto& limiter = SpeedLimiter::Instance();
    bool isActive = limiter.IsActive();
    
    if (isActive) {
        limiter.Enable(false);
        AfxMessageBox(L"Speed limiter disabled.", MB_ICONINFORMATION);
    } else {
        // Ask for speed limit
        CString prompt;
        prompt.Format(L"Enter speed limit in KB/s (current: %s):",
            isActive ? L"active" : L"disabled");
        
        // Simple input - in full implementation this would be a proper dialog
        limiter.SetLimit(512 * 1024);  // Default 512 KB/s
        limiter.Enable(true);
        AfxMessageBox(L"Speed limiter enabled: 512 KB/s", MB_ICONINFORMATION);
    }
}

void CMainFrame::OnViewCategories() {
    // Toggle category panel visibility
    // In full implementation, this would show/hide the left splitter pane
}

void CMainFrame::OnViewToolbar() {
    ShowControlBar(&m_wndToolBar, !m_wndToolBar.IsWindowVisible(), FALSE);
}

void CMainFrame::OnViewDarkMode() {
    m_darkMode = !m_darkMode;
    
    // Save preference
    Registry::Instance().WriteBool(L"Options", L"DarkMode", m_darkMode);
    
    ApplyDarkMode();
    
    // Update menu checkmark
    CMenu* pMenu = GetMenu();
    if (pMenu) {
        CMenu* pViewMenu = pMenu->GetSubMenu(4); // View menu
        if (pViewMenu) {
            pViewMenu->CheckMenuItem(ID_VIEW_DARK_MODE, 
                m_darkMode ? MF_CHECKED : MF_UNCHECKED);
        }
    }
}

void CMainFrame::OnHelpAbout() {
    CString aboutText;
    aboutText.Format(
        L"IDM Clone - Internet Download Manager\n"
        L"Version %s\n\n"
        L"A faithful recreation of Internet Download Manager\n"
        L"built with C++17 and MFC.\n\n"
        L"Features:\n"
        L"  - Dynamic file segmentation\n"
        L"  - Multi-threaded download acceleration\n"
        L"  - HTTP/HTTPS/FTP support\n"
        L"  - Resume capability\n"
        L"  - Download scheduling & queuing\n\n"
        L"Built with Visual Studio 2022, Win32 API, MFC, WinHTTP",
        constants::APP_VERSION);
    
    AfxMessageBox(aboutText, MB_OK | MB_ICONINFORMATION);
}

void CMainFrame::OnTrayOpen() {
    ShowWindow(SW_RESTORE);
    SetForegroundWindow();
}

void CMainFrame::OnTrayExit() {
    DownloadEngine::Instance().StopAll();
    DestroyWindow();
}

// ─── Update Handlers ───────────────────────────────────────────────────────
void CMainFrame::OnUpdateDlResume(CCmdUI* pCmdUI) {
    pCmdUI->Enable(m_pListView && !m_pListView->GetSelectedDownloadIds().empty());
}

void CMainFrame::OnUpdateDlStop(CCmdUI* pCmdUI) {
    pCmdUI->Enable(DownloadEngine::Instance().GetActiveCount() > 0);
}

void CMainFrame::OnUpdateDlDelete(CCmdUI* pCmdUI) {
    pCmdUI->Enable(m_pListView && !m_pListView->GetSelectedDownloadIds().empty());
}

// ─── OnCmdMsg ──────────────────────────────────────────────────────────────
BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, 
                           AFX_CMDHANDLERINFO* pHandlerInfo) {
    // Route commands to active views first
    if (m_pListView && m_pListView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;
    
    return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

// ─── Observer Callbacks (called from download threads) ─────────────────────
void CMainFrame::OnDownloadAdded(const String& id) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
}

void CMainFrame::OnDownloadStarted(const String& id) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
}

void CMainFrame::OnDownloadProgress(const String& id, int64 downloaded, 
                                     int64 total, double speed) {
    // Post message to UI thread for safe update
    PostMessage(WM_APP_DOWNLOAD_PROGRESS, 0, 0);
}

void CMainFrame::OnDownloadComplete(const String& id) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
    
    // Show balloon notification
    auto entry = DownloadEngine::Instance().GetDownload(id);
    if (entry.has_value()) {
        ShowBalloonNotification(L"Download Complete", entry->fileName);
    }
}

void CMainFrame::OnDownloadError(const String& id, const String& error) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
}

void CMainFrame::OnDownloadPaused(const String& id) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
}

void CMainFrame::OnDownloadRemoved(const String& id) {
    PostMessage(WM_APP_REFRESH_LIST, 0, 0);
}

void CMainFrame::OnSpeedUpdate(double totalSpeed, int activeCount) {
    // Update status bar (must be on UI thread)
    if (GetSafeHwnd()) {
        CString speedText;
        speedText.Format(L"Speed: %s", Unicode::FormatSpeed(totalSpeed).c_str());
        
        CString activeText;
        activeText.Format(L"Active: %d", activeCount);
        
        m_wndStatusBar.SetPaneText(1, speedText);
        m_wndStatusBar.SetPaneText(2, activeText);
    }
}

// ─── Message Handlers ──────────────────────────────────────────────────────
LRESULT CMainFrame::OnDownloadProgressMsg(WPARAM, LPARAM) {
    if (m_pListView) m_pListView->UpdateProgress();
    return 0;
}

LRESULT CMainFrame::OnClipboardUrl(WPARAM, LPARAM) {
    auto settings = Registry::Instance().LoadSettings();
    if (settings.clipboardMonitoring && settings.showInfoDialog) {
        OnTasksAddUrl();
    }
    return 0;
}

LRESULT CMainFrame::OnThemeChanged(WPARAM, LPARAM) {
    ApplyDarkMode();
    return 0;
}

LRESULT CMainFrame::OnRefreshList(WPARAM, LPARAM) {
    RefreshDownloadList();
    return 0;
}

void CMainFrame::RefreshDownloadList() {
    if (m_pListView) {
        m_pListView->RefreshFromDatabase();
    }
}

// ─── Dark Mode ─────────────────────────────────────────────────────────────
void CMainFrame::SetDarkMode(bool dark) {
    m_darkMode = dark;
    ApplyDarkMode();
}

void CMainFrame::ApplyDarkMode() {
    // Windows 10/11 dark mode API
    // Use DwmSetWindowAttribute to enable dark title bar
    BOOL darkMode = m_darkMode ? TRUE : FALSE;
    
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    
    HRESULT hr = ::DwmSetWindowAttribute(GetSafeHwnd(), 
        DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    
    if (FAILED(hr)) {
        ::DwmSetWindowAttribute(GetSafeHwnd(),
            DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &darkMode, sizeof(darkMode));
    }
    
    // Invalidate to redraw
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

} // namespace idm
