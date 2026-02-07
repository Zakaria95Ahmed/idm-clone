/**
 * @file CategoryTreeView.cpp
 * @brief Category tree implementation matching IDM's left panel
 */

#include "stdafx.h"
#include "CategoryTreeView.h"
#include "../core/DownloadEngine.h"

namespace idm {

IMPLEMENT_DYNAMIC(CategoryTreeView, CTreeCtrl)

BEGIN_MESSAGE_MAP(CategoryTreeView, CTreeCtrl)
    ON_WM_CREATE()
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CategoryTreeView::OnTvnSelchanged)
END_MESSAGE_MAP()

CategoryTreeView::CategoryTreeView() = default;
CategoryTreeView::~CategoryTreeView() = default;

int CategoryTreeView::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CTreeCtrl::OnCreate(lpCreateStruct) == -1) return -1;
    Initialize();
    return 0;
}

void CategoryTreeView::Initialize() {
    // Create image list for tree icons
    m_imageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 12, 4);
    
    // Add system icons as placeholders (in production, use custom icons)
    HICON hIcon = (HICON)::LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 16, 16, LR_SHARED);
    for (int i = 0; i < 12; ++i) {
        m_imageList.Add(hIcon);
    }
    
    SetImageList(&m_imageList, TVSIL_NORMAL);
    
    PopulateTree();
}

void CategoryTreeView::PopulateTree() {
    // Build the category tree matching IDM's structure
    m_hAll = InsertItem(L"All Downloads", 0, 0, TVI_ROOT);
    m_hFinished = InsertItem(L"Finished", 1, 1, TVI_ROOT);
    m_hUnfinished = InsertItem(L"Unfinished", 2, 2, TVI_ROOT);
    m_hQueued = InsertItem(L"Queued", 3, 3, TVI_ROOT);
    m_hGrabber = InsertItem(L"Grabber Projects", 4, 4, TVI_ROOT);
    
    // Categories branch
    m_hCategories = InsertItem(L"Categories", 5, 5, TVI_ROOT);
    m_hMusic = InsertItem(L"Music", 6, 6, m_hCategories);
    m_hVideo = InsertItem(L"Video", 7, 7, m_hCategories);
    m_hPrograms = InsertItem(L"Programs", 8, 8, m_hCategories);
    m_hDocuments = InsertItem(L"Documents", 9, 9, m_hCategories);
    m_hCompressed = InsertItem(L"Compressed", 10, 10, m_hCategories);
    
    // Expand all root items
    Expand(m_hCategories, TVE_EXPAND);
    
    // Select "All Downloads" by default
    SelectItem(m_hAll);
    
    // Set item data for filtering
    SetItemData(m_hAll, 0);
    SetItemData(m_hFinished, 1);
    SetItemData(m_hUnfinished, 2);
    SetItemData(m_hQueued, 3);
    SetItemData(m_hMusic, 10);
    SetItemData(m_hVideo, 11);
    SetItemData(m_hPrograms, 12);
    SetItemData(m_hDocuments, 13);
    SetItemData(m_hCompressed, 14);
}

String CategoryTreeView::GetSelectedCategory() const {
    HTREEITEM hSel = GetSelectedItem();
    if (!hSel) return L"All";
    
    CString text = GetItemText(hSel);
    return String(text.GetString());
}

void CategoryTreeView::UpdateCounts() {
    auto& engine = DownloadEngine::Instance();
    auto& db = engine.GetDatabase();
    
    // Update item text with counts
    CString text;
    
    text.Format(L"All Downloads (%d)", db.GetTotalCount());
    SetItemText(m_hAll, text);
    
    text.Format(L"Finished (%d)", db.GetCountByStatus(DownloadStatus::Complete));
    SetItemText(m_hFinished, text);
    
    int unfinished = db.GetCountByStatus(DownloadStatus::Paused) + 
                     db.GetCountByStatus(DownloadStatus::Error);
    text.Format(L"Unfinished (%d)", unfinished);
    SetItemText(m_hUnfinished, text);
    
    text.Format(L"Queued (%d)", db.GetCountByStatus(DownloadStatus::Queued));
    SetItemText(m_hQueued, text);
}

void CategoryTreeView::OnTvnSelchanged(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    // Notify the main frame to filter the download list
    CWnd* pParent = GetParent();
    if (pParent) {
        pParent->SendMessage(WM_APP_REFRESH_LIST, 0, 0);
    }
    *pResult = 0;
}

} // namespace idm
