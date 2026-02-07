/**
 * @file CategoryTreeView.h
 * @brief Category tree panel (left side of the splitter)
 */

#pragma once
#include "stdafx.h"
#include "resource.h"

namespace idm {

class CategoryTreeView : public CTreeCtrl {
    DECLARE_DYNAMIC(CategoryTreeView)
    
public:
    CategoryTreeView();
    virtual ~CategoryTreeView();
    
    void Initialize();
    String GetSelectedCategory() const;
    void UpdateCounts();
    
protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    
    DECLARE_MESSAGE_MAP()
    
private:
    void PopulateTree();
    
    CImageList  m_imageList;
    HTREEITEM   m_hAll{nullptr};
    HTREEITEM   m_hFinished{nullptr};
    HTREEITEM   m_hUnfinished{nullptr};
    HTREEITEM   m_hQueued{nullptr};
    HTREEITEM   m_hGrabber{nullptr};
    HTREEITEM   m_hCategories{nullptr};
    HTREEITEM   m_hMusic{nullptr};
    HTREEITEM   m_hVideo{nullptr};
    HTREEITEM   m_hPrograms{nullptr};
    HTREEITEM   m_hDocuments{nullptr};
    HTREEITEM   m_hCompressed{nullptr};
};

} // namespace idm
