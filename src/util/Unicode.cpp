/**
 * @file Unicode.cpp
 * @brief Unicode utility implementations
 */

#include "stdafx.h"
#include "Unicode.h"

namespace idm {

// ─── Encoding Conversions ──────────────────────────────────────────────────
std::string Unicode::WideToUtf8(const String& wide) {
    if (wide.empty()) return {};
    int size = ::WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), 
        static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), 
        static_cast<int>(wide.length()), &result[0], size, nullptr, nullptr);
    return result;
}

String Unicode::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), 
        static_cast<int>(utf8.length()), nullptr, 0);
    String result(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), 
        static_cast<int>(utf8.length()), &result[0], size);
    return result;
}

std::string Unicode::WideToAnsi(const String& wide) {
    if (wide.empty()) return {};
    int size = ::WideCharToMultiByte(CP_ACP, 0, wide.c_str(), 
        static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, wide.c_str(), 
        static_cast<int>(wide.length()), &result[0], size, nullptr, nullptr);
    return result;
}

String Unicode::AnsiToWide(const std::string& ansi) {
    if (ansi.empty()) return {};
    int size = ::MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), 
        static_cast<int>(ansi.length()), nullptr, 0);
    String result(size, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), 
        static_cast<int>(ansi.length()), &result[0], size);
    return result;
}

// ─── URL Operations ────────────────────────────────────────────────────────
String Unicode::UrlEncode(const String& input) {
    std::string utf8 = WideToUtf8(input);
    String result;
    result.reserve(utf8.size() * 3);
    
    for (unsigned char c : utf8) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += static_cast<wchar_t>(c);
        } else {
            wchar_t buf[8];
            _snwprintf_s(buf, _TRUNCATE, L"%%%02X", c);
            result += buf;
        }
    }
    return result;
}

String Unicode::UrlDecode(const String& input) {
    std::string utf8 = WideToUtf8(input);
    std::string decoded;
    decoded.reserve(utf8.size());
    
    for (size_t i = 0; i < utf8.size(); ++i) {
        if (utf8[i] == '%' && i + 2 < utf8.size()) {
            char hex[3] = { utf8[i+1], utf8[i+2], '\0' };
            decoded += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else if (utf8[i] == '+') {
            decoded += ' ';
        } else {
            decoded += utf8[i];
        }
    }
    return Utf8ToWide(decoded);
}

String Unicode::ExtractFilenameFromUrl(const String& url) {
    // Remove query string and fragment
    String cleanUrl = url;
    auto queryPos = cleanUrl.find(L'?');
    if (queryPos != String::npos) cleanUrl = cleanUrl.substr(0, queryPos);
    auto fragPos = cleanUrl.find(L'#');
    if (fragPos != String::npos) cleanUrl = cleanUrl.substr(0, fragPos);
    
    // Find last path component
    auto lastSlash = cleanUrl.rfind(L'/');
    String filename;
    if (lastSlash != String::npos && lastSlash + 1 < cleanUrl.length()) {
        filename = cleanUrl.substr(lastSlash + 1);
    } else {
        filename = L"download";
    }
    
    // URL decode the filename
    filename = UrlDecode(filename);
    
    // Sanitize
    return SanitizeFilename(filename);
}

String Unicode::ExtractHostFromUrl(const String& url) {
    // Skip protocol
    size_t start = 0;
    auto protoEnd = url.find(L"://");
    if (protoEnd != String::npos) start = protoEnd + 3;
    
    // Skip authentication
    auto atPos = url.find(L'@', start);
    auto slashPos = url.find(L'/', start);
    if (atPos != String::npos && (slashPos == String::npos || atPos < slashPos)) {
        start = atPos + 1;
    }
    
    // Find end of host (port or path)
    auto end = url.find_first_of(L":/?#", start);
    return url.substr(start, end == String::npos ? String::npos : end - start);
}

uint16 Unicode::ExtractPortFromUrl(const String& url) {
    size_t start = 0;
    auto protoEnd = url.find(L"://");
    if (protoEnd != String::npos) start = protoEnd + 3;
    
    // Find port separator after host
    auto hostEnd = url.find_first_of(L"/?#", start);
    String hostPart = url.substr(start, hostEnd == String::npos ? String::npos : hostEnd - start);
    
    auto colonPos = hostPart.rfind(L':');
    if (colonPos != String::npos) {
        try {
            return static_cast<uint16>(std::stoi(hostPart.substr(colonPos + 1)));
        } catch (...) {}
    }
    
    // Default ports
    if (url.find(L"https://") == 0) return 443;
    if (url.find(L"http://") == 0) return 80;
    if (url.find(L"ftp://") == 0) return 21;
    return 80;
}

String Unicode::ExtractPathFromUrl(const String& url) {
    size_t start = 0;
    auto protoEnd = url.find(L"://");
    if (protoEnd != String::npos) start = protoEnd + 3;
    
    auto pathStart = url.find(L'/', start);
    if (pathStart == String::npos) return L"/";
    
    auto queryPos = url.find(L'?', pathStart);
    if (queryPos != String::npos) return url.substr(pathStart, queryPos - pathStart);
    
    auto fragPos = url.find(L'#', pathStart);
    if (fragPos != String::npos) return url.substr(pathStart, fragPos - pathStart);
    
    return url.substr(pathStart);
}

bool Unicode::IsHttpUrl(const String& url) {
    return url.find(L"http://") == 0;
}

bool Unicode::IsHttpsUrl(const String& url) {
    return url.find(L"https://") == 0;
}

bool Unicode::IsFtpUrl(const String& url) {
    return url.find(L"ftp://") == 0;
}

// ─── File Operations ───────────────────────────────────────────────────────
String Unicode::SanitizeFilename(const String& filename) {
    // Remove characters not allowed in Windows filenames
    static const String invalidChars = L"\\/:*?\"<>|";
    String result;
    result.reserve(filename.length());
    
    for (wchar_t c : filename) {
        if (invalidChars.find(c) == String::npos && c >= 32) {
            result += c;
        } else {
            result += L'_';
        }
    }
    
    // Trim whitespace and dots from end
    while (!result.empty() && (result.back() == L' ' || result.back() == L'.')) {
        result.pop_back();
    }
    
    // Ensure non-empty
    if (result.empty()) result = L"download";
    
    // Truncate if too long (Windows MAX_PATH consideration)
    if (result.length() > 200) {
        // Keep extension
        auto dotPos = result.rfind(L'.');
        if (dotPos != String::npos && dotPos > 180) {
            String ext = result.substr(dotPos);
            result = result.substr(0, 200 - ext.length()) + ext;
        } else {
            result = result.substr(0, 200);
        }
    }
    
    return result;
}

String Unicode::GetFileExtension(const String& filename) {
    auto dotPos = filename.rfind(L'.');
    if (dotPos == String::npos || dotPos == filename.length() - 1) return L"";
    return ToLower(filename.substr(dotPos));
}

String Unicode::CategorizeByExtension(const String& extension) {
    String ext = ToLower(extension);
    
    // Music
    if (ext == L".mp3" || ext == L".wav" || ext == L".flac" || ext == L".aac" ||
        ext == L".ogg" || ext == L".wma" || ext == L".m4a" || ext == L".opus" ||
        ext == L".mid")
        return L"Music";
    
    // Video
    if (ext == L".mp4" || ext == L".avi" || ext == L".mkv" || ext == L".mov" ||
        ext == L".wmv" || ext == L".flv" || ext == L".webm" || ext == L".mpg" ||
        ext == L".mpeg" || ext == L".3gp" || ext == L".m4v")
        return L"Video";
    
    // Programs
    if (ext == L".exe" || ext == L".msi" || ext == L".apk" || ext == L".deb" ||
        ext == L".rpm" || ext == L".bin" || ext == L".run" || ext == L".sh" ||
        ext == L".bat" || ext == L".dmg" || ext == L".iso")
        return L"Programs";
    
    // Documents
    if (ext == L".doc" || ext == L".docx" || ext == L".pdf" || ext == L".xls" ||
        ext == L".xlsx" || ext == L".ppt" || ext == L".pptx" || ext == L".txt" ||
        ext == L".rtf" || ext == L".odt" || ext == L".csv")
        return L"Documents";
    
    // Compressed
    if (ext == L".zip" || ext == L".rar" || ext == L".7z" || ext == L".tar" ||
        ext == L".gz" || ext == L".bz2" || ext == L".xz" || ext == L".cab")
        return L"Compressed";
    
    return L"General";
}

// ─── Human-Readable Formatting ─────────────────────────────────────────────
String Unicode::FormatFileSize(int64 bytes) {
    if (bytes < 0) return L"Unknown";
    
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 5) {
        size /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buf[64];
    if (unitIndex == 0) {
        _snwprintf_s(buf, _TRUNCATE, L"%lld B", bytes);
    } else if (size >= 100.0) {
        _snwprintf_s(buf, _TRUNCATE, L"%.0f %s", size, units[unitIndex]);
    } else if (size >= 10.0) {
        _snwprintf_s(buf, _TRUNCATE, L"%.1f %s", size, units[unitIndex]);
    } else {
        _snwprintf_s(buf, _TRUNCATE, L"%.2f %s", size, units[unitIndex]);
    }
    return buf;
}

String Unicode::FormatSpeed(double bytesPerSec) {
    if (bytesPerSec <= 0.0) return L"0 B/s";
    
    const wchar_t* units[] = { L"B/s", L"KB/s", L"MB/s", L"GB/s" };
    int unitIndex = 0;
    double speed = bytesPerSec;
    
    while (speed >= 1024.0 && unitIndex < 3) {
        speed /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buf[64];
    if (speed >= 100.0) {
        _snwprintf_s(buf, _TRUNCATE, L"%.0f %s", speed, units[unitIndex]);
    } else if (speed >= 10.0) {
        _snwprintf_s(buf, _TRUNCATE, L"%.1f %s", speed, units[unitIndex]);
    } else {
        _snwprintf_s(buf, _TRUNCATE, L"%.2f %s", speed, units[unitIndex]);
    }
    return buf;
}

String Unicode::FormatTimeRemaining(double seconds) {
    if (seconds <= 0.0 || !std::isfinite(seconds)) return L"Unknown";
    
    int totalSec = static_cast<int>(seconds);
    int days  = totalSec / 86400;
    int hours = (totalSec % 86400) / 3600;
    int mins  = (totalSec % 3600) / 60;
    int secs  = totalSec % 60;
    
    wchar_t buf[64];
    if (days > 0) {
        _snwprintf_s(buf, _TRUNCATE, L"%dd %dh %dm", days, hours, mins);
    } else if (hours > 0) {
        _snwprintf_s(buf, _TRUNCATE, L"%dh %dm %ds", hours, mins, secs);
    } else if (mins > 0) {
        _snwprintf_s(buf, _TRUNCATE, L"%dm %ds", mins, secs);
    } else {
        _snwprintf_s(buf, _TRUNCATE, L"%ds", secs);
    }
    return buf;
}

String Unicode::FormatDateTime(const SystemTimePoint& time) {
    auto timeT = SystemClock::to_time_t(time);
    struct tm timeInfo;
    localtime_s(&timeInfo, &timeT);
    
    wchar_t buf[64];
    wcsftime(buf, 64, L"%Y-%m-%d %H:%M:%S", &timeInfo);
    return buf;
}

// ─── String Operations ─────────────────────────────────────────────────────
String Unicode::ToLower(const String& str) {
    String result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

String Unicode::ToUpper(const String& str) {
    String result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towupper);
    return result;
}

String Unicode::Trim(const String& str) {
    auto start = str.find_first_not_of(L" \t\r\n");
    if (start == String::npos) return L"";
    auto end = str.find_last_not_of(L" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<String> Unicode::Split(const String& str, wchar_t delimiter) {
    std::vector<String> tokens;
    std::wistringstream stream(str);
    String token;
    while (std::getline(stream, token, delimiter)) {
        String trimmed = Trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    return tokens;
}

bool Unicode::WildcardMatch(const String& text, const String& pattern) {
    // Simple wildcard matching (* and ?)
    size_t ti = 0, pi = 0;
    size_t starTi = String::npos, starPi = String::npos;
    
    while (ti < text.length()) {
        if (pi < pattern.length() && (pattern[pi] == L'?' || 
            towlower(pattern[pi]) == towlower(text[ti]))) {
            ++ti; ++pi;
        } else if (pi < pattern.length() && pattern[pi] == L'*') {
            starPi = pi++;
            starTi = ti;
        } else if (starPi != String::npos) {
            pi = starPi + 1;
            ti = ++starTi;
        } else {
            return false;
        }
    }
    
    while (pi < pattern.length() && pattern[pi] == L'*') ++pi;
    return pi == pattern.length();
}

} // namespace idm
