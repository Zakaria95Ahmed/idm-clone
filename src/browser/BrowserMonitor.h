#pragma once
#include "stdafx.h"
namespace idm {
class BrowserMonitor {
public:
    static BrowserMonitor& Instance();
    bool Initialize();
    void Shutdown();
private:
    BrowserMonitor() = default;
    bool m_initialized{false};
};
} // namespace idm
