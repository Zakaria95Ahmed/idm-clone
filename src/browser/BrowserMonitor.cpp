#include "stdafx.h"
#include "BrowserMonitor.h"
#include "../util/Logger.h"
namespace idm {
BrowserMonitor& BrowserMonitor::Instance() {
    static BrowserMonitor instance;
    return instance;
}
bool BrowserMonitor::Initialize() {
    LOG_INFO(L"BrowserMonitor: initialized (Phase 4)");
    m_initialized = true;
    return true;
}
void BrowserMonitor::Shutdown() {
    m_initialized = false;
}
} // namespace idm
