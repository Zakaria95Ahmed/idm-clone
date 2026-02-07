#pragma once
#include "stdafx.h"
namespace idm {
class ComServer {
public:
    static ComServer& Instance();
    bool Initialize();
    void Shutdown();
private:
    ComServer() = default;
    bool m_initialized{false};
};
} // namespace idm
