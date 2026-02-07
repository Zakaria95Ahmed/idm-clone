/**
 * @file Unicode.h
 * @brief Unicode string utility functions
 *
 * Provides conversion between UTF-8, UTF-16 (Windows native), and ANSI,
 * plus URL encoding/decoding, filename sanitization, and human-readable
 * size formatting.
 */

#pragma once
#include "stdafx.h"

namespace idm {

class Unicode {
public:
    // ─── Encoding Conversion ───────────────────────────────────────────────
    static std::string WideToUtf8(const String& wide);
    static String Utf8ToWide(const std::string& utf8);
    static std::string WideToAnsi(const String& wide);
    static String AnsiToWide(const std::string& ansi);
    
    // ─── URL Operations ────────────────────────────────────────────────────
    static String UrlEncode(const String& input);
    static String UrlDecode(const String& input);
    static String ExtractFilenameFromUrl(const String& url);
    static String ExtractHostFromUrl(const String& url);
    static uint16 ExtractPortFromUrl(const String& url);
    static String ExtractPathFromUrl(const String& url);
    static bool IsHttpUrl(const String& url);
    static bool IsHttpsUrl(const String& url);
    static bool IsFtpUrl(const String& url);
    
    // ─── File Operations ───────────────────────────────────────────────────
    static String SanitizeFilename(const String& filename);
    static String GetFileExtension(const String& filename);
    static String CategorizeByExtension(const String& extension);
    
    // ─── Human-Readable Formatting ─────────────────────────────────────────
    static String FormatFileSize(int64 bytes);
    static String FormatSpeed(double bytesPerSec);
    static String FormatTimeRemaining(double seconds);
    static String FormatDateTime(const SystemTimePoint& time);
    
    // ─── String Operations ─────────────────────────────────────────────────
    static String ToLower(const String& str);
    static String ToUpper(const String& str);
    static String Trim(const String& str);
    static std::vector<String> Split(const String& str, wchar_t delimiter);
    static bool WildcardMatch(const String& text, const String& pattern);
};

} // namespace idm
