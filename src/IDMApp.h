/**
 * @file IDMApp.h
 * @brief MFC Application class for IDM Clone
 *
 * CIDMApp is the MFC application object. It:
 *   1. Ensures single-instance behavior via a named mutex
 *   2. Initializes COM (required for shell operations and browser integration)
 *   3. Initializes the download engine
 *   4. Creates the main frame window
 *   5. Processes the command line for URL arguments (from browser integration)
 */

#pragma once
#include "stdafx.h"
#include "ui/MainFrame.h"

namespace idm {

class CIDMApp : public CWinApp {
public:
    CIDMApp();
    virtual ~CIDMApp();
    
    virtual BOOL InitInstance() override;
    virtual int ExitInstance() override;
    virtual BOOL OnIdle(LONG lCount) override;
    
private:
    bool CheckSingleInstance();
    void InitializeEngine();
    void ProcessCommandLine();
    
    HANDLE  m_hMutex{nullptr};
    bool    m_engineInitialized{false};
};

} // namespace idm
