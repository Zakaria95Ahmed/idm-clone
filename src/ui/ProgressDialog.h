/**
 * @file ProgressDialog.h / ProgressDialog.cpp
 * @brief Per-file download progress window
 *
 * Shows: overall progress, segment visualization bar, speed graph,
 * current/average speed, downloaded bytes, time remaining, connections.
 */

#pragma once
#include "stdafx.h"
#include "resource.h"
#include "SegmentBar.h"
#include "SpeedGraph.h"
#include "../util/Database.h"
#include "../core/DownloadEngine.h"

namespace idm {

class CProgressDialog : public CDialog, public IDownloadObserver {
    DECLARE_DYNAMIC(CProgressDialog)
    
public:
    CProgressDialog(const DownloadEntry& entry, CWnd* pParent = nullptr);
    virtual ~CProgressDialog();
    
    // IDownloadObserver
    void OnDownloadProgress(const String& id, int64 downloaded, 
                            int64 total, double speed) override;
    void OnDownloadSegmentUpdate(const String& id, 
                                  const std::vector<Segment>& segments) override;
    void OnDownloadComplete(const String& id) override;
    void OnDownloadError(const String& id, const String& error) override;
    
protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnCancel() override;
    
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBnClickedPause();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedOpenFolder();
    afx_msg void OnBnClickedOpenFile();
    afx_msg void OnDestroy();
    
    DECLARE_MESSAGE_MAP()
    
private:
    void UpdateDisplay();
    
    DownloadEntry   m_entry;
    CProgressCtrl   m_progressBar;
    CSegmentBar     m_segmentBar;
    CSpeedGraph     m_speedGraph;
    
    static constexpr UINT_PTR TIMER_UPDATE = 100;
};

} // namespace idm
