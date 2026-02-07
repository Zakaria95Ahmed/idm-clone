/**
 * @file MainFrame.h
 * @brief Main application window - the primary IDM Clone interface
 *
 * The main frame implements IDM's characteristic split-panel layout:
 *   - Left: Category tree view for filtering downloads
 *   - Right: Download list view showing all downloads
 *   - Top: Menu bar and customizable toolbar
 *   - Bottom: Status bar with speed and active download count
 *
 * The frame also manages:
 *   - System tray icon with context menu
 *   - Clipboard monitoring for URL detection
 *   - Drag & drop support
 *   - Dark mode theming
 *   - Observer pattern connection to the download engine
 */

#pragma once
#include "stdafx.h"
#include "resource.h"
#include "../core/DownloadEngine.h"

namespace idm {

class DownloadListView;
class CategoryTreeView;

class CMainFrame : public CFrameWnd, public IDownloadObserver {
    DECLARE_DYNAMIC(CMainFrame)
    
public:
    CMainFrame();
    virtual ~CMainFrame();
    
    // ─── MFC Overrides ────────────────────────────────────────────────────
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, 
                          AFX_CMDHANDLERINFO* pHandlerInfo) override;
    
    // ─── Public Interface ─────────────────────────────────────────────────
    void SetDarkMode(bool dark);
    bool IsDarkMode() const { return m_darkMode; }
    void RefreshDownloadList();
    void ShowBalloonNotification(const String& title, const String& text);
    
    // ─── IDownloadObserver Implementation ─────────────────────────────────
    void OnDownloadAdded(const String& id) override;
    void OnDownloadStarted(const String& id) override;
    void OnDownloadProgress(const String& id, int64 downloaded, 
                            int64 total, double speed) override;
    void OnDownloadComplete(const String& id) override;
    void OnDownloadError(const String& id, const String& error) override;
    void OnDownloadPaused(const String& id) override;
    void OnDownloadRemoved(const String& id) override;
    void OnSpeedUpdate(double totalSpeed, int activeCount) override;
    
protected:
    // ─── Message Handlers ─────────────────────────────────────────────────
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg LRESULT OnTrayNotify(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDownloadProgressMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnClipboardUrl(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnThemeChanged(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRefreshList(WPARAM wParam, LPARAM lParam);
    
    // ─── Menu Command Handlers ────────────────────────────────────────────
    // Tasks menu
    afx_msg void OnTasksAddUrl();
    afx_msg void OnTasksAddBatch();
    afx_msg void OnTasksExport();
    afx_msg void OnTasksImport();
    afx_msg void OnTasksSiteGrabber();
    afx_msg void OnFileExit();
    
    // Downloads menu
    afx_msg void OnDlResume();
    afx_msg void OnDlResumeAll();
    afx_msg void OnDlStop();
    afx_msg void OnDlStopAll();
    afx_msg void OnDlDelete();
    afx_msg void OnDlDeleteCompleted();
    afx_msg void OnDlDeleteWithFile();
    afx_msg void OnDlOpenFile();
    afx_msg void OnDlOpenFolder();
    afx_msg void OnDlProperties();
    afx_msg void OnDlSelectAll();
    
    // Queues menu
    afx_msg void OnQueueScheduler();
    afx_msg void OnQueueStart();
    afx_msg void OnQueueStop();
    
    // Options menu
    afx_msg void OnOptionsSettings();
    afx_msg void OnOptionsSpeedLimiter();
    
    // View menu
    afx_msg void OnViewCategories();
    afx_msg void OnViewToolbar();
    afx_msg void OnViewDarkMode();
    
    // Help menu
    afx_msg void OnHelpAbout();
    
    // Tray commands
    afx_msg void OnTrayOpen();
    afx_msg void OnTrayExit();
    
    // ─── Update Handlers ──────────────────────────────────────────────────
    afx_msg void OnUpdateDlResume(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDlStop(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDlDelete(CCmdUI* pCmdUI);
    
    DECLARE_MESSAGE_MAP()
    
private:
    // ─── UI Setup ─────────────────────────────────────────────────────────
    void CreateMenuBar();
    void CreateToolBar();
    void CreateStatusBar();
    void CreateSplitterLayout();
    void SetupSystemTray();
    void RemoveSystemTray();
    void SetupClipboardMonitor();
    void SetupTimers();
    void ApplyDarkMode();
    
    // ─── UI Components ────────────────────────────────────────────────────
    CToolBar            m_wndToolBar;
    CStatusBar          m_wndStatusBar;
    CSplitterWnd        m_wndSplitter;
    CategoryTreeView*   m_pTreeView{nullptr};
    DownloadListView*   m_pListView{nullptr};
    
    // ─── State ────────────────────────────────────────────────────────────
    bool                m_darkMode{false};
    bool                m_trayIconCreated{false};
    NOTIFYICONDATAW     m_trayIconData{};
    UINT                m_clipboardFormatUrl{0};
    HWND                m_nextClipboardViewer{nullptr};
    
    // Timer IDs
    static constexpr UINT_PTR TIMER_UI_REFRESH = 1;
    static constexpr UINT_PTR TIMER_CLIPBOARD  = 2;
    static constexpr UINT_PTR TIMER_TRAY_ANIM  = 3;
};

} // namespace idm
