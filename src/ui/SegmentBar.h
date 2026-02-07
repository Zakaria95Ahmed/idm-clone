/**
 * @file SegmentBar.h / SegmentBar.cpp
 * @brief Color-coded segment visualization bar (IDM signature feature)
 *
 * Visual representation:
 *   Blue  = downloaded portions
 *   White = remaining portions
 *   Pink  = segment boundaries (connection split points)
 */

#pragma once
#include "stdafx.h"
#include "../core/SegmentManager.h"

namespace idm {

class CSegmentBar : public CStatic {
    DECLARE_DYNAMIC(CSegmentBar)
    
public:
    CSegmentBar();
    virtual ~CSegmentBar();
    
    void SetSegments(const std::vector<Segment>& segments, int64 fileSize);
    void Clear();
    
protected:
    afx_msg void OnPaint();
    
    DECLARE_MESSAGE_MAP()
    
private:
    std::vector<Segment>    m_segments;
    int64                   m_fileSize{0};
    
    COLORREF    m_clrDownloaded{RGB(70, 130, 220)};     // Blue
    COLORREF    m_clrRemaining{RGB(255, 255, 255)};     // White
    COLORREF    m_clrBoundary{RGB(255, 180, 200)};      // Pink
    COLORREF    m_clrActive{RGB(100, 200, 100)};        // Green (currently downloading)
    COLORREF    m_clrBorder{RGB(160, 160, 160)};        // Gray border
};

} // namespace idm
