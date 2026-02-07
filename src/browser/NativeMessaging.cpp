/**
 * @file NativeMessaging.cpp
 */
#include "stdafx.h"
#include "NativeMessaging.h"
#include "../util/Logger.h"

namespace idm {

NativeMessaging& NativeMessaging::Instance() {
    static NativeMessaging instance;
    return instance;
}

bool NativeMessaging::Initialize() {
    LOG_INFO(L"NativeMessaging: initialized (Phase 4 - browser integration)");
    m_initialized = true;
    return true;
}

void NativeMessaging::Shutdown() {
    m_initialized = false;
}

bool NativeMessaging::RegisterHost() {
    // Register native messaging host manifest in registry
    // HKCU\Software\Google\Chrome\NativeMessagingHosts\com.idmclone.native
    LOG_INFO(L"NativeMessaging: host registration (Phase 4)");
    return true;
}

bool NativeMessaging::UnregisterHost() {
    return true;
}

} // namespace idm
