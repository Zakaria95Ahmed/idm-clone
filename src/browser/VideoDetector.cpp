#include "stdafx.h"
#include "VideoDetector.h"
#include "../util/Logger.h"
namespace idm {
VideoDetector& VideoDetector::Instance() {
    static VideoDetector instance;
    return instance;
}
bool VideoDetector::Initialize() {
    LOG_INFO(L"VideoDetector: initialized (Phase 4)");
    m_initialized = true;
    return true;
}
void VideoDetector::Shutdown() {
    m_initialized = false;
}
} // namespace idm
