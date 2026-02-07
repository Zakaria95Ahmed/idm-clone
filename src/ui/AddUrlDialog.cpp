/**
 * @file AddUrlDialog.cpp
 * @brief Add URL dialog implementation
 */

#include "stdafx.h"
#include "AddUrlDialog.h"
#include "../core/DownloadEngine.h"
#include "../core/HttpClient.h"
#include "../util/Unicode.h"
#include "../util/Registry.h"
#include "../util/Logger.h"

namespace idm {

IMPLEMENT_DYNAMIC(CAddUrlDialog, CDialog)

BEGIN_MESSAGE_MAP(CAddUrlDialog, CDialog)
    ON_BN_CLICKED(IDC_START_DOWNLOAD_BTN, &CAddUrlDialog::OnBnClickedStartDownload)
    ON_BN_CLICKED(IDC_DOWNLOAD_LATER_BTN, &CAddUrlDialog::OnBnClickedDownloadLater)
    ON_BN_CLICKED(IDC_BROWSE_BTN, &CAddUrlDialog::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_USE_AUTH_CHECK, &CAddUrlDialog::OnBnClickedUseAuth)
    ON_EN_KILLFOCUS(IDC_URL_EDIT, &CAddUrlDialog::OnEnChangeUrl)
END_MESSAGE_MAP()

CAddUrlDialog::CAddUrlDialog(CWnd* pParent)
    : CDialog(IDD_ADD_URL, pParent) {}

CAddUrlDialog::~CAddUrlDialog() = default;

void CAddUrlDialog::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URL_EDIT, m_editUrl);
    DDX_Control(pDX, IDC_FILENAME_EDIT, m_editFileName);
    DDX_Control(pDX, IDC_SAVE_TO_EDIT, m_editSaveTo);
    DDX_Control(pDX, IDC_CATEGORY_COMBO, m_comboCategory);
    DDX_Control(pDX, IDC_DESCRIPTION_EDIT, m_editDescription);
    DDX_Control(pDX, IDC_REFERER_EDIT, m_editReferrer);
    DDX_Control(pDX, IDC_USERNAME_EDIT, m_editUsername);
    DDX_Control(pDX, IDC_PASSWORD_EDIT, m_editPassword);
    DDX_Control(pDX, IDC_USE_AUTH_CHECK, m_checkUseAuth);
    DDX_Control(pDX, IDC_FILESIZE_STATIC, m_staticFileSize);
    DDX_Control(pDX, IDC_RESUME_STATIC, m_staticResume);
}

BOOL CAddUrlDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    
    SetWindowText(L"Add New Download");
    
    // Populate category dropdown
    m_comboCategory.AddString(L"General");
    m_comboCategory.AddString(L"Music");
    m_comboCategory.AddString(L"Video");
    m_comboCategory.AddString(L"Programs");
    m_comboCategory.AddString(L"Documents");
    m_comboCategory.AddString(L"Compressed");
    m_comboCategory.SetCurSel(0);
    
    // Set default save directory
    auto settings = Registry::Instance().LoadSettings();
    m_editSaveTo.SetWindowTextW(settings.defaultSaveDir.c_str());
    
    // Check clipboard for URL
    if (::IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        if (::OpenClipboard(GetSafeHwnd())) {
            HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                LPCWSTR text = static_cast<LPCWSTR>(::GlobalLock(hData));
                if (text) {
                    String clipText = text;
                    ::GlobalUnlock(hData);
                    
                    if (Unicode::IsHttpUrl(clipText) || Unicode::IsHttpsUrl(clipText) ||
                        Unicode::IsFtpUrl(clipText)) {
                        m_editUrl.SetWindowTextW(clipText.c_str());
                    }
                }
            }
            ::CloseClipboard();
        }
    }
    
    // Disable auth fields by default
    UpdateAuthControls();
    
    // Set initial file size and resume status
    m_staticFileSize.SetWindowTextW(L"File Size: Unknown");
    m_staticResume.SetWindowTextW(L"Resume: Unknown");
    
    // Focus on URL field
    m_editUrl.SetFocus();
    
    return FALSE; // We set focus
}

void CAddUrlDialog::OnBnClickedStartDownload() {
    CString url, fileName, saveTo, desc, ref, user, pass;
    m_editUrl.GetWindowTextW(url);
    m_editFileName.GetWindowTextW(fileName);
    m_editSaveTo.GetWindowTextW(saveTo);
    m_editDescription.GetWindowTextW(desc);
    m_editReferrer.GetWindowTextW(ref);
    m_editUsername.GetWindowTextW(user);
    m_editPassword.GetWindowTextW(pass);
    
    if (url.IsEmpty()) {
        AfxMessageBox(L"Please enter a URL.", MB_ICONWARNING);
        return;
    }
    
    // Get category
    int catSel = m_comboCategory.GetCurSel();
    CString category;
    if (catSel >= 0) m_comboCategory.GetLBText(catSel, category);
    
    // Create download entry
    DownloadEntry entry;
    entry.url = url.GetString();
    entry.fileName = fileName.GetString();
    entry.savePath = saveTo.GetString();
    entry.category = category.GetString();
    entry.description = desc.GetString();
    entry.referrer = ref.GetString();
    
    if (m_checkUseAuth.GetCheck() == BST_CHECKED) {
        entry.username = user.GetString();
        entry.password = pass.GetString();
    }
    
    // Add and start download
    DownloadEngine::Instance().AddDownload(entry, true);
    
    m_startImmediately = true;
    EndDialog(IDOK);
}

void CAddUrlDialog::OnBnClickedDownloadLater() {
    CString url, fileName, saveTo;
    m_editUrl.GetWindowTextW(url);
    m_editFileName.GetWindowTextW(fileName);
    m_editSaveTo.GetWindowTextW(saveTo);
    
    if (url.IsEmpty()) {
        AfxMessageBox(L"Please enter a URL.", MB_ICONWARNING);
        return;
    }
    
    int catSel = m_comboCategory.GetCurSel();
    CString category;
    if (catSel >= 0) m_comboCategory.GetLBText(catSel, category);
    
    DownloadEntry entry;
    entry.url = url.GetString();
    entry.fileName = fileName.GetString();
    entry.savePath = saveTo.GetString();
    entry.category = category.GetString();
    entry.status = DownloadStatus::Queued;
    
    DownloadEngine::Instance().AddDownload(entry, false);
    
    m_startImmediately = false;
    EndDialog(IDOK);
}

void CAddUrlDialog::OnBnClickedBrowse() {
    CFolderPickerDialog dlg(nullptr, 0, this);
    if (dlg.DoModal() == IDOK) {
        m_editSaveTo.SetWindowTextW(dlg.GetPathName());
    }
}

void CAddUrlDialog::OnBnClickedUseAuth() {
    UpdateAuthControls();
}

void CAddUrlDialog::UpdateAuthControls() {
    BOOL enable = (m_checkUseAuth.GetCheck() == BST_CHECKED);
    m_editUsername.EnableWindow(enable);
    m_editPassword.EnableWindow(enable);
}

void CAddUrlDialog::OnEnChangeUrl() {
    // When URL field loses focus, probe the URL for file info
    ProbeUrl();
}

void CAddUrlDialog::ProbeUrl() {
    CString urlText;
    m_editUrl.GetWindowTextW(urlText);
    
    if (urlText.IsEmpty()) return;
    
    String url = urlText.GetString();
    if (!Unicode::IsHttpUrl(url) && !Unicode::IsHttpsUrl(url) && !Unicode::IsFtpUrl(url)) {
        return;
    }
    
    // Set busy cursor
    CWaitCursor wait;
    
    HttpResponseInfo response;
    String suggestedName, category;
    
    if (DownloadEngine::Instance().ProbeUrl(url, response, suggestedName, category)) {
        // Update filename
        if (!suggestedName.empty()) {
            m_editFileName.SetWindowTextW(suggestedName.c_str());
        }
        
        // Update file size
        CString sizeText;
        if (response.contentLength > 0) {
            sizeText.Format(L"File Size: %s", 
                Unicode::FormatFileSize(response.contentLength).c_str());
        } else {
            sizeText = L"File Size: Unknown";
        }
        m_staticFileSize.SetWindowTextW(sizeText);
        
        // Update resume capability
        m_staticResume.SetWindowTextW(
            response.acceptRanges ? L"Resume: Yes" : L"Resume: No");
        
        // Auto-select category
        for (int i = 0; i < m_comboCategory.GetCount(); ++i) {
            CString catText;
            m_comboCategory.GetLBText(i, catText);
            if (catText.GetString() == category) {
                m_comboCategory.SetCurSel(i);
                break;
            }
        }
    } else {
        m_staticFileSize.SetWindowTextW(L"File Size: Error");
        m_staticResume.SetWindowTextW(L"Resume: Error");
    }
}

} // namespace idm
