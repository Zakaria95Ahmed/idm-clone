/**
 * @file SchedulerDialog.cpp
 */

#include "stdafx.h"
#include "SchedulerDialog.h"

namespace idm {

IMPLEMENT_DYNAMIC(CSchedulerDialog, CDialog)

BEGIN_MESSAGE_MAP(CSchedulerDialog, CDialog)
END_MESSAGE_MAP()

CSchedulerDialog::CSchedulerDialog(CWnd* pParent)
    : CDialog(IDD_SCHEDULER, pParent) {}

CSchedulerDialog::~CSchedulerDialog() = default;

BOOL CSchedulerDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    SetWindowText(L"Scheduler / Queue Manager");
    SetWindowPos(nullptr, 0, 0, 600, 450, SWP_NOMOVE | SWP_NOZORDER);
    CenterWindow();
    
    int y = 10;
    
    // Queue list (left side)
    CListBox* pQueueList = new CListBox();
    pQueueList->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
                       CRect(10, y, 180, 350), this, IDC_QUEUE_LIST);
    pQueueList->AddString(L"Main Download Queue");
    pQueueList->AddString(L"Synchronization Queue");
    pQueueList->SetCurSel(0);
    
    // Queue controls (right side)
    int rx = 200;
    
    CStatic* pLabel = new CStatic();
    pLabel->Create(L"Queue Settings:", WS_CHILD | WS_VISIBLE, 
                   CRect(rx, y, 580, y + 20), this);
    y += 30;
    
    auto addLabel = [&](const wchar_t* text) {
        CStatic* p = new CStatic();
        p->Create(text, WS_CHILD | WS_VISIBLE, CRect(rx, y, 580, y + 20), this);
        y += 25;
    };
    
    addLabel(L"Max simultaneous downloads: 3");
    addLabel(L"Start time: Not set");
    addLabel(L"Stop time: Not set");
    addLabel(L"Days: Mon Tue Wed Thu Fri Sat Sun");
    addLabel(L"Action after completion: None");
    
    y += 10;
    addLabel(L"Queue files:");
    
    // File list for the selected queue
    CListCtrl* pFileList = new CListCtrl();
    pFileList->Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
                      CRect(rx, y, 580, 350), this, IDC_QUEUE_FILES_LIST);
    pFileList->SetExtendedStyle(LVS_EX_FULLROWSELECT);
    pFileList->InsertColumn(0, L"File Name", LVCFMT_LEFT, 200);
    pFileList->InsertColumn(1, L"Status", LVCFMT_LEFT, 80);
    pFileList->InsertColumn(2, L"Size", LVCFMT_RIGHT, 80);
    
    // Buttons
    int btnY = 370;
    auto addButton = [&](const wchar_t* text, UINT id) {
        CButton* p = new CButton();
        p->Create(text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  CRect(10 + (id - IDC_QUEUE_NEW_BTN) * 100, btnY, 
                        100 + (id - IDC_QUEUE_NEW_BTN) * 100, btnY + 28), this, id);
    };
    
    addButton(L"New Queue", IDC_QUEUE_NEW_BTN);
    addButton(L"Delete Queue", IDC_QUEUE_DELETE_BTN);
    addButton(L"Start Queue", IDC_QUEUE_START_BTN);
    addButton(L"Stop Queue", IDC_QUEUE_STOP_BTN);
    
    // OK/Cancel
    CButton* pOk = new CButton();
    pOk->Create(L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                CRect(490, btnY, 580, btnY + 28), this, IDOK);
    
    return TRUE;
}

} // namespace idm
