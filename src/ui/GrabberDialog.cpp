/**
 * @file GrabberDialog.cpp
 * @brief Site Grabber / Spider dialog (Phase 5 stub)
 */
#include "stdafx.h"
#include "GrabberDialog.h"

namespace idm {

IMPLEMENT_DYNAMIC(CGrabberDialog, CDialog)

BEGIN_MESSAGE_MAP(CGrabberDialog, CDialog)
END_MESSAGE_MAP()

CGrabberDialog::CGrabberDialog(CWnd* pParent)
    : CDialog(IDD_SITE_GRABBER, pParent) {
}

CGrabberDialog::~CGrabberDialog() = default;

BOOL CGrabberDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText(L"Site Grabber (Phase 5)");
    return TRUE;
}

} // namespace idm
