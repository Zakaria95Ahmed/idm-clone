#pragma once
#include "stdafx.h"
namespace idm {
class VideoDetector {
public:
    static VideoDetector& Instance();
    bool Initialize();
    void Shutdown();
private:
    VideoDetector() = default;
    bool m_initialized{false};
};
} // namespace idm
