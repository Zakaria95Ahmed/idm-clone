# BUILD.md - IDM Clone Build Instructions

Complete guide to building IDM Clone from source on Windows.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Build Method 1: Visual Studio 2022 (Recommended)](#build-method-1-visual-studio-2022-recommended)
3. [Build Method 2: CMake Command Line](#build-method-2-cmake-command-line)
4. [Build Method 3: MSBuild Command Line](#build-method-3-msbuild-command-line)
5. [Running the Application](#running-the-application)
6. [Project Structure](#project-structure)
7. [Build Configurations](#build-configurations)
8. [Troubleshooting](#troubleshooting)
9. [CI/CD (GitHub Actions)](#cicd-github-actions)

---

## Prerequisites

### Required Software

| Software | Version | Download Link |
|----------|---------|---------------|
| **Visual Studio 2022** | 17.0+ (Community, Professional, or Enterprise) | [https://visualstudio.microsoft.com/vs/](https://visualstudio.microsoft.com/vs/) |
| **Windows SDK** | 10.0.19041.0 or later | Included with Visual Studio installer |
| **CMake** | 3.20+ (optional, for CMake builds) | [https://cmake.org/download/](https://cmake.org/download/) |
| **Git** | 2.30+ (for cloning the repository) | [https://git-scm.com/download/win](https://git-scm.com/download/win) |

### Visual Studio 2022 Workloads

During Visual Studio installation, select the following workloads:

1. **Desktop development with C++** (required)
   - This automatically includes:
   - MSVC v143 build tools (x86/x64)
   - Windows 10/11 SDK
   - C++ core features
   - C++ MFC for latest v143 build tools (x86 & x64)

2. Ensure these **Individual Components** are checked:
   - `C++ MFC for latest v143 build tools (x86 & x64)` **(CRITICAL - MFC is required)**
   - `C++ ATL for latest v143 build tools (x86 & x64)`
   - `Windows 10 SDK (10.0.19041.0)` or later
   - `MSVC v143 - VS 2022 C++ x64/x86 build tools`

> **Important**: MFC is **NOT** installed by default. You must explicitly select it in the Visual Studio Installer under "Individual Components". Search for "MFC" and check the appropriate version.

### No External Dependencies

IDM Clone uses **only Windows SDK libraries** - no external downloads needed:
- **WinHTTP** (HTTP/HTTPS) - included in Windows SDK
- **WinINet** (FTP, cookies) - included in Windows SDK
- **BCrypt** (hashing) - included in Windows SDK
- **MFC** (UI framework) - included with Visual Studio
- **Common Controls 6.0** - included in Windows SDK

---

## Build Method 1: Visual Studio 2022 (Recommended)

### Step-by-Step

1. **Clone the repository:**
   ```cmd
   git clone https://github.com/user/IDMClone.git
   cd IDMClone
   ```

2. **Open the solution:**
   - Double-click `IDMClone.sln` in Windows Explorer, OR
   - In Visual Studio: File > Open > Project/Solution > select `IDMClone.sln`

3. **Select configuration:**
   - In the toolbar, choose **Release** and **x64** (or **Win32** for 32-bit)
   - Available configurations: `Debug|Win32`, `Debug|x64`, `Release|Win32`, `Release|x64`

4. **Build the project:**
   - Press `Ctrl+Shift+B` (Build Solution), OR
   - Menu: Build > Build Solution

5. **Find the output:**
   ```
   bin\x64\Release\IDMClone.exe        (64-bit release)
   bin\x64\Release\extension\           (browser extension files)
   bin\Win32\Release\IDMClone.exe       (32-bit release)
   ```

6. **Run/Debug:**
   - Press `F5` to run with debugger, or `Ctrl+F5` to run without

---

## Build Method 2: CMake Command Line

### Step-by-Step

1. **Open Developer Command Prompt:**
   - Start Menu > "Developer Command Prompt for VS 2022"
   - OR "x64 Native Tools Command Prompt for VS 2022" for 64-bit

2. **Navigate to project root:**
   ```cmd
   cd path\to\IDMClone
   ```

3. **Generate build files:**

   **For x64 (64-bit):**
   ```cmd
   mkdir build-x64
   cd build-x64
   cmake -G "Visual Studio 17 2022" -A x64 ..
   ```

   **For Win32 (32-bit):**
   ```cmd
   mkdir build-x86
   cd build-x86
   cmake -G "Visual Studio 17 2022" -A Win32 ..
   ```

4. **Build:**
   ```cmd
   cmake --build . --config Release --parallel
   ```

   Or for Debug:
   ```cmd
   cmake --build . --config Debug --parallel
   ```

5. **Find the output:**
   ```
   build-x64\bin\Release\IDMClone.exe
   build-x64\bin\Release\extension\
   ```

### CMake with Ninja (faster builds)

```cmd
:: Open "x64 Native Tools Command Prompt for VS 2022"
mkdir build-ninja
cd build-ninja
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

---

## Build Method 3: MSBuild Command Line

For CI/CD or script-based builds:

```cmd
:: Open Developer Command Prompt for VS 2022
cd path\to\IDMClone

:: Build Release x64
msbuild IDMClone.sln /p:Configuration=Release /p:Platform=x64 /m

:: Build Release Win32  
msbuild IDMClone.sln /p:Configuration=Release /p:Platform=Win32 /m

:: Build Debug x64
msbuild IDMClone.sln /p:Configuration=Debug /p:Platform=x64 /m

:: Clean and rebuild
msbuild IDMClone.sln /t:Clean /p:Configuration=Release /p:Platform=x64
msbuild IDMClone.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

---

## Running the Application

### First Run

1. Navigate to the output directory:
   ```cmd
   cd bin\x64\Release
   ```

2. Run the executable:
   ```cmd
   IDMClone.exe
   ```

3. The application will:
   - Create settings in `%APPDATA%\IDMClone\`
   - Create a log file at `%APPDATA%\IDMClone\Logs\idmclone.log`
   - Create a download database at `%APPDATA%\IDMClone\downloads.db`
   - Appear in the system tray

### Command-Line URL Launch

You can pass URLs directly:
```cmd
IDMClone.exe "https://example.com/file.zip"
```

### MFC Runtime Requirements

Since the project uses MFC as a shared DLL, the target machine needs the **Visual C++ Redistributable for Visual Studio 2022**:
- [Download VC++ Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)
- [Download VC++ Redistributable (x86)](https://aka.ms/vs/17/release/vc_redist.x86.exe)

> **Tip**: On the build machine, these are already installed with Visual Studio.

### Browser Extension Installation

1. Open Chrome/Edge and navigate to `chrome://extensions/`
2. Enable "Developer mode"
3. Click "Load unpacked"
4. Select the `extension` folder from the output directory

---

## Project Structure

```
IDMClone/
|-- IDMClone.sln              # Visual Studio 2022 solution
|-- IDMClone.vcxproj           # Visual Studio project
|-- IDMClone.vcxproj.filters   # VS Solution Explorer filters
|-- CMakeLists.txt             # CMake build system
|-- BUILD.md                   # This file
|-- README.md                  # Project overview
|
|-- src/
|   |-- stdafx.h               # Precompiled header (MFC + Windows SDK + STL)
|   |-- stdafx.cpp             # PCH source
|   |-- targetver.h            # Windows version targeting
|   |-- resource.h             # Resource identifiers
|   |-- IDMApp.h / .cpp        # MFC application class (entry point)
|   |
|   |-- core/                  # Download engine (11 components)
|   |   |-- DownloadEngine.*   # Orchestrator (Singleton, Observer pattern)
|   |   |-- SegmentManager.*   # Dynamic segmentation algorithm
|   |   |-- HttpClient.*       # WinHTTP-based HTTP/HTTPS client
|   |   |-- FtpClient.*        # WinINet-based FTP client
|   |   |-- ResumeEngine.*     # Pause/resume and crash recovery
|   |   |-- FileAssembler.*    # Segment merge and file finalization
|   |   |-- ConnectionPool.*   # Client reuse pool
|   |   |-- ProxyManager.*     # HTTP/SOCKS proxy support
|   |   |-- AuthManager.*      # Site credential management
|   |   |-- CookieJar.*        # Cookie management
|   |   |-- SpeedLimiter.*     # Token bucket rate limiting
|   |
|   |-- ui/                    # MFC user interface (12 components)
|   |   |-- MainFrame.*        # Main window with split layout
|   |   |-- DownloadListView.* # Download list with custom progress bars
|   |   |-- CategoryTreeView.* # Category filter tree
|   |   |-- ProgressDialog.*   # Per-file progress window
|   |   |-- AddUrlDialog.*     # New download dialog
|   |   |-- OptionsDialog.*    # 9-tab settings dialog
|   |   |-- SchedulerDialog.*  # Queue management
|   |   |-- BatchDownloadDialog.* # Wildcard URL templates
|   |   |-- SegmentBar.*       # Color-coded segment visualization
|   |   |-- SpeedGraph.*       # Real-time speed chart
|   |   |-- SkinManager.*      # Dark/light theme support
|   |   |-- GrabberDialog.*    # Site grabber (Phase 5 stub)
|   |
|   |-- browser/               # Browser integration
|   |   |-- NativeMessaging.*  # Chrome/Edge native messaging
|   |   |-- ComServer.*        # COM automation interface
|   |   |-- VideoDetector.*    # Video stream detection
|   |   |-- BrowserMonitor.*   # Download interception
|   |
|   |-- util/                  # Utility library
|   |   |-- Logger.*           # Async thread-safe logging
|   |   |-- Database.*         # Download persistence (binary format)
|   |   |-- Registry.*         # Windows Registry wrapper
|   |   |-- Crypto.*           # BCrypt hash verification
|   |   |-- Unicode.*          # URL parsing, encoding, formatting
|   |
|   |-- resources/             # Application resources
|   |   |-- IDM.rc             # Dialog templates, string tables, version
|   |   |-- IDMClone.manifest  # Visual styles, DPI, UAC
|   |   |-- icons/app.ico      # Application icon
|   |   |-- locales/           # Language DLLs (future)
|   |
|   |-- extension/             # Chrome Manifest V3 extension
|       |-- manifest.json
|       |-- background.js
|       |-- content.js
|       |-- video_detector.js
|       |-- popup.html / popup.js
|
|-- include/                   # Public headers (reserved for future SDK)
|-- lib/                       # Third-party libraries (reserved)
|   |-- openssl/               # Reserved for OpenSSL (optional)
|   |-- sqlite3/               # Reserved for SQLite (optional)
|
|-- bin/                       # Build output (created by build)
|-- obj/                       # Intermediate files (created by build)
|-- build/                     # CMake build directory (created by build)
```

---

## Build Configurations

| Configuration | Platform | Use Case |
|---------------|----------|----------|
| **Debug\|Win32** | 32-bit | Development and debugging on x86 |
| **Debug\|x64** | 64-bit | Development and debugging on x64 |
| **Release\|Win32** | 32-bit | Production build for 32-bit Windows |
| **Release\|x64** | 64-bit | Production build for 64-bit Windows (recommended) |

### Debug vs Release

| Feature | Debug | Release |
|---------|-------|---------|
| Optimization | Disabled (/Od) | Maximum Speed (/O2) |
| Runtime checks | Enabled (/RTC1) | Disabled |
| Debug symbols | Full PDB (/Zi) | Embedded PDB |
| Link-time optimization | Disabled | Enabled (/LTCG) |
| MFC libraries | Debug DLLs | Release DLLs |
| Preprocessor | _DEBUG defined | NDEBUG defined |

---

## Troubleshooting

### Error: "Cannot open include file: 'afxwin.h'"

**Cause**: MFC is not installed.

**Fix**: Open Visual Studio Installer > Modify > Individual Components > search "MFC" > check `C++ MFC for latest v143 build tools (x86 & x64)` > click Modify.

### Error: "Cannot open include file: 'winhttp.h'"

**Cause**: Windows SDK is not installed or outdated.

**Fix**: Visual Studio Installer > Modify > Individual Components > check `Windows 10 SDK (10.0.19041.0)` or later.

### Error: "unresolved external symbol" for WinHTTP/BCrypt functions

**Cause**: Link libraries missing.

**Fix**: These should be configured already. If building manually, ensure these libs are linked:
```
ws2_32.lib winhttp.lib wininet.lib crypt32.lib bcrypt.lib
shlwapi.lib comctl32.lib uxtheme.lib dwmapi.lib winmm.lib
version.lib ole32.lib oleaut32.lib uuid.lib
```

### Error: "fatal error C1010: unexpected end of file while looking for precompiled header"

**Cause**: A .cpp file is missing `#include "stdafx.h"` as its first line.

**Fix**: Ensure every .cpp file starts with `#include "stdafx.h"`.

### Error: "LNK2001: unresolved external symbol _WinMain@16"

**Cause**: The subsystem is set to Windows but MFC's entry point isn't configured.

**Fix**: The project uses `wWinMainCRTStartup` as entry point. Ensure the Linker > Advanced > Entry Point is set to `wWinMainCRTStartup` (already configured in .vcxproj).

### Error: "RC1015: cannot open include file 'resource.h'" during resource compilation

**Cause**: Resource compiler can't find include paths.

**Fix**: Ensure Additional Include Directories in Resource Compiler settings includes `$(ProjectDir)src`.

### Warning: "C4996: 'xxx': This function or variable may be unsafe"

**Cause**: MSVC CRT security warnings.

**Fix**: Already suppressed via `_CRT_SECURE_NO_WARNINGS` preprocessor define.

### CMake error: "Could not find MFC"

**Cause**: CMake can't locate MFC installation.

**Fix**:
1. Ensure MFC is installed (see first troubleshooting item)
2. Use the Visual Studio generator: `cmake -G "Visual Studio 17 2022" -A x64 ..`
3. Do not use Ninja generator for MFC projects unless vcvars are set

### Application crashes on startup

**Possible causes and fixes**:
1. **Missing VC++ Redistributable**: Install the Visual C++ Redistributable for VS 2022
2. **Missing MFC DLLs**: Ensure `mfc140u.dll` is accessible (in PATH or alongside .exe)
3. **Check logs**: Look at `%APPDATA%\IDMClone\Logs\idmclone.log` for error details

### Build is very slow

**Fix**: Enable multi-processor compilation (already configured):
- Visual Studio: Project Properties > C/C++ > General > Multi-processor Compilation = Yes
- MSBuild: `/m` flag (already included in examples)
- CMake: `--parallel` flag

---

## CI/CD (GitHub Actions)

The project includes a GitHub Actions workflow at `.github/workflows/build.yml` that:

1. Builds both x64 and Win32 Release configurations
2. Uses `windows-latest` runner with VS 2022 and MFC
3. Produces downloadable artifacts (IDMClone.exe + extension files)

### Manual Trigger

You can trigger a build from the GitHub Actions tab by clicking "Run workflow".

### Artifacts

After a successful build, download artifacts from the Actions run:
- `IDMClone-x64-Release` - 64-bit executable + extension
- `IDMClone-Win32-Release` - 32-bit executable + extension

---

## Development Tips

### Adding a New Source File

1. Create the .h and .cpp files in the appropriate directory
2. Add them to `IDMClone.vcxproj` (in the correct `<ItemGroup>`)
3. Add them to `IDMClone.vcxproj.filters` (for VS folder grouping)
4. Add them to the appropriate `set(...)` in `CMakeLists.txt`
5. Ensure the .cpp file starts with `#include "stdafx.h"`

### Debugging Downloads

Set the log level to Trace in code or the Options dialog for detailed network activity:
```cpp
Logger::Instance().SetLevel(LogLevel::Trace);
```

Log file location: `%APPDATA%\IDMClone\Logs\idmclone.log`

### Testing with a Local Server

Use Python's built-in HTTP server for testing:
```cmd
python -m http.server 8080
```
Then add a download URL: `http://localhost:8080/testfile.bin`
