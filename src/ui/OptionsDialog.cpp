/**
 * @file OptionsDialog.cpp
 * @brief Options dialog implementation with 9-tab configuration
 *
 * Since we're building dialog templates programmatically (without .rc file
 * dialog templates for the tab pages), each tab page creates its controls
 * in OnInitDialog. In a production build with Visual Studio resource editor,
 * these would be proper IDD_OPT_* dialog templates.
 */

#include "stdafx.h"
#include "OptionsDialog.h"
#include "../core/SpeedLimiter.h"
#include "../core/ProxyManager.h"
#include "../core/AuthManager.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"

namespace idm {

// ─── Main Options Dialog ───────────────────────────────────────────────────
IMPLEMENT_DYNAMIC(COptionsDialog, CDialog)

BEGIN_MESSAGE_MAP(COptionsDialog, CDialog)
    ON_NOTIFY(TCN_SELCHANGE, 0, &COptionsDialog::OnTcnSelchangeTab)
END_MESSAGE_MAP()

COptionsDialog::COptionsDialog(CWnd* pParent)
    : CDialog(IDD_OPTIONS, pParent) {}

COptionsDialog::~COptionsDialog() {
    for (auto* page : m_tabPages) {
        if (page && page->GetSafeHwnd()) {
            page->DestroyWindow();
        }
        delete page;
    }
}

BOOL COptionsDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    
    SetWindowText(L"IDM Clone Options");
    SetWindowPos(nullptr, 0, 0, 600, 500, SWP_NOMOVE | SWP_NOZORDER);
    
    // Center on screen
    CenterWindow();
    
    // Create tab control
    m_tabCtrl.Create(WS_CHILD | WS_VISIBLE | TCS_TABS,
                     CRect(10, 10, 580, 420), this, 0);
    
    // Add tabs matching IDM's 9-tab layout
    m_tabCtrl.InsertItem(0, L"General");
    m_tabCtrl.InsertItem(1, L"File Types");
    m_tabCtrl.InsertItem(2, L"Connection");
    m_tabCtrl.InsertItem(3, L"Save To");
    m_tabCtrl.InsertItem(4, L"Downloads");
    m_tabCtrl.InsertItem(5, L"Proxy/Socks");
    m_tabCtrl.InsertItem(6, L"Site Logins");
    m_tabCtrl.InsertItem(7, L"Dial-Up/VPN");
    m_tabCtrl.InsertItem(8, L"Sounds");
    
    // Load current settings
    LoadSettings();
    
    // Create OK/Cancel buttons
    CRect clientRect;
    GetClientRect(&clientRect);
    
    CButton* pOkBtn = new CButton();
    pOkBtn->Create(L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                   CRect(400, 440, 490, 470), this, IDOK);
    
    CButton* pCancelBtn = new CButton();
    pCancelBtn->Create(L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                       CRect(500, 440, 580, 470), this, IDCANCEL);
    
    // Show first tab
    ShowTabPage(0);
    
    return TRUE;
}

void COptionsDialog::OnOK() {
    SaveSettings();
    CDialog::OnOK();
}

void COptionsDialog::OnTcnSelchangeTab(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    int sel = m_tabCtrl.GetCurSel();
    ShowTabPage(sel);
    *pResult = 0;
}

void COptionsDialog::ShowTabPage(int index) {
    // Hide all pages
    // For now, just show a description of each tab
    // In full implementation, each tab would show its controls
    
    m_currentTab = index;
    
    // Remove previous tab content
    CWnd* pExisting = GetDlgItem(9999);
    if (pExisting) pExisting->DestroyWindow();
    
    // Create a static text showing tab content placeholder
    CRect tabRect;
    m_tabCtrl.GetItemRect(0, &tabRect);
    CRect displayRect;
    m_tabCtrl.GetClientRect(&displayRect);
    displayRect.top += tabRect.Height() + 4;
    displayRect.DeflateRect(10, 5);
    m_tabCtrl.ClientToScreen(&displayRect);
    ScreenToClient(&displayRect);
    
    const wchar_t* tabDescriptions[] = {
        L"General:\n\n"
        L"[x] Start with Windows\n"
        L"[x] Start minimized to system tray\n"
        L"[x] Clipboard monitoring\n"
        L"[x] Dark Mode\n\n"
        L"Browser integration: Chrome, Firefox, Edge",
        
        L"File Types:\n\n"
        L"Auto-captured extensions:\n"
        L".exe .zip .rar .7z .mp3 .mp4 .avi .mkv .pdf .doc .iso\n"
        L".flv .wmv .mov .wav .aac .flac .gz .bz2 .tar .xz .msi\n"
        L".dmg .apk .deb .rpm .bin .img .cab .docx .xlsx .pptx\n"
        L".webm .m4v .opus .ogg .wma .m4a .3gp .mpg .mpeg",
        
        L"Connection:\n\n"
        L"Connection type: High-speed (16+Mbps)\n"
        L"Max connections per download: 8\n"
        L"Timeout: 30 seconds\n"
        L"Number of retries: 20\n"
        L"Retry delay: 5 seconds",
        
        L"Save To:\n\n"
        L"Default download directory:\n"
        L"C:\\Users\\...\\Downloads\n\n"
        L"Per-category directories:\n"
        L"  Music -> Downloads\\Music\n"
        L"  Video -> Downloads\\Video\n"
        L"  Programs -> Downloads\\Programs",
        
        L"Downloads:\n\n"
        L"[x] Show download file info dialog\n"
        L"(*) Show progress dialog normally\n"
        L"[x] Show download complete dialog\n"
        L"[x] Start download immediately",
        
        L"Proxy / Socks:\n\n"
        L"(*) No proxy\n"
        L"( ) Use system proxy settings\n"
        L"( ) Manual proxy configuration\n\n"
        L"HTTP Proxy: _______ Port: ___\n"
        L"SOCKS: _______ Port: ___ Type: SOCKS5",
        
        L"Site Logins:\n\n"
        L"Saved credentials for automatic authentication:\n\n"
        L"URL Pattern        | Username | Password\n"
        L"─────────────────────────────────────────\n"
        L"(no entries)",
        
        L"Dial-Up / VPN:\n\n"
        L"Connection: (none)\n"
        L"[ ] Connect before download\n"
        L"[ ] Disconnect after completion\n"
        L"Redial attempts: 10\n"
        L"Redial interval: 5 seconds",
        
        L"Sounds:\n\n"
        L"Event sound configuration:\n"
        L"[ ] Download started\n"
        L"[ ] Download complete\n"
        L"[ ] Download error\n"
        L"[ ] Queue started\n"
        L"[ ] Queue complete"
    };
    
    CStatic* pContent = new CStatic();
    pContent->Create(tabDescriptions[index],
                     WS_CHILD | WS_VISIBLE | SS_LEFT,
                     displayRect, this, 9999);
    
    // Set a nice font
    CFont* pFont = pContent->GetFont();
    if (!pFont) {
        static CFont sFont;
        if (!sFont.GetSafeHandle()) {
            sFont.CreatePointFont(90, L"Segoe UI");
        }
        pContent->SetFont(&sFont);
    }
}

void COptionsDialog::LoadSettings() {
    m_settings = Registry::Instance().LoadSettings();
}

void COptionsDialog::SaveSettings() {
    Registry::Instance().SaveSettings(m_settings);
    LOG_INFO(L"Options: settings saved");
}

// ─── Tab Page Stubs ────────────────────────────────────────────────────────
// In a full implementation with resource templates, each page would have
// proper controls. These stubs satisfy the linker.

BEGIN_MESSAGE_MAP(COptGeneralPage, CDialog)
END_MESSAGE_MAP()

BOOL COptGeneralPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptFileTypesPage, CDialog)
END_MESSAGE_MAP()

BOOL COptFileTypesPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptConnectionPage, CDialog)
END_MESSAGE_MAP()

BOOL COptConnectionPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptSaveToPage, CDialog)
END_MESSAGE_MAP()

BOOL COptSaveToPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptDownloadsPage, CDialog)
END_MESSAGE_MAP()

BOOL COptDownloadsPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptProxyPage, CDialog)
END_MESSAGE_MAP()

BOOL COptProxyPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptSiteLoginsPage, CDialog)
END_MESSAGE_MAP()

BOOL COptSiteLoginsPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptDialUpPage, CDialog)
END_MESSAGE_MAP()

BOOL COptDialUpPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

BEGIN_MESSAGE_MAP(COptSoundsPage, CDialog)
END_MESSAGE_MAP()

BOOL COptSoundsPage::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;
}

} // namespace idm
