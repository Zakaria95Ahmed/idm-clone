/**
 * @file OptionsDialog.h / OptionsDialog.cpp
 * @brief Options dialog with 9 tabs matching IDM's settings
 */

#pragma once
#include "stdafx.h"
#include "resource.h"
#include "../util/Registry.h"

namespace idm {

class COptionsDialog : public CDialog {
    DECLARE_DYNAMIC(COptionsDialog)
    
public:
    COptionsDialog(CWnd* pParent = nullptr);
    virtual ~COptionsDialog();
    
protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
    
    afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
    
    DECLARE_MESSAGE_MAP()
    
private:
    void CreateTabPages();
    void ShowTabPage(int index);
    void LoadSettings();
    void SaveSettings();
    
    // Tab control
    CTabCtrl    m_tabCtrl;
    int         m_currentTab{0};
    
    // Tab page dialogs (modeless children)
    std::vector<CDialog*> m_tabPages;
    
    // Settings cache
    Registry::AppSettings m_settings;
};

// ─── Tab Page Dialogs ──────────────────────────────────────────────────────
// Each tab is a child dialog positioned within the tab control's display area

class COptGeneralPage : public CDialog {
public:
    COptGeneralPage(CWnd* pParent) : CDialog(IDD_OPT_GENERAL, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptFileTypesPage : public CDialog {
public:
    COptFileTypesPage(CWnd* pParent) : CDialog(IDD_OPT_FILE_TYPES, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptConnectionPage : public CDialog {
public:
    COptConnectionPage(CWnd* pParent) : CDialog(IDD_OPT_CONNECTION, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptSaveToPage : public CDialog {
public:
    COptSaveToPage(CWnd* pParent) : CDialog(IDD_OPT_SAVE_TO, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptDownloadsPage : public CDialog {
public:
    COptDownloadsPage(CWnd* pParent) : CDialog(IDD_OPT_DOWNLOADS, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptProxyPage : public CDialog {
public:
    COptProxyPage(CWnd* pParent) : CDialog(IDD_OPT_PROXY, pParent) {}
    Registry::AppSettings* m_pSettings{nullptr};
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptSiteLoginsPage : public CDialog {
public:
    COptSiteLoginsPage(CWnd* pParent) : CDialog(IDD_OPT_SITE_LOGINS, pParent) {}
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptDialUpPage : public CDialog {
public:
    COptDialUpPage(CWnd* pParent) : CDialog(IDD_OPT_DIALUP, pParent) {}
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

class COptSoundsPage : public CDialog {
public:
    COptSoundsPage(CWnd* pParent) : CDialog(IDD_OPT_SOUNDS, pParent) {}
    
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

} // namespace idm
