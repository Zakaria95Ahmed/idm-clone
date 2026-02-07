################################################################################
# IDM Clone - Internet Download Manager Recreation
# 
# Production-grade download manager built with C++17, Win32 API, and MFC.
# Faithfully replicates IDM's core architecture: multi-threaded download engine
# with dynamic segmentation, HTTP/HTTPS/FTP support, resume capability, and
# a full-featured MFC-based user interface.
#
# Build Requirements:
#   - Visual Studio 2022 (v143 toolset)
#   - Windows SDK 10.0+
#   - CMake 3.20+
#
# Build Instructions:
#   mkdir build && cd build
#   cmake -G "Visual Studio 17 2022" -A x64 ..
#   cmake --build . --config Release
################################################################################

cmake_minimum_required(VERSION 3.20)
project(IDMClone VERSION 6.42.0 LANGUAGES CXX C)

# ─── C++ Standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Windows-specific configuration ─────────────────────────────────────────
if(WIN32)
    # Use MFC as shared DLL (or static, depending on deployment preference)
    set(CMAKE_MFC_FLAG 2)  # 1=static MFC, 2=shared MFC DLL
    
    # Unicode build (matching IDM's Unicode support)
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Windows version targeting (Windows 7+)
    add_definitions(-DWINVER=0x0601 -D_WIN32_WINNT=0x0601)
    add_definitions(-D_WIN32_IE=0x0A00)
    
    # MFC and ATL definitions
    add_definitions(-D_AFXDLL -D_ATL_CSTRING_EXPLICIT_CONSTRUCTORS)
    
    # Suppress CRT security warnings for cross-platform compat headers
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
    
    # Application version macros
    add_definitions(
        -DIDM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -DIDM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -DIDM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        -DIDM_VERSION_STRING="6.42 Build 1"
    )
endif()

# ─── Compiler flags ─────────────────────────────────────────────────────────
if(MSVC)
    # /W4 = high warning level, /WX- = don't treat warnings as errors (for dev)
    # /MP = multi-processor compilation
    # /Zi = program database for debugging
    # /EHsc = C++ exception handling
    # /permissive- = strict conformance mode
    add_compile_options(/W4 /WX- /MP /EHsc /permissive-)
    
    # Release optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Gy /Gw")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    
    # Debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /RTC1")
endif()

# ─── Source Files ────────────────────────────────────────────────────────────

# Utility layer - foundational classes used by all other layers
set(UTIL_SOURCES
    src/util/Logger.cpp
    src/util/Logger.h
    src/util/Database.cpp
    src/util/Database.h
    src/util/Registry.cpp
    src/util/Registry.h
    src/util/Crypto.cpp
    src/util/Crypto.h
    src/util/Unicode.cpp
    src/util/Unicode.h
)

# Core download engine - the heart of the application
set(CORE_SOURCES
    src/core/HttpClient.cpp
    src/core/HttpClient.h
    src/core/FtpClient.cpp
    src/core/FtpClient.h
    src/core/SegmentManager.cpp
    src/core/SegmentManager.h
    src/core/ResumeEngine.cpp
    src/core/ResumeEngine.h
    src/core/FileAssembler.cpp
    src/core/FileAssembler.h
    src/core/ConnectionPool.cpp
    src/core/ConnectionPool.h
    src/core/ProxyManager.cpp
    src/core/ProxyManager.h
    src/core/AuthManager.cpp
    src/core/AuthManager.h
    src/core/CookieJar.cpp
    src/core/CookieJar.h
    src/core/SpeedLimiter.cpp
    src/core/SpeedLimiter.h
    src/core/DownloadEngine.cpp
    src/core/DownloadEngine.h
)

# UI layer - MFC-based user interface
set(UI_SOURCES
    src/ui/MainFrame.cpp
    src/ui/MainFrame.h
    src/ui/DownloadListView.cpp
    src/ui/DownloadListView.h
    src/ui/CategoryTreeView.cpp
    src/ui/CategoryTreeView.h
    src/ui/ProgressDialog.cpp
    src/ui/ProgressDialog.h
    src/ui/AddUrlDialog.cpp
    src/ui/AddUrlDialog.h
    src/ui/OptionsDialog.cpp
    src/ui/OptionsDialog.h
    src/ui/SchedulerDialog.cpp
    src/ui/SchedulerDialog.h
    src/ui/BatchDownloadDialog.cpp
    src/ui/BatchDownloadDialog.h
    src/ui/SegmentBar.cpp
    src/ui/SegmentBar.h
    src/ui/SpeedGraph.cpp
    src/ui/SpeedGraph.h
    src/ui/SkinManager.cpp
    src/ui/SkinManager.h
    src/ui/GrabberDialog.cpp
    src/ui/GrabberDialog.h
)

# Browser integration layer
set(BROWSER_SOURCES
    src/browser/NativeMessaging.cpp
    src/browser/NativeMessaging.h
    src/browser/ComServer.cpp
    src/browser/ComServer.h
    src/browser/VideoDetector.cpp
    src/browser/VideoDetector.h
    src/browser/BrowserMonitor.cpp
    src/browser/BrowserMonitor.h
)

# Application entry point and MFC application class
set(APP_SOURCES
    src/IDMApp.cpp
    src/IDMApp.h
    src/stdafx.cpp
    src/stdafx.h
    src/targetver.h
    src/resource.h
)

# Resource files
set(RESOURCE_FILES
    src/resources/IDM.rc
)

# ─── Main Executable ────────────────────────────────────────────────────────
add_executable(IDMClone WIN32
    ${APP_SOURCES}
    ${UTIL_SOURCES}
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${BROWSER_SOURCES}
    ${RESOURCE_FILES}
)

# Include directories
target_include_directories(IDMClone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ─── Link Libraries ─────────────────────────────────────────────────────────
if(WIN32)
    target_link_libraries(IDMClone PRIVATE
        # Windows networking
        ws2_32          # Winsock2 for socket operations
        winhttp         # WinHTTP for high-level HTTP/HTTPS
        wininet         # WinINet for FTP and HTTP operations
        
        # Windows security and crypto
        crypt32         # Certificate operations
        bcrypt          # Modern crypto API (hashing)
        
        # Windows shell and UI
        shlwapi         # Shell lightweight utility APIs
        comctl32        # Common controls (list view, tree view, etc.)
        uxtheme         # Visual styles / theming
        dwmapi          # Desktop Window Manager API (dark mode)
        
        # COM support
        ole32           # COM runtime
        oleaut32        # COM automation
        uuid            # COM GUIDs
        
        # Multimedia (for sound events)
        winmm           # PlaySound API
        
        # Version info
        version         # GetFileVersionInfo
    )
endif()

# ─── Precompiled Header ─────────────────────────────────────────────────────
if(MSVC)
    target_precompile_headers(IDMClone PRIVATE src/stdafx.h)
endif()

# ─── Post-Build: Copy browser extension files ───────────────────────────────
add_custom_command(TARGET IDMClone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/extension
        $<TARGET_FILE_DIR:IDMClone>/extension
    COMMENT "Copying browser extension files..."
)

# ─── Installation rules ─────────────────────────────────────────────────────
install(TARGETS IDMClone DESTINATION bin)
install(DIRECTORY src/extension/ DESTINATION bin/extension)
install(DIRECTORY src/resources/locales/ DESTINATION bin/locales)

# ─── Output Configuration ───────────────────────────────────────────────────
set_target_properties(IDMClone PROPERTIES
    OUTPUT_NAME "IDMClone"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
message(STATUS "IDM Clone v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
