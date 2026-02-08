/**
 * @file FileAssembler.cpp
 * @brief File assembly implementation with random-access I/O
 */

#include "stdafx.h"
#include "FileAssembler.h"
#include "../util/Logger.h"
#include "../util/Unicode.h"

namespace idm {

HANDLE FileAssembler::OpenPartialFile(const String& partialPath, int64 fileSize) {
    // Ensure directory exists
    std::filesystem::path dir = std::filesystem::path(partialPath).parent_path();
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }
    
    // Open/create with random access pattern
    // FILE_FLAG_RANDOM_ACCESS hints to the OS cache manager that we'll
    // be seeking around (not reading sequentially), optimizing prefetch
    HANDLE hFile = ::CreateFileW(
        partialPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,  // Allow other threads to read for hash verification
        nullptr,
        OPEN_ALWAYS,      // Open existing or create new
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"FileAssembler: failed to open %s (error %lu)",
                  partialPath.c_str(), ::GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    
    // Pre-allocate if we know the file size and file is newly created
    if (fileSize > 0) {
        LARGE_INTEGER currentSize;
        if (::GetFileSizeEx(hFile, &currentSize) && currentSize.QuadPart == 0) {
            PreallocateFile(hFile, fileSize);
        }
    }
    
    return hFile;
}

bool FileAssembler::WriteAtPosition(HANDLE hFile, int64 position,
                                     const uint8* data, size_t length) {
    if (hFile == INVALID_HANDLE_VALUE || !data || length == 0) return false;
    
    // Use overlapped structure for positioned I/O
    // This avoids needing a mutex around SetFilePointer + WriteFile
    OVERLAPPED ov = {};
    ov.Offset = static_cast<DWORD>(position & 0xFFFFFFFF);
    ov.OffsetHigh = static_cast<DWORD>(position >> 32);
    
    DWORD bytesWritten = 0;
    size_t totalWritten = 0;
    
    while (totalWritten < length) {
        DWORD toWrite = static_cast<DWORD>(
            (std::min<size_t>)(length - totalWritten, 1024 * 1024));  // 1MB max per call
        
        ov.Offset = static_cast<DWORD>((position + totalWritten) & 0xFFFFFFFF);
        ov.OffsetHigh = static_cast<DWORD>((position + totalWritten) >> 32);
        
        if (!::WriteFile(hFile, data + totalWritten, toWrite, &bytesWritten, &ov)) {
            LOG_ERROR(L"FileAssembler: write failed at position %lld (error %lu)",
                      position + static_cast<int64>(totalWritten), ::GetLastError());
            return false;
        }
        
        totalWritten += bytesWritten;
    }
    
    return true;
}

String FileAssembler::Finalize(const String& partialPath, const String& targetPath,
                                ConflictResolution conflictMode) {
    String finalPath = targetPath;
    
    // Handle naming conflicts
    if (std::filesystem::exists(targetPath)) {
        switch (conflictMode) {
            case ConflictResolution::AutoRename:
                finalPath = GenerateUniqueName(targetPath);
                break;
            case ConflictResolution::Overwrite:
                ::DeleteFileW(targetPath.c_str());
                break;
            case ConflictResolution::Skip:
                LOG_INFO(L"FileAssembler: skipping - file already exists: %s",
                         targetPath.c_str());
                // Delete the partial file
                ::DeleteFileW(partialPath.c_str());
                return targetPath;
        }
    }
    
    // Rename partial file to final name
    if (!::MoveFileExW(partialPath.c_str(), finalPath.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        // If move fails (different volume), try copy + delete
        if (::CopyFileW(partialPath.c_str(), finalPath.c_str(), FALSE)) {
            ::DeleteFileW(partialPath.c_str());
        } else {
            LOG_ERROR(L"FileAssembler: failed to finalize %s -> %s (error %lu)",
                      partialPath.c_str(), finalPath.c_str(), ::GetLastError());
            return L"";
        }
    }
    
    LOG_INFO(L"FileAssembler: finalized %s", finalPath.c_str());
    return finalPath;
}

String FileAssembler::GenerateUniqueName(const String& targetPath) {
    std::filesystem::path p(targetPath);
    String stem = p.stem().wstring();
    String ext = p.extension().wstring();
    String dir = p.parent_path().wstring();
    
    for (int i = 1; i < 10000; ++i) {
        wchar_t buf[16];
        _snwprintf_s(buf, _TRUNCATE, L"(%d)", i);
        String candidate = dir + L"\\" + stem + buf + ext;
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    
    // Fallback: use timestamp
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    wchar_t buf[32];
    _snwprintf_s(buf, _TRUNCATE, L"_%lld", static_cast<long long>(now));
    return dir + L"\\" + stem + buf + ext;
}

bool FileAssembler::SetFileTimestamp(const String& filePath, const String& lastModified) {
    if (lastModified.empty()) return false;
    
    // Parse HTTP date format: "Thu, 01 Dec 2020 12:00:00 GMT"
    SYSTEMTIME st = {};
    if (!::InternetTimeToSystemTimeW(lastModified.c_str(), &st, 0)) {
        return false;
    }
    
    FILETIME ft;
    if (!::SystemTimeToFileTime(&st, &ft)) {
        return false;
    }
    
    HANDLE hFile = ::CreateFileW(filePath.c_str(), FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) return false;
    
    BOOL result = ::SetFileTime(hFile, nullptr, nullptr, &ft);
    ::CloseHandle(hFile);
    
    return result != FALSE;
}

bool FileAssembler::PreallocateFile(HANDLE hFile, int64 fileSize) {
    if (fileSize <= 0) return false;
    
    LARGE_INTEGER li;
    li.QuadPart = fileSize;
    
    if (!::SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN)) {
        return false;
    }
    
    if (!::SetEndOfFile(hFile)) {
        return false;
    }
    
    // Reset pointer to beginning
    li.QuadPart = 0;
    ::SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN);
    
    LOG_DEBUG(L"FileAssembler: pre-allocated %lld bytes", fileSize);
    return true;
}

} // namespace idm
