/**
 * @file AddUrlDialog.h
 * @brief "Add URL" dialog - shown when user initiates a new download
 */

#pragma once
#include "stdafx.h"
#include "resource.h"

namespace idm {

class CAddUrlDialog : public CDialog {
    DECLARE_DYNAMIC(CAddUrlDialog)
    
public:
    CAddUrlDialog(CWnd* pParent = nullptr);
    virtual ~CAddUrlDialog();
    
    // Output: the URL and settings entered by the user
    String  m_url;
    String  m_fileName;
    String  m_savePath;
    String  m_category;
    String  m_description;
    String  m_referrer;
    String  m_username;
    String  m_password;
    bool    m_useAuth{false};
    bool    m_startImmediately{true};
    
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    
    afx_msg void OnBnClickedStartDownload();
    afx_msg void OnBnClickedDownloadLater();
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedUseAuth();
    afx_msg void OnEnChangeUrl();
    
    DECLARE_MESSAGE_MAP()
    
private:
    void ProbeUrl();
    void UpdateAuthControls();
    
    CEdit       m_editUrl;
    CEdit       m_editFileName;
    CEdit       m_editSaveTo;
    CComboBox   m_comboCategory;
    CEdit       m_editDescription;
    CEdit       m_editReferrer;
    CEdit       m_editUsername;
    CEdit       m_editPassword;
    CButton     m_checkUseAuth;
    CStatic     m_staticFileSize;
    CStatic     m_staticResume;
};

} // namespace idm
