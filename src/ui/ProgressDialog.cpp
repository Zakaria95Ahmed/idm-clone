/**
 * @file ProgressDialog.cpp
 * @brief Progress dialog implementation with live visualization
 */

#include "stdafx.h"
#include "ProgressDialog.h"
#include "../core/DownloadEngine.h"
#include "../util/Unicode.h"
#include "../util/Logger.h"

namespace idm {

IMPLEMENT_DYNAMIC(CProgressDialog, CDialog)

BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_PAUSE_BTN, &CProgressDialog::OnBnClickedPause)
    ON_BN_CLICKED(IDC_CANCEL_BTN, &CProgressDialog::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_OPEN_FOLDER_BTN, &CProgressDialog::OnBnClickedOpenFolder)
    ON_BN_CLICKED(IDC_OPEN_FILE_BTN, &CProgressDialog::OnBnClickedOpenFile)
END_MESSAGE_MAP()

CProgressDialog::CProgressDialog(const DownloadEntry& entry, CWnd* pParent)
    : CDialog(IDD_PROGRESS, pParent)
    , m_entry(entry) {}

CProgressDialog::~CProgressDialog() = default;

BOOL CProgressDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    
    CString title;
    title.Format(L"Downloading - %s", m_entry.fileName.c_str());
    SetWindowText(title);
    
    // Resize to fit all controls
    SetWindowPos(nullptr, 0, 0, 550, 500, SWP_NOMOVE | SWP_NOZORDER);
    
    // Create controls programmatically since we don't have a resource template
    int y = 10;
    int leftMargin = 10;
    int rightEdge = 530;
    int controlWidth = rightEdge - leftMargin;
    
    // File name label
    CStatic* pFileName = new CStatic();
    pFileName->Create(m_entry.fileName.c_str(), WS_CHILD | WS_VISIBLE, 
                      CRect(leftMargin, y, rightEdge, y + 20), this, IDC_FILENAME_LABEL);
    y += 25;
    
    // URL label
    CStatic* pUrl = new CStatic();
    CString urlText = L"URL: " + CString(m_entry.url.c_str());
    pUrl->Create(urlText.Left(80), WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS,
                 CRect(leftMargin, y, rightEdge, y + 18), this, IDC_URL_LABEL);
    y += 25;
    
    // Progress bar
    m_progressBar.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                         CRect(leftMargin, y, rightEdge, y + 22), this, IDC_PROGRESS_BAR);
    m_progressBar.SetRange(0, 1000);
    y += 30;
    
    // Segment visualization bar
    m_segmentBar.Create(L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                        CRect(leftMargin, y, rightEdge, y + 30), this, IDC_SEGMENT_BAR);
    y += 38;
    
    // Speed graph
    m_speedGraph.Create(L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                        CRect(leftMargin, y, rightEdge, y + 120), this, IDC_SPEED_GRAPH);
    y += 128;
    
    // Info labels
    auto createLabel = [&](UINT id, const CString& text) {
        CStatic* pLabel = new CStatic();
        pLabel->Create(text, WS_CHILD | WS_VISIBLE,
                       CRect(leftMargin, y, rightEdge, y + 18), this, id);
        y += 20;
    };
    
    createLabel(IDC_STATUS_LABEL, L"Status: " + CString(m_entry.StatusString().c_str()));
    createLabel(IDC_SPEED_LABEL, L"Speed: 0 B/s");
    createLabel(IDC_AVG_SPEED_LABEL, L"Average Speed: 0 B/s");
    createLabel(IDC_DOWNLOADED_LABEL, L"Downloaded: 0 B of " + 
                CString(Unicode::FormatFileSize(m_entry.fileSize).c_str()));
    createLabel(IDC_TIME_LEFT_LABEL, L"Time Left: Unknown");
    createLabel(IDC_RESUME_LABEL, 
                CString(L"Resume: ") + (m_entry.resumeSupported ? L"Yes" : L"No"));
    createLabel(IDC_CONNECTIONS_LABEL, 
                CString(L"Connections: ") + CString(std::to_wstring(m_entry.numConnections).c_str()));
    
    y += 10;
    
    // Buttons
    int btnWidth = 90;
    int btnHeight = 28;
    int btnX = leftMargin;
    
    CButton* pPause = new CButton();
    pPause->Create(L"Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                   CRect(btnX, y, btnX + btnWidth, y + btnHeight), this, IDC_PAUSE_BTN);
    btnX += btnWidth + 10;
    
    CButton* pCancel = new CButton();
    pCancel->Create(L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    CRect(btnX, y, btnX + btnWidth, y + btnHeight), this, IDC_CANCEL_BTN);
    btnX += btnWidth + 10;
    
    CButton* pOpenFolder = new CButton();
    pOpenFolder->Create(L"Open Folder", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        CRect(btnX, y, btnX + btnWidth, y + btnHeight), this, IDC_OPEN_FOLDER_BTN);
    btnX += btnWidth + 10;
    
    CButton* pOpenFile = new CButton();
    pOpenFile->Create(L"Open File", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      CRect(btnX, y, btnX + btnWidth, y + btnHeight), this, IDC_OPEN_FILE_BTN);
    
    // Enable/disable based on status
    if (m_entry.status != DownloadStatus::Complete) {
        pOpenFile->EnableWindow(FALSE);
    }
    
    // Register as observer
    DownloadEngine::Instance().AddObserver(this);
    
    // Start update timer
    SetTimer(TIMER_UPDATE, 1000, nullptr);
    
    UpdateDisplay();
    
    return TRUE;
}

void CProgressDialog::OnDestroy() {
    KillTimer(TIMER_UPDATE);
    DownloadEngine::Instance().RemoveObserver(this);
    CDialog::OnDestroy();
}

void CProgressDialog::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == TIMER_UPDATE) {
        UpdateDisplay();
    }
    CDialog::OnTimer(nIDEvent);
}

void CProgressDialog::UpdateDisplay() {
    auto entryOpt = DownloadEngine::Instance().GetDownload(m_entry.id);
    if (!entryOpt.has_value()) return;
    
    m_entry = entryOpt.value();
    
    // Update progress bar
    if (m_entry.fileSize > 0) {
        int progress = static_cast<int>(m_entry.ProgressPercent() * 10);
        m_progressBar.SetPos(progress);
    }
    
    // Update labels
    CWnd* pWnd;
    CString text;
    
    pWnd = GetDlgItem(IDC_STATUS_LABEL);
    if (pWnd) {
        text.Format(L"Status: %s", m_entry.StatusString().c_str());
        pWnd->SetWindowTextW(text);
    }
    
    pWnd = GetDlgItem(IDC_SPEED_LABEL);
    if (pWnd) {
        text.Format(L"Speed: %s", Unicode::FormatSpeed(m_entry.currentSpeed).c_str());
        pWnd->SetWindowTextW(text);
    }
    
    pWnd = GetDlgItem(IDC_AVG_SPEED_LABEL);
    if (pWnd) {
        text.Format(L"Average Speed: %s", Unicode::FormatSpeed(m_entry.averageSpeed).c_str());
        pWnd->SetWindowTextW(text);
    }
    
    pWnd = GetDlgItem(IDC_DOWNLOADED_LABEL);
    if (pWnd) {
        text.Format(L"Downloaded: %s of %s",
            Unicode::FormatFileSize(m_entry.downloadedBytes).c_str(),
            Unicode::FormatFileSize(m_entry.fileSize).c_str());
        pWnd->SetWindowTextW(text);
    }
    
    pWnd = GetDlgItem(IDC_TIME_LEFT_LABEL);
    if (pWnd) {
        if (m_entry.currentSpeed > 0 && m_entry.fileSize > 0) {
            double remaining = static_cast<double>(m_entry.fileSize - m_entry.downloadedBytes) / m_entry.currentSpeed;
            text.Format(L"Time Left: %s", Unicode::FormatTimeRemaining(remaining).c_str());
        } else {
            text = L"Time Left: Unknown";
        }
        pWnd->SetWindowTextW(text);
    }
    
    // Update speed graph
    m_speedGraph.AddDataPoint(m_entry.currentSpeed);
    
    // Update window title
    if (m_entry.fileSize > 0) {
        CString title;
        title.Format(L"%.1f%% - %s", m_entry.ProgressPercent(), m_entry.fileName.c_str());
        SetWindowText(title);
    }
}

// Observer callbacks (called from download thread)
void CProgressDialog::OnDownloadProgress(const String& id, int64, int64, double) {
    if (id == m_entry.id) PostMessage(WM_TIMER, TIMER_UPDATE, 0);
}

void CProgressDialog::OnDownloadSegmentUpdate(const String& id, 
                                               const std::vector<Segment>& segments) {
    if (id == m_entry.id) {
        m_segmentBar.SetSegments(segments, m_entry.fileSize);
    }
}

void CProgressDialog::OnDownloadComplete(const String& id) {
    if (id == m_entry.id) {
        PostMessage(WM_TIMER, TIMER_UPDATE, 0);
        
        CWnd* pOpenFile = GetDlgItem(IDC_OPEN_FILE_BTN);
        if (pOpenFile) pOpenFile->EnableWindow(TRUE);
    }
}

void CProgressDialog::OnDownloadError(const String& id, const String& error) {
    if (id == m_entry.id) {
        PostMessage(WM_TIMER, TIMER_UPDATE, 0);
    }
}

// Button handlers
void CProgressDialog::OnBnClickedPause() {
    if (m_entry.status == DownloadStatus::Downloading) {
        DownloadEngine::Instance().PauseDownload(m_entry.id);
    } else {
        DownloadEngine::Instance().StartDownload(m_entry.id);
    }
}

void CProgressDialog::OnBnClickedCancel() {
    DownloadEngine::Instance().StopDownload(m_entry.id);
    EndDialog(IDCANCEL);
}

void CProgressDialog::OnBnClickedOpenFolder() {
    String param = L"/select,\"" + m_entry.FullPath() + L"\"";
    ::ShellExecuteW(GetSafeHwnd(), nullptr, L"explorer.exe", 
                    param.c_str(), nullptr, SW_SHOWNORMAL);
}

void CProgressDialog::OnBnClickedOpenFile() {
    if (m_entry.status == DownloadStatus::Complete) {
        ::ShellExecuteW(GetSafeHwnd(), L"open", m_entry.FullPath().c_str(),
                        nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void CProgressDialog::OnCancel() {
    // Don't stop the download when closing the dialog
    CDialog::OnCancel();
}

} // namespace idm
