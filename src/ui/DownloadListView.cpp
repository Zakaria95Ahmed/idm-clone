/**
 * @file DownloadListView.cpp
 * @brief Download list implementation with custom drawing
 *
 * The list uses LVS_REPORT style with custom-drawn progress bars
 * in the progress column, matching IDM's inline progress visualization.
 */

#include "stdafx.h"
#include "DownloadListView.h"
#include "../core/DownloadEngine.h"
#include "../util/Unicode.h"
#include "../util/Logger.h"
#include "resource.h"

namespace idm {

IMPLEMENT_DYNAMIC(DownloadListView, CListCtrl)

BEGIN_MESSAGE_MAP(DownloadListView, CListCtrl)
    ON_WM_CREATE()
    ON_NOTIFY_REFLECT(NM_DBLCLK, &DownloadListView::OnNMDblclk)
    ON_NOTIFY_REFLECT(NM_RCLICK, &DownloadListView::OnNMRClick)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &DownloadListView::OnLvnColumnClick)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &DownloadListView::OnCustomDraw)
END_MESSAGE_MAP()

DownloadListView::DownloadListView() = default;
DownloadListView::~DownloadListView() = default;

int DownloadListView::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CListCtrl::OnCreate(lpCreateStruct) == -1) return -1;
    Initialize();
    return 0;
}

void DownloadListView::Initialize() {
    // Extended styles: full row select, grid lines, checkboxes
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | 
                     LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER);
    
    SetupColumns();
    RefreshFromDatabase();
}

void DownloadListView::SetupColumns() {
    // Matching IDM's column layout with configurable widths
    InsertColumn(COL_ROW_NUM,       L"#",             LVCFMT_RIGHT,  35);
    InsertColumn(COL_FILENAME,      L"File Name",     LVCFMT_LEFT,   200);
    InsertColumn(COL_STATUS,        L"Status",        LVCFMT_LEFT,   100);
    InsertColumn(COL_SIZE,          L"Size",          LVCFMT_RIGHT,  80);
    InsertColumn(COL_SPEED,         L"Transfer Rate", LVCFMT_RIGHT,  100);
    InsertColumn(COL_TIME_LEFT,     L"Time Left",     LVCFMT_RIGHT,  80);
    InsertColumn(COL_PROGRESS,      L"% Complete",    LVCFMT_LEFT,   120);
    InsertColumn(COL_DESCRIPTION,   L"Description",   LVCFMT_LEFT,   100);
    InsertColumn(COL_DATE_ADDED,    L"Date Added",    LVCFMT_LEFT,   130);
    InsertColumn(COL_DATE_COMPLETED,L"Date Completed",LVCFMT_LEFT,   130);
    InsertColumn(COL_URL,           L"URL",           LVCFMT_LEFT,   250);
    InsertColumn(COL_SAVE_TO,       L"Save To",       LVCFMT_LEFT,   150);
    InsertColumn(COL_QUEUE,         L"Q",             LVCFMT_CENTER, 30);
}

void DownloadListView::RefreshFromDatabase() {
    // Preserve selection
    auto selectedIds = GetSelectedDownloadIds();
    
    SetRedraw(FALSE);
    DeleteAllItems();
    m_downloadIds.clear();
    
    auto entries = DownloadEngine::Instance().GetAllDownloads();
    
    // Apply filter
    int index = 0;
    for (const auto& entry : entries) {
        if (!m_filterCategory.empty() && m_filterCategory != L"All Downloads") {
            if (m_filterCategory == L"Finished" && entry.status != DownloadStatus::Complete) continue;
            if (m_filterCategory == L"Unfinished" && entry.status == DownloadStatus::Complete) continue;
            if (m_filterCategory == L"Queued" && entry.status != DownloadStatus::Queued) continue;
            if (m_filterCategory == L"Music" && entry.category != L"Music") continue;
            if (m_filterCategory == L"Video" && entry.category != L"Video") continue;
            if (m_filterCategory == L"Programs" && entry.category != L"Programs") continue;
            if (m_filterCategory == L"Documents" && entry.category != L"Documents") continue;
            if (m_filterCategory == L"Compressed" && entry.category != L"Compressed") continue;
        }
        
        AddDownloadToList(entry, index);
        m_downloadIds.push_back(entry.id);
        index++;
    }
    
    SetRedraw(TRUE);
    Invalidate();
    
    // Restore selection
    for (int i = 0; i < GetItemCount(); ++i) {
        if (i < static_cast<int>(m_downloadIds.size())) {
            for (const auto& selId : selectedIds) {
                if (m_downloadIds[i] == selId) {
                    SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                }
            }
        }
    }
}

void DownloadListView::AddDownloadToList(const DownloadEntry& entry, int index) {
    // Row number
    CString rowNum;
    rowNum.Format(L"%d", index + 1);
    int nItem = InsertItem(index, rowNum);
    
    // File name
    SetItemText(nItem, COL_FILENAME, entry.fileName.c_str());
    
    // Status
    SetItemText(nItem, COL_STATUS, entry.StatusString().c_str());
    
    // Size
    SetItemText(nItem, COL_SIZE, Unicode::FormatFileSize(entry.fileSize).c_str());
    
    // Speed
    if (entry.status == DownloadStatus::Downloading) {
        SetItemText(nItem, COL_SPEED, Unicode::FormatSpeed(entry.currentSpeed).c_str());
    } else {
        SetItemText(nItem, COL_SPEED, L"");
    }
    
    // Time left
    if (entry.status == DownloadStatus::Downloading && entry.currentSpeed > 0 && entry.fileSize > 0) {
        double remaining = static_cast<double>(entry.fileSize - entry.downloadedBytes) / entry.currentSpeed;
        SetItemText(nItem, COL_TIME_LEFT, Unicode::FormatTimeRemaining(remaining).c_str());
    } else {
        SetItemText(nItem, COL_TIME_LEFT, L"");
    }
    
    // Progress percentage
    CString progress;
    if (entry.fileSize > 0) {
        progress.Format(L"%.1f%%", entry.ProgressPercent());
    } else if (entry.status == DownloadStatus::Complete) {
        progress = L"100%";
    }
    SetItemText(nItem, COL_PROGRESS, progress);
    
    // Description
    SetItemText(nItem, COL_DESCRIPTION, entry.description.c_str());
    
    // Date Added
    SetItemText(nItem, COL_DATE_ADDED, Unicode::FormatDateTime(entry.dateAdded).c_str());
    
    // Date Completed
    if (entry.status == DownloadStatus::Complete) {
        SetItemText(nItem, COL_DATE_COMPLETED, 
                    Unicode::FormatDateTime(entry.dateCompleted).c_str());
    }
    
    // URL
    SetItemText(nItem, COL_URL, entry.url.c_str());
    
    // Save To
    SetItemText(nItem, COL_SAVE_TO, entry.savePath.c_str());
    
    // Queue indicator
    SetItemText(nItem, COL_QUEUE, entry.queueId.empty() ? L"" : L"Q");
}

void DownloadListView::UpdateProgress() {
    // Fast-path update: only refresh speed, time, and progress columns
    auto entries = DownloadEngine::Instance().GetAllDownloads();
    
    for (int i = 0; i < GetItemCount() && i < static_cast<int>(m_downloadIds.size()); ++i) {
        auto entryOpt = DownloadEngine::Instance().GetDownload(m_downloadIds[i]);
        if (!entryOpt.has_value()) continue;
        
        const auto& entry = entryOpt.value();
        
        SetItemText(i, COL_STATUS, entry.StatusString().c_str());
        
        if (entry.status == DownloadStatus::Downloading) {
            SetItemText(i, COL_SPEED, Unicode::FormatSpeed(entry.currentSpeed).c_str());
            
            if (entry.currentSpeed > 0 && entry.fileSize > 0) {
                double remaining = static_cast<double>(entry.fileSize - entry.downloadedBytes) / entry.currentSpeed;
                SetItemText(i, COL_TIME_LEFT, Unicode::FormatTimeRemaining(remaining).c_str());
            }
        } else {
            SetItemText(i, COL_SPEED, L"");
            SetItemText(i, COL_TIME_LEFT, L"");
        }
        
        if (entry.fileSize > 0) {
            CString progress;
            progress.Format(L"%.1f%%", entry.ProgressPercent());
            SetItemText(i, COL_PROGRESS, progress);
        }
    }
}

void DownloadListView::SelectAll() {
    for (int i = 0; i < GetItemCount(); ++i) {
        SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
    }
}

std::vector<String> DownloadListView::GetSelectedDownloadIds() const {
    std::vector<String> ids;
    POSITION pos = GetFirstSelectedItemPosition();
    while (pos) {
        int nItem = GetNextSelectedItem(pos);
        if (nItem >= 0 && nItem < static_cast<int>(m_downloadIds.size())) {
            ids.push_back(m_downloadIds[nItem]);
        }
    }
    return ids;
}

void DownloadListView::SetFilter(const String& category) {
    m_filterCategory = category;
    RefreshFromDatabase();
}

void DownloadListView::ClearFilter() {
    m_filterCategory.clear();
    RefreshFromDatabase();
}

// ─── Custom Draw (Progress Bar in Progress Column) ─────────────────────────
void DownloadListView::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
    *pResult = CDRF_DODEFAULT;
    
    switch (pCD->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
            *pResult = CDRF_NOTIFYITEMDRAW;
            break;
            
        case CDDS_ITEMPREPAINT:
            *pResult = CDRF_NOTIFYSUBITEMDRAW;
            break;
            
        case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            if (pCD->iSubItem == COL_PROGRESS) {
                // Draw custom progress bar
                int nItem = static_cast<int>(pCD->nmcd.dwItemSpec);
                if (nItem >= 0 && nItem < static_cast<int>(m_downloadIds.size())) {
                    auto entry = DownloadEngine::Instance().GetDownload(m_downloadIds[nItem]);
                    if (entry.has_value() && entry->fileSize > 0) {
                        CRect rect;
                        GetSubItemRect(nItem, COL_PROGRESS, LVIR_BOUNDS, rect);
                        rect.DeflateRect(2, 2);
                        
                        CDC* pDC = CDC::FromHandle(pCD->nmcd.hdc);
                        
                        // Background
                        pDC->FillSolidRect(rect, m_clrProgressBg);
                        
                        // Progress fill
                        double progress = entry->ProgressPercent() / 100.0;
                        CRect fillRect = rect;
                        fillRect.right = fillRect.left + 
                            static_cast<int>(fillRect.Width() * progress);
                        
                        COLORREF fillColor = m_clrProgressBar;
                        if (entry->status == DownloadStatus::Complete) fillColor = m_clrComplete;
                        else if (entry->status == DownloadStatus::Error) fillColor = m_clrError;
                        else if (entry->status == DownloadStatus::Paused) fillColor = m_clrPaused;
                        
                        pDC->FillSolidRect(fillRect, fillColor);
                        
                        // Draw percentage text centered
                        CString text;
                        text.Format(L"%.1f%%", entry->ProgressPercent());
                        pDC->SetBkMode(TRANSPARENT);
                        pDC->SetTextColor(RGB(0, 0, 0));
                        pDC->DrawText(text, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                        
                        *pResult = CDRF_SKIPDEFAULT;
                        return;
                    }
                }
            }
            
            // Color status text
            if (pCD->iSubItem == COL_STATUS) {
                int nItem = static_cast<int>(pCD->nmcd.dwItemSpec);
                if (nItem >= 0 && nItem < static_cast<int>(m_downloadIds.size())) {
                    auto entry = DownloadEngine::Instance().GetDownload(m_downloadIds[nItem]);
                    if (entry.has_value()) {
                        switch (entry->status) {
                            case DownloadStatus::Complete:    pCD->clrText = m_clrComplete; break;
                            case DownloadStatus::Error:       pCD->clrText = m_clrError; break;
                            case DownloadStatus::Paused:      pCD->clrText = m_clrPaused; break;
                            case DownloadStatus::Downloading: pCD->clrText = m_clrDownloading; break;
                            default: break;
                        }
                    }
                }
            }
            
            *pResult = CDRF_DODEFAULT;
            break;
    }
}

// ─── Event Handlers ────────────────────────────────────────────────────────
void DownloadListView::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    auto selectedIds = GetSelectedDownloadIds();
    if (!selectedIds.empty()) {
        auto entry = DownloadEngine::Instance().GetDownload(selectedIds[0]);
        if (entry.has_value()) {
            if (entry->status == DownloadStatus::Complete) {
                // Open the file
                ::ShellExecuteW(GetSafeHwnd(), L"open", entry->FullPath().c_str(),
                                nullptr, nullptr, SW_SHOWNORMAL);
            } else {
                // Show properties / resume
                DownloadEngine::Instance().StartDownload(selectedIds[0]);
            }
        }
    }
    *pResult = 0;
}

void DownloadListView::OnNMRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    CPoint pt;
    ::GetCursorPos(&pt);
    ShowContextMenu(pt);
    *pResult = 0;
}

void DownloadListView::OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) {
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    
    if (m_sortColumn == pNMLV->iSubItem) {
        m_sortAscending = !m_sortAscending;
    } else {
        m_sortColumn = pNMLV->iSubItem;
        m_sortAscending = true;
    }
    
    // Re-sort and refresh
    RefreshFromDatabase();
    
    *pResult = 0;
}

void DownloadListView::ShowContextMenu(CPoint point) {
    CMenu contextMenu;
    contextMenu.CreatePopupMenu();
    
    bool hasSelection = !GetSelectedDownloadIds().empty();
    
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_RESUME, L"&Resume");
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_STOP, L"&Stop");
    contextMenu.AppendMenu(MF_SEPARATOR);
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_DELETE, L"&Delete");
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_DELETE_WITH_FILE, L"Delete with &File");
    contextMenu.AppendMenu(MF_SEPARATOR);
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_OPEN_FILE, L"&Open File");
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_OPEN_FOLDER, L"Open &Folder");
    contextMenu.AppendMenu(MF_SEPARATOR);
    contextMenu.AppendMenu(MF_STRING, ID_DL_SELECT_ALL, L"Select &All");
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? 0 : MF_GRAYED), 
                           ID_DL_PROPERTIES, L"&Properties...");
    
    contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                                point.x, point.y, GetParent());
}

} // namespace idm
