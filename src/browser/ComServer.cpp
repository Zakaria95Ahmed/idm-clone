#include "stdafx.h"
#include "ComServer.h"
#include "../util/Logger.h"
namespace idm {
ComServer& ComServer::Instance() {
    static ComServer instance;
    return instance;
}
bool ComServer::Initialize() {
    LOG_INFO(L"ComServer: initialized (Phase 4)");
    m_initialized = true;
    return true;
}
void ComServer::Shutdown() {
    m_initialized = false;
}
} // namespace idm
