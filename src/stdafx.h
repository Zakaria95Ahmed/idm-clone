/**
 * @file stdafx.h
 * @brief Precompiled header for the IDM Clone application
 *
 * This precompiled header aggregates all frequently-used system and MFC
 * headers. The build system compiles this once and reuses the PCH binary
 * for all translation units, dramatically reducing compile times.
 *
 * Inclusion order matters:
 *   1. Target version definitions
 *   2. MFC core headers (which pull in Windows.h)
 *   3. MFC extension headers (common controls, sockets, etc.)
 *   4. Additional Windows SDK headers
 *   5. C++ Standard Library headers
 *   6. Third-party library headers
 *
 * Design decision: We use MFC's afxwin.h instead of directly including
 * windows.h because MFC wraps and extends the Win32 API with RAII
 * wrappers, message maps, and the document/view architecture that we
 * leverage for the main UI.
 */

#pragma once

// ─── Prevent Windows.h min/max macros ──────────────────────────────────────
// NOMINMAX MUST be defined before any Windows header is included.
// Windows.h defines min/max macros that conflict with std::min/std::max
// and std::numeric_limits<T>::max(), causing C2589/C2059 compile errors.
#ifndef NOMINMAX
#define NOMINMAX
#endif

// ─── Reduce Windows header bloat ────────────────────────────────────────────
// VC_EXTRALEAN reduces MFC header inclusions (safe with MFC)
// NOTE: WIN32_LEAN_AND_MEAN is NOT used here because it conflicts with MFC's
// requirement for full Windows headers (Winsock, OLE, etc.)
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

// ─── Target platform ───────────────────────────────────────────────────────
#include "targetver.h"

// ─── MFC Core ──────────────────────────────────────────────────────────────
// afxwin.h includes windows.h and provides CWnd, CFrameWnd, CDialog, etc.
#include <afxwin.h>

// afxext.h provides CToolBar, CStatusBar, CSplitterWnd, and other 
// extended MFC UI classes that IDM's interface requires
#include <afxext.h>

// Common controls: CListCtrl, CTreeCtrl, CTabCtrl, CProgressCtrl, etc.
// These are essential for the download list, category tree, options tabs
#include <afxcmn.h>

// MFC control bars - for the dockable toolbar
#include <afxcontrolbars.h>

// OLE/COM support - needed for COM server interface, drag & drop,
// and browser integration via IDispatch
#include <afxole.h>
#include <afxdisp.h>

// Shell API integration - for system tray, file operations
#include <afxpriv.h>

// ─── Windows SDK Headers ───────────────────────────────────────────────────
// Winsock2 must come before windows.h (already included by MFC)
// so we include it here for the definitions
#include <WinSock2.h>
#include <WS2tcpip.h>

// WinHTTP - our primary HTTP/HTTPS client API
// Chosen over WinINet for better proxy support, async operations,
// and server certificate validation callbacks
#include <winhttp.h>

// WinINet - used for cookie import and FTP operations where
// WinHTTP doesn't provide equivalent functionality
#include <WinInet.h>

// Shell operations (SHGetFolderPath, SHBrowseForFolder, etc.)
#include <ShlObj.h>
#include <Shlwapi.h>

// Modern crypto API for hash computations (MD5, SHA-1, SHA-256)
#include <bcrypt.h>

// Visual styles / theming support (dark mode)
#include <Uxtheme.h>
#include <dwmapi.h>

// Version information API
#include <VersionHelpers.h>

// Multimedia API (PlaySound for event notifications)
#include <mmsystem.h>

// ─── C++ Standard Library ──────────────────────────────────────────────────
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <deque>
#include <array>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <optional>
#include <variant>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>

// ─── Link Libraries (via pragmas as backup to CMake) ───────────────────────
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

// ─── Common Type Aliases ───────────────────────────────────────────────────
// Used throughout the codebase for clarity and consistency
namespace idm {
    using int64  = std::int64_t;    // File sizes up to 2^63
    using uint64 = std::uint64_t;   // Byte counts
    using uint32 = std::uint32_t;   // Connection IDs, counters
    using uint16 = std::uint16_t;   // Port numbers
    using uint8  = std::uint8_t;    // Flags, byte buffers
    
    using String = std::wstring;    // Unicode strings throughout
    using StringView = std::wstring_view;
    
    // Smart pointer aliases
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;
    
    template<typename T>
    using SharedPtr = std::shared_ptr<T>;
    
    template<typename T>
    using WeakPtr = std::weak_ptr<T>;
    
    // Thread synchronization aliases
    using Mutex = std::mutex;
    using RecursiveMutex = std::recursive_mutex;
    using Lock = std::unique_lock<std::mutex>;
    using RecursiveLock = std::unique_lock<std::recursive_mutex>;
    using CondVar = std::condition_variable;
    
    // Time types
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using SystemClock = std::chrono::system_clock;
    using SystemTimePoint = SystemClock::time_point;
}

// ─── Global Constants ──────────────────────────────────────────────────────
namespace idm::constants {
    // Application identity
    constexpr wchar_t APP_NAME[]         = L"Internet Download Manager";
    constexpr wchar_t APP_VERSION[]      = L"6.42 Build 1";
    constexpr wchar_t APP_TITLE[]        = L"Internet Download Manager 6.42";
    constexpr wchar_t APP_MUTEX_NAME[]   = L"Global\\IDMClone_SingleInstance_Mutex";
    constexpr wchar_t APP_CLASS_NAME[]   = L"IDMCloneMainWnd";
    
    // Network constants
    constexpr int DEFAULT_MAX_CONNECTIONS    = 8;
    constexpr int MIN_CONNECTIONS            = 1;
    constexpr int MAX_CONNECTIONS            = 32;
    constexpr int DEFAULT_TIMEOUT_SECONDS    = 30;
    constexpr int DEFAULT_RETRY_COUNT        = 20;
    constexpr int DEFAULT_RETRY_DELAY_SEC    = 5;
    constexpr int BUFFER_SIZE                = 65536;  // 64KB read buffer
    constexpr int MIN_SEGMENT_SIZE           = 65536;  // 64KB minimum segment
    constexpr int64 MAX_FILE_SIZE            = INT64_MAX; // 2^63 - 1 bytes
    
    // State persistence intervals
    constexpr int SEGMENT_SAVE_INTERVAL_MS   = 15000;  // Save segment state every 15s
    constexpr int SPEED_SAMPLE_INTERVAL_MS   = 1000;   // Speed sampling every 1s
    constexpr int UI_UPDATE_INTERVAL_MS      = 250;    // UI refresh every 250ms
    
    // File extensions for auto-capture (matching IDM's default list)
    constexpr wchar_t DEFAULT_FILE_TYPES[] = 
        L".exe .zip .rar .7z .mp3 .mp4 .avi .mkv .pdf .doc .iso .torrent "
        L".flv .wmv .mov .wav .aac .flac .gz .bz2 .tar .xz .msi .dmg "
        L".apk .deb .rpm .bin .img .cab .docx .xlsx .pptx .webm .m4v "
        L".opus .ogg .wma .m4a .3gp .mpg .mpeg";
    
    // User-Agent string (matching IDM's actual UA)
    constexpr wchar_t DEFAULT_USER_AGENT[] = 
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        L"(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
    
    // Registry paths
    constexpr wchar_t REG_ROOT_KEY[]       = L"Software\\IDMClone";
    constexpr wchar_t REG_DOWNLOADS_KEY[]  = L"Software\\IDMClone\\Downloads";
    constexpr wchar_t REG_OPTIONS_KEY[]    = L"Software\\IDMClone\\Options";
    constexpr wchar_t REG_QUEUES_KEY[]     = L"Software\\IDMClone\\Queues";
}
