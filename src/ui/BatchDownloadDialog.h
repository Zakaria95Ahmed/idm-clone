/**
 * @file BatchDownloadDialog.h / BatchDownloadDialog.cpp
 * @brief Batch download dialog with wildcard URL templates
 */

#pragma once
#include "stdafx.h"
#include "resource.h"

namespace idm {

class CBatchDownloadDialog : public CDialog {
    DECLARE_DYNAMIC(CBatchDownloadDialog)
public:
    CBatchDownloadDialog(CWnd* pParent = nullptr);
    virtual ~CBatchDownloadDialog();
protected:
    virtual BOOL OnInitDialog() override;
    afx_msg void OnBnClickedPreview();
    afx_msg void OnBnClickedDownload();
    DECLARE_MESSAGE_MAP()
private:
    void GenerateUrls();
    std::vector<String> m_generatedUrls;
};

} // namespace idm
