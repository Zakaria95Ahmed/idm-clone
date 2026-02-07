/**
 * @file IDMApp.cpp
 * @brief Application entry point and initialization
 *
 * Startup sequence:
 *   1. Check for existing instance (single-instance enforcement)
 *   2. Initialize COM subsystem
 *   3. Initialize common controls (visual styles)
 *   4. Initialize the logger
 *   5. Load application settings from registry
 *   6. Initialize the download engine
 *   7. Create and show the main frame window
 *   8. Process command-line arguments (URLs from browser integration)
 *   9. Enter the MFC message loop
 *
 * Shutdown sequence:
 *   1. Stop all active downloads
 *   2. Save pending database changes
 *   3. Save settings to registry
 *   4. Shut down the download engine
 *   5. Shut down the logger
 *   6. Release the single-instance mutex
 *   7. Uninitialize COM
 */

#include "stdafx.h"
#include "IDMApp.h"
#include "ui/MainFrame.h"
#include "core/DownloadEngine.h"
#include "util/Logger.h"
#include "util/Registry.h"
#include "util/Unicode.h"

namespace idm {

// ─── The one and only CIDMApp object ───────────────────────────────────────
CIDMApp theApp;

// ─── Construction ──────────────────────────────────────────────────────────
CIDMApp::CIDMApp() {
    // Enable visual styles (common controls 6.0)
    // This gives us the modern Windows look for all controls
    SetAppID(L"IDMClone.DownloadManager.1");
}

CIDMApp::~CIDMApp() = default;

// ─── InitInstance ──────────────────────────────────────────────────────────
BOOL CIDMApp::InitInstance() {
    // ── Step 1: Single instance check ──────────────────────────────────
    if (!CheckSingleInstance()) {
        // Another instance is running - activate it and exit
        HWND hExisting = ::FindWindowW(constants::APP_CLASS_NAME, nullptr);
        if (hExisting) {
            ::ShowWindow(hExisting, SW_RESTORE);
            ::SetForegroundWindow(hExisting);
            
            // Forward command-line URL to existing instance
            // (In full implementation, use WM_COPYDATA or COM)
        }
        return FALSE;
    }
    
    // ── Step 2: Initialize COM ─────────────────────────────────────────
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        AfxMessageBox(L"Failed to initialize COM subsystem.", MB_ICONERROR);
        return FALSE;
    }
    
    // ── Step 3: Initialize MFC and common controls ─────────────────────
    // Enable visual styles
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | 
                ICC_BAR_CLASSES | ICC_PROGRESS_CLASS |
                ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES |
                ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES;
    ::InitCommonControlsEx(&icc);
    
    CWinApp::InitInstance();
    
    // Enable OLE (for drag & drop, COM server)
    AfxOleInit();
    
    // ── Step 4: Initialize logger ──────────────────────────────────────
    // Get the AppData directory for log files
    wchar_t appDataPath[MAX_PATH];
    ::SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath);
    String logDir = String(appDataPath) + L"\\IDMClone\\Logs";
    String logFile = logDir + L"\\idmclone.log";
    
    Logger::Instance().Initialize(logFile, LogLevel::Info);
    LOG_INFO(L"═══════════════════════════════════════════════════════");
    LOG_INFO(L"IDM Clone %s starting up...", constants::APP_VERSION);
    LOG_INFO(L"═══════════════════════════════════════════════════════");
    
    // ── Step 5: Load settings ──────────────────────────────────────────
    auto settings = Registry::Instance().LoadSettings();
    
    // Ensure default save directory exists
    if (settings.defaultSaveDir.empty()) {
        wchar_t docsPath[MAX_PATH];
        ::SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docsPath);
        settings.defaultSaveDir = String(docsPath) + L"\\Downloads";
    }
    std::error_code ec;
    std::filesystem::create_directories(settings.defaultSaveDir, ec);
    
    // ── Step 6: Initialize download engine ─────────────────────────────
    InitializeEngine();
    
    // ── Step 7: Create main frame ──────────────────────────────────────
    CMainFrame* pFrame = new CMainFrame();
    m_pMainWnd = pFrame;
    
    if (!pFrame->Create(constants::APP_CLASS_NAME, constants::APP_TITLE,
            WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
            CFrameWnd::rectDefault, nullptr, nullptr)) {
        LOG_FATAL(L"Failed to create main window");
        return FALSE;
    }
    
    // Show the window
    if (settings.startMinimized) {
        pFrame->ShowWindow(SW_HIDE);  // Start in tray
    } else {
        pFrame->ShowWindow(SW_SHOW);
    }
    pFrame->UpdateWindow();
    
    // ── Step 8: Process command line ───────────────────────────────────
    ProcessCommandLine();
    
    LOG_INFO(L"IDM Clone: initialization complete");
    return TRUE;
}

// ─── ExitInstance ──────────────────────────────────────────────────────────
int CIDMApp::ExitInstance() {
    LOG_INFO(L"IDM Clone: shutting down...");
    
    // Shut down download engine
    if (m_engineInitialized) {
        DownloadEngine::Instance().Shutdown();
    }
    
    // Save settings
    Registry::Instance().SaveSettings(Registry::Instance().LoadSettings());
    
    // Shut down logger
    Logger::Instance().Shutdown();
    
    // Release single-instance mutex
    if (m_hMutex) {
        ::ReleaseMutex(m_hMutex);
        ::CloseHandle(m_hMutex);
        m_hMutex = nullptr;
    }
    
    // Uninitialize COM
    ::CoUninitialize();
    
    return CWinApp::ExitInstance();
}

// ─── OnIdle ────────────────────────────────────────────────────────────────
BOOL CIDMApp::OnIdle(LONG lCount) {
    BOOL bResult = CWinApp::OnIdle(lCount);
    
    // Periodic maintenance during idle time
    if (lCount == 0) {
        // Update UI elements that need periodic refresh
    }
    
    return bResult;
}

// ─── Single Instance Check ─────────────────────────────────────────────────
bool CIDMApp::CheckSingleInstance() {
    m_hMutex = ::CreateMutexW(nullptr, TRUE, constants::APP_MUTEX_NAME);
    
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        if (m_hMutex) {
            ::CloseHandle(m_hMutex);
            m_hMutex = nullptr;
        }
        return false;
    }
    
    return true;
}

// ─── Initialize Download Engine ────────────────────────────────────────────
void CIDMApp::InitializeEngine() {
    // Data directory in AppData
    wchar_t appDataPath[MAX_PATH];
    ::SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath);
    String dataDir = String(appDataPath) + L"\\IDMClone";
    
    if (DownloadEngine::Instance().Initialize(dataDir)) {
        m_engineInitialized = true;
        LOG_INFO(L"Download engine initialized");
    } else {
        LOG_FATAL(L"Failed to initialize download engine!");
        AfxMessageBox(L"Failed to initialize download engine.", MB_ICONERROR);
    }
}

// ─── Process Command Line ──────────────────────────────────────────────────
void CIDMApp::ProcessCommandLine() {
    // Check for URLs passed via command line
    // Usage: IDMClone.exe "https://example.com/file.zip"
    // This is how browser integration sends URLs to the application
    
    CString cmdLine = GetCommandLine();
    
    // Skip the executable name
    LPWSTR* argv;
    int argc;
    argv = ::CommandLineToArgvW(cmdLine, &argc);
    
    if (argv && argc > 1) {
        for (int i = 1; i < argc; ++i) {
            String arg = argv[i];
            
            if (Unicode::IsHttpUrl(arg) || Unicode::IsHttpsUrl(arg) || 
                Unicode::IsFtpUrl(arg)) {
                // Add URL as a download
                LOG_INFO(L"Command line URL: %s", arg.c_str());
                DownloadEngine::Instance().AddDownload(arg, L"");
            }
        }
        ::LocalFree(argv);
    }
}

} // namespace idm
