/**
 * @file SegmentBar.cpp
 * @brief Segment visualization bar implementation
 */

#include "stdafx.h"
#include "SegmentBar.h"

namespace idm {

IMPLEMENT_DYNAMIC(CSegmentBar, CStatic)

BEGIN_MESSAGE_MAP(CSegmentBar, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

CSegmentBar::CSegmentBar() = default;
CSegmentBar::~CSegmentBar() = default;

void CSegmentBar::SetSegments(const std::vector<Segment>& segments, int64 fileSize) {
    m_segments = segments;
    m_fileSize = fileSize;
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void CSegmentBar::Clear() {
    m_segments.clear();
    m_fileSize = 0;
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void CSegmentBar::OnPaint() {
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);
    
    // Create memory DC for double-buffered drawing
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBmp;
    memBmp.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBmp);
    
    // Fill background
    memDC.FillSolidRect(rect, m_clrRemaining);
    
    if (m_fileSize > 0 && !m_segments.empty()) {
        double pixelsPerByte = static_cast<double>(rect.Width()) / m_fileSize;
        
        for (const auto& seg : m_segments) {
            // Calculate pixel positions
            int segStartPx = static_cast<int>(seg.startByte * pixelsPerByte);
            int segEndPx = static_cast<int>((seg.endByte + 1) * pixelsPerByte);
            int downloadedPx = static_cast<int>(seg.currentPos * pixelsPerByte);
            
            // Clamp to rect
            segStartPx = (std::clamp)(segStartPx, 0, rect.Width());
            segEndPx = (std::clamp)(segEndPx, 0, rect.Width());
            downloadedPx = (std::clamp)(downloadedPx, segStartPx, segEndPx);
            
            // Draw downloaded portion (blue)
            if (downloadedPx > segStartPx) {
                CRect dlRect(segStartPx, rect.top, downloadedPx, rect.bottom);
                COLORREF color = m_clrDownloaded;
                if (seg.status == SegmentStatus::Active) {
                    color = m_clrActive;
                }
                memDC.FillSolidRect(dlRect, color);
            }
            
            // Draw segment boundary marker (pink line)
            if (segStartPx > 0) {
                CRect boundaryRect(segStartPx - 1, rect.top, segStartPx + 1, rect.bottom);
                memDC.FillSolidRect(boundaryRect, m_clrBoundary);
            }
        }
    }
    
    // Draw border
    memDC.Draw3dRect(rect, m_clrBorder, m_clrBorder);
    
    // Blit to screen
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

} // namespace idm
