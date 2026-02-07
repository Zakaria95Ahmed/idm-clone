/**
 * @file SpeedGraph.cpp
 * @brief Real-time speed graph implementation with GDI rendering
 */

#include "stdafx.h"
#include "SpeedGraph.h"
#include "../util/Unicode.h"

namespace idm {

IMPLEMENT_DYNAMIC(CSpeedGraph, CStatic)

BEGIN_MESSAGE_MAP(CSpeedGraph, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

CSpeedGraph::CSpeedGraph() = default;
CSpeedGraph::~CSpeedGraph() = default;

void CSpeedGraph::AddDataPoint(double bytesPerSec) {
    m_dataPoints.push_back(bytesPerSec);
    while (static_cast<int>(m_dataPoints.size()) > m_maxPoints) {
        m_dataPoints.pop_front();
    }
    
    // Track max for auto-scaling
    m_maxSpeed = 0;
    for (double d : m_dataPoints) {
        if (d > m_maxSpeed) m_maxSpeed = d;
    }
    // Add 10% headroom
    m_maxSpeed *= 1.1;
    if (m_maxSpeed < 1024) m_maxSpeed = 1024; // Minimum 1 KB/s scale
    
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void CSpeedGraph::Clear() {
    m_dataPoints.clear();
    m_maxSpeed = 0;
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void CSpeedGraph::OnPaint() {
    CPaintDC dc(this);
    CRect rect;
    GetClientRect(&rect);
    
    // Double buffer
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBmp;
    memBmp.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBmp);
    
    // Dark background
    memDC.FillSolidRect(rect, m_clrBackground);
    
    // Draw grid lines
    CPen gridPen(PS_DOT, 1, m_clrGrid);
    CPen* pOldPen = memDC.SelectObject(&gridPen);
    
    int gridLines = 4;
    for (int i = 1; i < gridLines; ++i) {
        int y = rect.top + (rect.Height() * i / gridLines);
        memDC.MoveTo(rect.left, y);
        memDC.LineTo(rect.right, y);
    }
    
    // Draw speed labels on Y axis
    memDC.SetBkMode(TRANSPARENT);
    memDC.SetTextColor(m_clrText);
    CFont font;
    font.CreatePointFont(70, L"Segoe UI");
    CFont* pOldFont = memDC.SelectObject(&font);
    
    for (int i = 0; i <= gridLines; ++i) {
        int y = rect.top + (rect.Height() * i / gridLines);
        double speed = m_maxSpeed * (1.0 - static_cast<double>(i) / gridLines);
        CString label;
        label.Format(L"%s", Unicode::FormatSpeed(speed).c_str());
        memDC.TextOut(rect.left + 2, y, label);
    }
    
    // Draw speed line
    if (m_dataPoints.size() >= 2 && m_maxSpeed > 0) {
        int drawWidth = rect.Width() - 60;  // Leave space for labels
        int drawLeft = rect.left + 55;
        int drawHeight = rect.Height() - 10;
        int drawTop = rect.top + 5;
        
        // Fill area under the curve
        CPen linePen(PS_SOLID, 2, m_clrLine);
        memDC.SelectObject(&linePen);
        
        // Create filled polygon
        std::vector<CPoint> fillPoints;
        fillPoints.reserve(m_dataPoints.size() + 2);
        
        double xStep = static_cast<double>(drawWidth) / (m_maxPoints - 1);
        int startIdx = static_cast<int>(m_maxPoints - m_dataPoints.size());
        
        // Bottom-left corner
        fillPoints.push_back(CPoint(drawLeft + static_cast<int>(startIdx * xStep), 
                                     drawTop + drawHeight));
        
        for (size_t i = 0; i < m_dataPoints.size(); ++i) {
            int x = drawLeft + static_cast<int>((startIdx + i) * xStep);
            int y = drawTop + drawHeight - 
                    static_cast<int>(m_dataPoints[i] / m_maxSpeed * drawHeight);
            y = std::clamp(y, drawTop, drawTop + drawHeight);
            fillPoints.push_back(CPoint(x, y));
        }
        
        // Bottom-right corner
        int lastX = drawLeft + static_cast<int>((startIdx + m_dataPoints.size() - 1) * xStep);
        fillPoints.push_back(CPoint(lastX, drawTop + drawHeight));
        
        // Fill the area
        CBrush fillBrush(m_clrFill);
        CBrush* pOldBrush = memDC.SelectObject(&fillBrush);
        CPen fillPen(PS_NULL, 0, RGB(0,0,0));
        memDC.SelectObject(&fillPen);
        memDC.Polygon(fillPoints.data(), static_cast<int>(fillPoints.size()));
        
        // Draw the line on top
        memDC.SelectObject(&linePen);
        for (size_t i = 0; i < m_dataPoints.size(); ++i) {
            int x = drawLeft + static_cast<int>((startIdx + i) * xStep);
            int y = drawTop + drawHeight - 
                    static_cast<int>(m_dataPoints[i] / m_maxSpeed * drawHeight);
            y = std::clamp(y, drawTop, drawTop + drawHeight);
            
            if (i == 0) memDC.MoveTo(x, y);
            else memDC.LineTo(x, y);
        }
        
        memDC.SelectObject(pOldBrush);
    }
    
    // Draw border
    memDC.Draw3dRect(rect, m_clrBorder, m_clrBorder);
    
    memDC.SelectObject(pOldFont);
    memDC.SelectObject(pOldPen);
    
    // Blit
    dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

} // namespace idm
