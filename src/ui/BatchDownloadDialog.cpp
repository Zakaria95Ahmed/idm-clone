/**
 * @file BatchDownloadDialog.cpp
 */

#include "stdafx.h"
#include "BatchDownloadDialog.h"
#include "../core/DownloadEngine.h"
#include "../util/Unicode.h"

namespace idm {

IMPLEMENT_DYNAMIC(CBatchDownloadDialog, CDialog)

BEGIN_MESSAGE_MAP(CBatchDownloadDialog, CDialog)
    ON_BN_CLICKED(IDC_BATCH_PREVIEW_BTN, &CBatchDownloadDialog::OnBnClickedPreview)
    ON_BN_CLICKED(IDOK, &CBatchDownloadDialog::OnBnClickedDownload)
END_MESSAGE_MAP()

CBatchDownloadDialog::CBatchDownloadDialog(CWnd* pParent)
    : CDialog(IDD_BATCH_DOWNLOAD, pParent) {}

CBatchDownloadDialog::~CBatchDownloadDialog() = default;

BOOL CBatchDownloadDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    
    SetWindowText(L"Add Batch Download");
    SetWindowPos(nullptr, 0, 0, 550, 450, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    
    int y = 10;
    
    // URL template
    CStatic* pLabel = new CStatic();
    pLabel->Create(L"URL template (use [from-to] for wildcards):", 
                   WS_CHILD | WS_VISIBLE, CRect(10, y, 530, y + 20), this);
    y += 22;
    
    CEdit* pUrlTemplate = new CEdit();
    pUrlTemplate->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                         CRect(10, y, 530, y + 24), this, IDC_BATCH_URL_TEMPLATE);
    pUrlTemplate->SetWindowTextW(L"http://example.com/file_[001-100].zip");
    y += 32;
    
    // From/To fields
    CStatic* pFromLabel = new CStatic();
    pFromLabel->Create(L"From:", WS_CHILD | WS_VISIBLE, CRect(10, y, 50, y + 20), this);
    
    CEdit* pFrom = new CEdit();
    pFrom->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(55, y, 130, y + 24), this, IDC_BATCH_FROM_EDIT);
    pFrom->SetWindowTextW(L"001");
    
    CStatic* pToLabel = new CStatic();
    pToLabel->Create(L"To:", WS_CHILD | WS_VISIBLE, CRect(150, y, 180, y + 20), this);
    
    CEdit* pTo = new CEdit();
    pTo->Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(185, y, 260, y + 24), this, IDC_BATCH_TO_EDIT);
    pTo->SetWindowTextW(L"100");
    
    CButton* pPreview = new CButton();
    pPreview->Create(L"Preview", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                     CRect(280, y, 370, y + 24), this, IDC_BATCH_PREVIEW_BTN);
    y += 35;
    
    // Preview list
    CStatic* pPreviewLabel = new CStatic();
    pPreviewLabel->Create(L"Generated URLs:", WS_CHILD | WS_VISIBLE, 
                          CRect(10, y, 530, y + 20), this);
    y += 22;
    
    CListBox* pPreviewList = new CListBox();
    pPreviewList->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_EXTENDEDSEL,
                         CRect(10, y, 530, y + 200), this, IDC_BATCH_PREVIEW_LIST);
    y += 210;
    
    // Buttons
    CButton* pDownload = new CButton();
    pDownload->Create(L"Download All", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                      CRect(340, y, 440, y + 28), this, IDOK);
    
    CButton* pCancel = new CButton();
    pCancel->Create(L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    CRect(450, y, 530, y + 28), this, IDCANCEL);
    
    return TRUE;
}

void CBatchDownloadDialog::OnBnClickedPreview() {
    GenerateUrls();
    
    CListBox* pList = static_cast<CListBox*>(GetDlgItem(IDC_BATCH_PREVIEW_LIST));
    if (!pList) return;
    
    pList->ResetContent();
    for (const auto& url : m_generatedUrls) {
        pList->AddString(url.c_str());
    }
}

void CBatchDownloadDialog::GenerateUrls() {
    m_generatedUrls.clear();
    
    CString urlTemplate;
    GetDlgItemText(IDC_BATCH_URL_TEMPLATE, urlTemplate);
    
    // Find wildcard pattern [from-to]
    String templ = urlTemplate.GetString();
    auto openBracket = templ.find(L'[');
    auto closeBracket = templ.find(L']');
    
    if (openBracket == String::npos || closeBracket == String::npos || 
        closeBracket <= openBracket) {
        m_generatedUrls.push_back(templ);
        return;
    }
    
    String prefix = templ.substr(0, openBracket);
    String suffix = templ.substr(closeBracket + 1);
    String range = templ.substr(openBracket + 1, closeBracket - openBracket - 1);
    
    auto dashPos = range.find(L'-');
    if (dashPos == String::npos) return;
    
    String fromStr = range.substr(0, dashPos);
    String toStr = range.substr(dashPos + 1);
    
    // Check if alphabetic or numeric
    bool isAlpha = !fromStr.empty() && iswalpha(fromStr[0]);
    
    if (isAlpha) {
        wchar_t fromChar = fromStr[0];
        wchar_t toChar = toStr[0];
        for (wchar_t c = fromChar; c <= toChar; ++c) {
            m_generatedUrls.push_back(prefix + c + suffix);
        }
    } else {
        int padWidth = static_cast<int>(fromStr.length());
        try {
            int from = std::stoi(fromStr);
            int to = std::stoi(toStr);
            for (int i = from; i <= to; ++i) {
                wchar_t numBuf[32];
                _snwprintf_s(numBuf, _TRUNCATE, L"%0*d", padWidth, i);
                m_generatedUrls.push_back(prefix + numBuf + suffix);
            }
        } catch (...) {}
    }
}

void CBatchDownloadDialog::OnBnClickedDownload() {
    if (m_generatedUrls.empty()) GenerateUrls();
    
    for (const auto& url : m_generatedUrls) {
        DownloadEngine::Instance().AddDownload(url, L"", L"", L"", L"", false);
    }
    
    CString msg;
    msg.Format(L"Added %d downloads to queue.", static_cast<int>(m_generatedUrls.size()));
    AfxMessageBox(msg, MB_ICONINFORMATION);
    
    EndDialog(IDOK);
}

} // namespace idm
