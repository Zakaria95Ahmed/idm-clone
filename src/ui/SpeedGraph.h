/**
 * @file SpeedGraph.h / SpeedGraph.cpp
 * @brief Real-time download speed chart
 */

#pragma once
#include "stdafx.h"

namespace idm {

class CSpeedGraph : public CStatic {
    DECLARE_DYNAMIC(CSpeedGraph)
    
public:
    CSpeedGraph();
    virtual ~CSpeedGraph();
    
    void AddDataPoint(double bytesPerSec);
    void Clear();
    void SetMaxPoints(int maxPoints) { m_maxPoints = maxPoints; }
    
protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()
    
private:
    std::deque<double>  m_dataPoints;
    int                 m_maxPoints{120};  // 2 minutes at 1Hz
    double              m_maxSpeed{0};
    
    COLORREF    m_clrBackground{RGB(20, 20, 30)};
    COLORREF    m_clrLine{RGB(0, 200, 100)};
    COLORREF    m_clrFill{RGB(0, 100, 50)};
    COLORREF    m_clrGrid{RGB(50, 50, 60)};
    COLORREF    m_clrText{RGB(180, 180, 180)};
    COLORREF    m_clrBorder{RGB(80, 80, 90)};
};

} // namespace idm
