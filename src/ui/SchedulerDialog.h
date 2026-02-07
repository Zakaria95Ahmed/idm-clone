/**
 * @file SchedulerDialog.h / SchedulerDialog.cpp
 * @brief Queue and scheduler management dialog
 */

#pragma once
#include "stdafx.h"
#include "resource.h"

namespace idm {

class CSchedulerDialog : public CDialog {
    DECLARE_DYNAMIC(CSchedulerDialog)
public:
    CSchedulerDialog(CWnd* pParent = nullptr);
    virtual ~CSchedulerDialog();
protected:
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
};

} // namespace idm
