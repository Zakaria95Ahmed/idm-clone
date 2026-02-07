/**
 * @file resource.h
 * @brief Resource identifiers for the IDM Clone application
 *
 * This header defines all resource IDs used throughout the application.
 * The numbering scheme follows MFC conventions:
 *   - IDR_*   : Resource identifiers (menus, toolbars, accelerators)
 *   - IDD_*   : Dialog template IDs
 *   - IDC_*   : Control IDs within dialogs
 *   - IDI_*   : Icon resource IDs
 *   - IDB_*   : Bitmap resource IDs
 *   - IDS_*   : String table IDs
 *   - ID_*    : Command IDs (menu items, toolbar buttons)
 *   - IDM_*   : Menu command aliases
 *   - WM_APP_*: Application-defined messages
 *
 * Ranges:
 *   1000-1999 : Dialog IDs
 *   2000-2999 : Control IDs
 *   3000-3999 : Command IDs (menus, toolbar)
 *   4000-4999 : String table IDs
 *   5000-5999 : Icons, bitmaps
 *   6000-6999 : Application-defined window messages
 */

#pragma once

// ═══════════════════════════════════════════════════════════════════════════
//  APPLICATION RESOURCES
// ═══════════════════════════════════════════════════════════════════════════

// Main application resources
#define IDR_MAINFRAME               100
#define IDR_MAIN_MENU               101
#define IDR_TOOLBAR                 102
#define IDR_ACCELERATOR             103
#define IDR_CONTEXT_MENU            104
#define IDR_TRAY_MENU               105

// ═══════════════════════════════════════════════════════════════════════════
//  ICONS
// ═══════════════════════════════════════════════════════════════════════════

#define IDI_APP_ICON                5000
#define IDI_TRAY_ICON               5001
#define IDI_TRAY_ICON_ACTIVE        5002
#define IDI_DOWNLOAD_COMPLETE       5003
#define IDI_DOWNLOAD_ERROR          5004
#define IDI_DOWNLOAD_PAUSED         5005
#define IDI_DOWNLOAD_QUEUED         5006
#define IDI_DOWNLOAD_RUNNING        5007
#define IDI_CATEGORY_ALL            5010
#define IDI_CATEGORY_FINISHED       5011
#define IDI_CATEGORY_UNFINISHED     5012
#define IDI_CATEGORY_QUEUED         5013
#define IDI_CATEGORY_GRABBER        5014
#define IDI_CATEGORY_MUSIC          5015
#define IDI_CATEGORY_VIDEO          5016
#define IDI_CATEGORY_PROGRAMS       5017
#define IDI_CATEGORY_DOCUMENTS      5018
#define IDI_CATEGORY_COMPRESSED     5019
#define IDI_CATEGORY_GENERAL        5020

// ═══════════════════════════════════════════════════════════════════════════
//  BITMAPS
// ═══════════════════════════════════════════════════════════════════════════

#define IDB_TOOLBAR_LARGE_3D        5100
#define IDB_TOOLBAR_SMALL_3D        5101
#define IDB_TOOLBAR_LARGE_NEON      5102
#define IDB_TOOLBAR_SMALL_NEON      5103

// ═══════════════════════════════════════════════════════════════════════════
//  DIALOG TEMPLATE IDS
// ═══════════════════════════════════════════════════════════════════════════

#define IDD_ADD_URL                 1000
#define IDD_DOWNLOAD_INFO           1001
#define IDD_PROGRESS                1002
#define IDD_OPTIONS                 1003
#define IDD_SCHEDULER               1004
#define IDD_BATCH_DOWNLOAD          1005
#define IDD_SITE_GRABBER            1006
#define IDD_ABOUT                   1007
#define IDD_SPEED_LIMITER           1008
#define IDD_BATCH_PREVIEW           1009
#define IDD_DOWNLOAD_COMPLETE       1010

// Options dialog tab pages
#define IDD_OPT_GENERAL             1100
#define IDD_OPT_FILE_TYPES          1101
#define IDD_OPT_CONNECTION          1102
#define IDD_OPT_SAVE_TO             1103
#define IDD_OPT_DOWNLOADS           1104
#define IDD_OPT_PROXY               1105
#define IDD_OPT_SITE_LOGINS         1106
#define IDD_OPT_DIALUP              1107
#define IDD_OPT_SOUNDS              1108

// ═══════════════════════════════════════════════════════════════════════════
//  DIALOG CONTROL IDS
// ═══════════════════════════════════════════════════════════════════════════

// --- Add URL Dialog Controls ---
#define IDC_URL_EDIT                2000
#define IDC_USE_AUTH_CHECK          2001
#define IDC_USERNAME_EDIT           2002
#define IDC_PASSWORD_EDIT           2003
#define IDC_REFERER_EDIT            2004
#define IDC_POSTDATA_EDIT           2005
#define IDC_CUSTOM_HEADERS_EDIT     2006
#define IDC_CATEGORY_COMBO          2007
#define IDC_SAVE_TO_EDIT            2008
#define IDC_BROWSE_BTN              2009
#define IDC_FILENAME_EDIT           2010
#define IDC_DESCRIPTION_EDIT        2011
#define IDC_ADD_QUEUE_CHECK         2012
#define IDC_QUEUE_COMBO             2013
#define IDC_DONT_SHOW_CHECK         2014
#define IDC_FILESIZE_STATIC         2015
#define IDC_RESUME_STATIC           2016
#define IDC_START_DOWNLOAD_BTN      2017
#define IDC_DOWNLOAD_LATER_BTN      2018

// --- Progress Dialog Controls ---
#define IDC_PROGRESS_BAR            2100
#define IDC_SEGMENT_BAR             2101
#define IDC_SPEED_GRAPH             2102
#define IDC_FILENAME_LABEL          2103
#define IDC_URL_LABEL               2104
#define IDC_SPEED_LABEL             2105
#define IDC_AVG_SPEED_LABEL         2106
#define IDC_DOWNLOADED_LABEL        2107
#define IDC_TIME_LEFT_LABEL         2108
#define IDC_RESUME_LABEL            2109
#define IDC_CONNECTIONS_LABEL       2110
#define IDC_PAUSE_BTN               2111
#define IDC_CANCEL_BTN              2112
#define IDC_OPEN_FOLDER_BTN         2113
#define IDC_OPEN_FILE_BTN           2114
#define IDC_STATUS_LABEL            2115

// --- Options: General Tab ---
#define IDC_START_WITH_WINDOWS      2200
#define IDC_START_MINIMIZED         2201
#define IDC_BROWSER_CHROME          2202
#define IDC_BROWSER_FIREFOX         2203
#define IDC_BROWSER_EDGE            2204
#define IDC_BROWSER_OPERA           2205
#define IDC_ADVANCED_INTEGRATION    2206
#define IDC_ADD_BROWSER_BTN         2207
#define IDC_CLIPBOARD_MONITOR       2208
#define IDC_KEYS_BTN                2209
#define IDC_DARK_MODE_CHECK         2210

// --- Options: File Types Tab ---
#define IDC_FILE_TYPES_EDIT         2300
#define IDC_EXCEPTION_SITES_LIST    2301
#define IDC_ADD_EXCEPTION_BTN       2302
#define IDC_REMOVE_EXCEPTION_BTN    2303
#define IDC_EDIT_EXCEPTION_BTN      2304

// --- Options: Connection Tab ---
#define IDC_CONNECTION_TYPE_COMBO   2400
#define IDC_MAX_CONNECTIONS_SPIN    2401
#define IDC_MAX_CONNECTIONS_EDIT    2402
#define IDC_SERVER_LIMITS_LIST      2403
#define IDC_ADD_SERVER_BTN          2404
#define IDC_REMOVE_SERVER_BTN       2405
#define IDC_TIMEOUT_EDIT            2406
#define IDC_TIMEOUT_SPIN            2407
#define IDC_RETRIES_EDIT            2408
#define IDC_RETRIES_SPIN            2409
#define IDC_QUOTA_CHECK             2410
#define IDC_QUOTA_EDIT              2411

// --- Options: Save To Tab ---
#define IDC_DEFAULT_DIR_EDIT        2500
#define IDC_DEFAULT_DIR_BROWSE      2501
#define IDC_TEMP_DIR_EDIT           2502
#define IDC_TEMP_DIR_BROWSE         2503
#define IDC_CATEGORY_DIR_LIST       2504
#define IDC_REMEMBER_LAST_DIR       2505

// --- Options: Downloads Tab ---
#define IDC_SHOW_INFO_DIALOG        2600
#define IDC_PROGRESS_SHOW_NORMAL    2601
#define IDC_PROGRESS_SHOW_MIN       2602
#define IDC_PROGRESS_DONT_SHOW      2603
#define IDC_SHOW_COMPLETE_DIALOG    2604
#define IDC_START_IMMEDIATELY       2605
#define IDC_SHOW_QUEUE_PANEL        2606
#define IDC_CUSTOM_USERAGENT_EDIT   2607
#define IDC_VIRUS_CHECK_PATH        2608
#define IDC_VIRUS_CHECK_BROWSE      2609
#define IDC_VIRUS_CHECK_PARAMS      2610

// --- Options: Proxy Tab ---
#define IDC_PROXY_NONE              2700
#define IDC_PROXY_SYSTEM            2701
#define IDC_PROXY_MANUAL            2702
#define IDC_HTTP_PROXY_ADDR         2703
#define IDC_HTTP_PROXY_PORT         2704
#define IDC_HTTPS_PROXY_ADDR        2705
#define IDC_HTTPS_PROXY_PORT        2706
#define IDC_FTP_PROXY_ADDR          2707
#define IDC_FTP_PROXY_PORT          2708
#define IDC_SOCKS_ADDR              2709
#define IDC_SOCKS_PORT              2710
#define IDC_SOCKS_TYPE_COMBO        2711
#define IDC_PROXY_USER              2712
#define IDC_PROXY_PASS              2713
#define IDC_PROXY_EXCEPTIONS_EDIT   2714
#define IDC_FTP_PASSIVE_MODE        2715
#define IDC_GET_SYSTEM_PROXY_BTN    2716

// --- Options: Site Logins Tab ---
#define IDC_LOGINS_LIST             2800
#define IDC_ADD_LOGIN_BTN           2801
#define IDC_EDIT_LOGIN_BTN          2802
#define IDC_DELETE_LOGIN_BTN        2803

// --- Options: Dial-Up Tab ---
#define IDC_DIALUP_CONNECTION       2900
#define IDC_DIALUP_USER             2901
#define IDC_DIALUP_PASS             2902
#define IDC_DIALUP_SAVE_PASS        2903
#define IDC_DIALUP_REDIAL_COUNT     2904
#define IDC_DIALUP_REDIAL_INTERVAL  2905
#define IDC_DIALUP_CONNECT_BEFORE   2906
#define IDC_DIALUP_DISCONNECT_AFTER 2907

// --- Options: Sounds Tab ---
#define IDC_SND_DOWNLOAD_START      2950
#define IDC_SND_DOWNLOAD_COMPLETE   2951
#define IDC_SND_DOWNLOAD_ERROR      2952
#define IDC_SND_QUEUE_START         2953
#define IDC_SND_QUEUE_COMPLETE      2954
#define IDC_SND_BROWSE_BTN          2955
#define IDC_SND_PLAY_BTN            2956
#define IDC_SND_ENABLE_CHECK        2957

// --- Scheduler Dialog ---
#define IDC_QUEUE_LIST              2960
#define IDC_QUEUE_NEW_BTN           2961
#define IDC_QUEUE_DELETE_BTN        2962
#define IDC_QUEUE_MAX_DOWNLOADS     2963
#define IDC_QUEUE_START_TIME        2964
#define IDC_QUEUE_STOP_TIME         2965
#define IDC_QUEUE_DAYS_CHECK        2966  // through 2972 (7 days)
#define IDC_QUEUE_RUN_ONCE          2973
#define IDC_QUEUE_RUN_PERIODIC      2974
#define IDC_QUEUE_ACTION_COMBO      2975
#define IDC_QUEUE_START_BTN         2976
#define IDC_QUEUE_STOP_BTN          2977
#define IDC_QUEUE_FILES_LIST        2978
#define IDC_QUEUE_MOVE_UP           2979
#define IDC_QUEUE_MOVE_DOWN         2980

// --- Batch Download Dialog ---
#define IDC_BATCH_URL_TEMPLATE      2990
#define IDC_BATCH_FROM_EDIT         2991
#define IDC_BATCH_TO_EDIT           2992
#define IDC_BATCH_PREVIEW_LIST      2993
#define IDC_BATCH_CATEGORY_COMBO    2994
#define IDC_BATCH_SAVE_TO_EDIT      2995
#define IDC_BATCH_PREVIEW_BTN       2996

// ═══════════════════════════════════════════════════════════════════════════
//  MENU COMMAND IDS
// ═══════════════════════════════════════════════════════════════════════════

// --- Tasks Menu ---
#define ID_TASKS_ADD_URL            3000
#define ID_TASKS_ADD_BATCH          3001
#define ID_TASKS_EXPORT             3002
#define ID_TASKS_IMPORT             3003
#define ID_TASKS_SITE_GRABBER       3004
#define ID_TASKS_EXIT               3005

// --- Downloads Menu ---
#define ID_DL_RESUME                3100
#define ID_DL_RESUME_ALL            3101
#define ID_DL_STOP                  3102
#define ID_DL_STOP_ALL              3103
#define ID_DL_DELETE                3104
#define ID_DL_DELETE_COMPLETED      3105
#define ID_DL_DELETE_WITH_FILE      3106
#define ID_DL_REDOWNLOAD            3107
#define ID_DL_OPEN_FILE             3108
#define ID_DL_OPEN_FOLDER           3109
#define ID_DL_PROPERTIES            3110
#define ID_DL_SELECT_ALL            3111
#define ID_DL_MOVE_TO_QUEUE         3112

// --- File Menu ---
#define ID_FILE_EXIT                3200

// --- Queues Menu ---
#define ID_QUEUE_SCHEDULER          3300
#define ID_QUEUE_START              3301
#define ID_QUEUE_STOP               3302
#define ID_QUEUE_ADD                3303
#define ID_QUEUE_REMOVE             3304

// --- Options Menu ---
#define ID_OPTIONS_SETTINGS         3400
#define ID_OPTIONS_SPEED_LIMITER    3401

// --- View Menu ---
#define ID_VIEW_CATEGORIES          3500
// ID_VIEW_TOOLBAR is already defined in afxres.h (MFC); guard to prevent C4005
#ifndef ID_VIEW_TOOLBAR
#define ID_VIEW_TOOLBAR             3501
#endif
#define ID_VIEW_COLUMNS             3502
#define ID_VIEW_TOOLBAR_LARGE3D     3503
#define ID_VIEW_TOOLBAR_SMALL3D     3504
#define ID_VIEW_TOOLBAR_LARGE_NEON  3505
#define ID_VIEW_TOOLBAR_SMALL_NEON  3506
#define ID_VIEW_DARK_MODE           3507

// --- Help Menu ---
#define ID_HELP_ABOUT               3600
#define ID_HELP_CHECK_UPDATES       3601
#define ID_HELP_REGISTRATION        3602

// --- Tray Menu ---
#define ID_TRAY_OPEN                3700
#define ID_TRAY_ADD_URL             3701
#define ID_TRAY_STOP_ALL            3702
#define ID_TRAY_START_QUEUE         3703
#define ID_TRAY_STOP_QUEUE          3704
#define ID_TRAY_SCHEDULER           3705
#define ID_TRAY_EXIT                3706

// ═══════════════════════════════════════════════════════════════════════════
//  APPLICATION-DEFINED WINDOW MESSAGES
// ═══════════════════════════════════════════════════════════════════════════

// WM_APP base is 0x8000
#define WM_APP_TRAY_NOTIFY          (WM_APP + 1)   // System tray notifications
#define WM_APP_DOWNLOAD_ADDED       (WM_APP + 2)   // New download added
#define WM_APP_DOWNLOAD_PROGRESS    (WM_APP + 3)   // Download progress update
#define WM_APP_DOWNLOAD_COMPLETE    (WM_APP + 4)   // Download finished
#define WM_APP_DOWNLOAD_ERROR       (WM_APP + 5)   // Download error occurred
#define WM_APP_DOWNLOAD_PAUSED      (WM_APP + 6)   // Download paused
#define WM_APP_DOWNLOAD_RESUMED     (WM_APP + 7)   // Download resumed
#define WM_APP_SPEED_UPDATE         (WM_APP + 8)   // Speed limiter state change
#define WM_APP_QUEUE_STATE_CHANGED  (WM_APP + 9)   // Queue start/stop
#define WM_APP_CLIPBOARD_URL        (WM_APP + 10)  // URL detected on clipboard
#define WM_APP_EXTERNAL_ADD_URL     (WM_APP + 11)  // URL from browser extension
#define WM_APP_THEME_CHANGED        (WM_APP + 12)  // Dark/Light mode toggled
#define WM_APP_REFRESH_LIST         (WM_APP + 13)  // Refresh download list view
#define WM_APP_SEGMENT_UPDATE       (WM_APP + 14)  // Segment state changed

// ═══════════════════════════════════════════════════════════════════════════
//  STRING TABLE IDS
// ═══════════════════════════════════════════════════════════════════════════

#define IDS_APP_TITLE               4000
#define IDS_READY                   4001
#define IDS_DOWNLOADING             4002
#define IDS_PAUSED                  4003
#define IDS_COMPLETE                4004
#define IDS_ERROR                   4005
#define IDS_QUEUED                  4006
#define IDS_WAITING                 4007
#define IDS_CONNECTING              4008
#define IDS_FILE_NOT_FOUND          4009
#define IDS_ACCESS_DENIED           4010
#define IDS_RESUME_NOT_SUPPORTED    4011
#define IDS_UNKNOWN_SIZE            4012
#define IDS_SPEED_FORMAT            4013
#define IDS_TIME_FORMAT             4014
#define IDS_CATEGORY_ALL            4015
#define IDS_CATEGORY_FINISHED       4016
#define IDS_CATEGORY_UNFINISHED     4017
#define IDS_CATEGORY_QUEUED         4018

// Next default resource ID values
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE    200
#define _APS_NEXT_COMMAND_VALUE     3800
#define _APS_NEXT_CONTROL_VALUE     3000
#define _APS_NEXT_SYMED_VALUE       200
#endif
#endif
