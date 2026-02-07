/**
 * @file GrabberDialog.h
 * @brief Site Grabber / Spider dialog (Phase 5 stub)
 */

#pragma once
#include "stdafx.h"
#include "resource.h"

namespace idm {

class CGrabberDialog : public CDialog {
    DECLARE_DYNAMIC(CGrabberDialog)
public:
    CGrabberDialog(CWnd* pParent = nullptr);
    virtual ~CGrabberDialog();
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

} // namespace idm
