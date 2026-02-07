/**
 * @file targetver.h
 * @brief Target Windows version definitions
 *
 * Defines the minimum Windows version that this application supports.
 * We target Windows 7 (0x0601) as the minimum, matching IDM's actual
 * system requirements. This header must be included before any Windows
 * SDK headers to ensure correct API availability.
 *
 * Architecture note: IDM supports both IA-32 and x64 architectures.
 * All code uses size_t and INT_PTR appropriately for pointer-width
 * independence.
 */

#pragma once

// Windows 7 as minimum supported platform
#ifndef WINVER
#define WINVER 0x0601
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Internet Explorer 10.0 common controls
#ifndef _WIN32_IE
#define _WIN32_IE 0x0A00
#endif

// Windows Sockets 2.2
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0601
#endif
