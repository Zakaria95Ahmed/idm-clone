/**
 * @file NativeMessaging.h / NativeMessaging.cpp
 * @brief Chrome/Firefox/Edge native messaging host (Phase 4 stub)
 */

#pragma once
#include "stdafx.h"

namespace idm {

class NativeMessaging {
public:
    static NativeMessaging& Instance();
    bool Initialize();
    void Shutdown();
    bool RegisterHost();
    bool UnregisterHost();
private:
    NativeMessaging() = default;
    bool m_initialized{false};
};

} // namespace idm
