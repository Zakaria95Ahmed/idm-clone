/**
 * @file DownloadListView.h
 * @brief Download list control (right side of the splitter)
 *
 * Implements the main download list with all IDM columns:
 * Row #, File Name, Status, Size, Transfer Rate, Time Left,
 * % Complete (with inline progress bar), Description, Date Added,
 * Date Completed, URL, Save To, Queue indicator.
 */

#pragma once
#include "stdafx.h"
#include "resource.h"
#include "../util/Database.h"

namespace idm {

class DownloadListView : public CListCtrl {
    DECLARE_DYNAMIC(DownloadListView)
    
public:
    DownloadListView();
    virtual ~DownloadListView();
    
    void Initialize();
    void RefreshFromDatabase();
    void UpdateProgress();
    void SelectAll();
    std::vector<String> GetSelectedDownloadIds() const;
    
    // Filtering
    void SetFilter(const String& category);
    void ClearFilter();
    
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnNMRClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    
    DECLARE_MESSAGE_MAP()
    
private:
    void SetupColumns();
    void AddDownloadToList(const DownloadEntry& entry, int index);
    void UpdateDownloadRow(int index, const DownloadEntry& entry);
    void ShowContextMenu(CPoint point);
    
    // Column indices
    enum Column {
        COL_ROW_NUM = 0,
        COL_FILENAME,
        COL_STATUS,
        COL_SIZE,
        COL_SPEED,
        COL_TIME_LEFT,
        COL_PROGRESS,
        COL_DESCRIPTION,
        COL_DATE_ADDED,
        COL_DATE_COMPLETED,
        COL_URL,
        COL_SAVE_TO,
        COL_QUEUE,
        COL_COUNT
    };
    
    // Sort state
    int     m_sortColumn{-1};
    bool    m_sortAscending{true};
    
    // Filter
    String  m_filterCategory;
    
    // Cached download IDs (parallel to list items)
    std::vector<String>  m_downloadIds;
    
    // Custom draw colors
    COLORREF m_clrBackground{RGB(255, 255, 255)};
    COLORREF m_clrText{RGB(0, 0, 0)};
    COLORREF m_clrProgressBar{RGB(76, 175, 80)};
    COLORREF m_clrProgressBg{RGB(224, 224, 224)};
    COLORREF m_clrComplete{RGB(0, 128, 0)};
    COLORREF m_clrError{RGB(200, 0, 0)};
    COLORREF m_clrPaused{RGB(128, 128, 0)};
    COLORREF m_clrDownloading{RGB(0, 0, 200)};
};

} // namespace idm
