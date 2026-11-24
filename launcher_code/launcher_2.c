#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>

#pragma comment(lib, "wininet.lib")

#ifndef APP_VERSION_STR
#define APP_VERSION_STR L"1.2"
#endif

// ============================================================
// GitHub update source
// ============================================================
#define GH_RAW_BASE       L"https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
#define GH_VERSION        GH_RAW_BASE L"version.txt"

// ============================================================
// Minimum supported Windows version (Windows 10 1809 - build 17763)
// ============================================================
#define WIN10_MIN_BUILD 17763

// ============================================================
// Windows 8 Flat / Metro Palette
// ============================================================
#define WIN8_CLR_BG           RGB(255, 255, 255)
#define WIN8_CLR_PANEL        RGB(245, 246, 248)
#define WIN8_CLR_BORDER       RGB(196, 196, 196)
#define WIN8_CLR_BTN_FACE     RGB(221, 221, 221)
#define WIN8_CLR_BTN_HOT      RGB(210, 227, 245)
#define WIN8_CLR_BTN_HOT_BDR  RGB(0, 120, 215)
#define WIN8_CLR_BTN_PRESS    RGB(0, 120, 215)
#define WIN8_CLR_BTN_DIS      RGB(240, 240, 240)
#define WIN8_CLR_TEXT         RGB(34, 34, 34)
#define WIN8_CLR_TEXT_MUTED   RGB(140, 140, 140)
#define WIN8_CLR_ACCENT       RGB(0, 120, 215)
#define WIN8_CLR_ACCENT_LIGHT RGB(230, 242, 252)
#define WIN8_CLR_SUCCESS      RGB(16, 124, 16)

// ============================================================
// IDs
// ============================================================
#define ID_BTN_INSTALL    1001
#define ID_BTN_EXIT       1003
#define ID_EDIT_OUTPUT    1004
#define ID_STATUSBAR      1005
#define ID_LISTVIEW       1006
#define ID_BTN_SELALL     1007
#define ID_BTN_SELNONE    1008
#define ID_PROGRESS_TOTAL 1009
#define ID_LABEL_TOTAL    1010
#define ID_BTN_SEARCH     1011
#define ID_BTN_SETTINGS   1013

#define ID_SEARCH_EDIT     2001
#define ID_SEARCH_BTN      2002
#define ID_SEARCH_LIST     2003
#define ID_SEARCH_ADD      2004
#define ID_SEARCH_CLOSE    2005
#define ID_SEARCH_PROGRESS 2006

#define ID_SET_COMBO_LANG  3001
#define ID_SET_BTN_REPO    3002
#define ID_SET_BTN_LOCAL_EXE 3009
#define ID_SET_BTN_LOGS    3003
#define ID_SET_BTN_DBGLOG  3004
#define ID_SET_BTN_UPDATE  3005
#define ID_SET_BTN_ABOUT   3006
#define ID_SET_BTN_CLOSE   3007
#define ID_SET_LBL_LANG    3008

#define WM_APP_SEARCH_DONE (WM_APP + 1)
#define WM_APP_UPDATE_DONE (WM_APP + 2)

// ============================================================
// Localization
// ============================================================
typedef struct {
    const wchar_t* btnInstall;
    const wchar_t* btnSelAll;
    const wchar_t* btnSelNone;
    const wchar_t* btnSearch;
    const wchar_t* btnOpenRepo;
    const wchar_t* btnLocalExe;
    const wchar_t* btnSettings;
    const wchar_t* btnLogs;
    const wchar_t* btnFullLog;
    const wchar_t* btnExit;
    const wchar_t* lblTotalFmt;
    const wchar_t* statusReady;
    const wchar_t* statusLoadedFmt;
    const wchar_t* statusRepoNotFound;
    const wchar_t* statusInstalling;
    const wchar_t* statusDoneOk;
    const wchar_t* statusDoneErr;
    const wchar_t* statusUpdErr;
    const wchar_t* statusUpdLatest;
    const wchar_t* statusUpdCancel;
    const wchar_t* statusUpdLaunchErr;
    const wchar_t* setTitle;
    const wchar_t* setLangLabel;
    const wchar_t* setBtnUpdate;
    const wchar_t* setBtnAbout;
    const wchar_t* setBtnClose;
    const wchar_t* aboutTitle;
    const wchar_t* aboutTextFmt;
    const wchar_t* searchWndTitle;
    const wchar_t* searchBtnFind;
    const wchar_t* searchColProg;
    const wchar_t* searchBtnAdd;
    const wchar_t* searchBtnClose;
    const wchar_t* msgBusyTitle;
    const wchar_t* msgBusyText;
    const wchar_t* msgWarnTitle;
    const wchar_t* msgNoApps;
    const wchar_t* msgLogsTitle;
    const wchar_t* msgNoLogs;
    const wchar_t* msgNoDebugLog;
    const wchar_t* msgErrTitle;
    const wchar_t* msgNoRepoFile;
    const wchar_t* msgConfirmTitle;
    const wchar_t* msgAbortExit;
    const wchar_t* msgUnsupportedTitle;
    const wchar_t* msgUnsupportedFmt;
    const wchar_t* msgNoWingetTitle;
    const wchar_t* msgNoWingetText;
    const wchar_t* msgUpdAvailTitle;
    const wchar_t* msgUpdAvailFmt;
    const wchar_t* msgSelectProg;
    const wchar_t* msgInfoTitle;
    const wchar_t* msgAlreadyInRepo;
    const wchar_t* msgSuccessTitle;
    const wchar_t* msgAddedFmt;
} LangPack;

static const LangPack g_Lang[2] = {
    // 0: RU
    {
        L"Установить выбранное",
        L"Выбрать все",
        L"Снять все",
        L"Поиск winget",
        L"Открыть репозиторий",
        L"Локальные установщики (source/exe)",
        L"Настройки",
        L"Открыть логи",
        L"Полный лог (Debug)",
        L"Выход",
        L"Установка: %d / %d",
        L"Готов",
        L"Загружено %d строк (пропущено %d)",
        L"Файл репозитория не найден",
        L"Установка...",
        L"Successfully  ·  Установка завершена",
        L"Ошибка  ·  Смотрите лог",
        L"Не удалось проверить обновления",
        L"У вас последняя версия",
        L"Обновление отменено",
        L"Ошибка запуска обновления",
        L"Настройки",
        L"Язык интерфейса:",
        L"Проверить обновления",
        L"О программе",
        L"Закрыть",
        L"О программе",
        L"WinGET Launcher\nУстановленная версия: %ls\n\nГрафическая оболочка для пакетного менеджера Windows (winget).",
        L"Поиск приложений в winget",
        L"Найти",
        L"Программа",
        L"Добавить в репозиторий",
        L"Закрыть",
        L"Занято",
        L"Установка уже выполняется.",
        L"Внимание",
        L"Не отмечено ни одного приложения.",
        L"Логи",
        L"Папка logs ещё не создана.",
        L"Файл полного лога debug.log ещё не создан.",
        L"Ошибка",
        L"Файл repositories.txt не найден.",
        L"Подтверждение",
        L"Установка ещё идёт. Прервать и выйти?",
        L"Неподдерживаемая версия Windows",
        L"Минимальная поддерживаемая версия Windows — Windows 10 1809 (сборка %d).\n\nВаша версия: %lu.%lu (сборка %lu)\n\nЛаунчер будет закрыт.",
        L"winget не найден",
        L"winget не найден.\n\nЗапустить установку winget из source\\install_winget.bat?",
        L"Обновление",
        L"Доступна новая версия!\n\nТекущая: %ls\nНовая:   %ls\n\nЛаунчер будет закрыт, а обновление продолжит\nsetup.exe с ключом -update.\n\nПродолжить?",
        L"Выберите программу в списке.",
        L"Информация",
        L"Этот пакет уже есть в repositories.txt",
        L"Успех",
        L"Добавлено:\n%ls|%ls"
    },
    // 1: EN
    {
        L"Install selected",
        L"Select all",
        L"Deselect all",
        L"Search winget",
        L"Open repository",
        L"Local installers (source/exe)",
        L"Settings",
        L"Open logs",
        L"Full log (Debug)",
        L"Exit",
        L"Installing: %d / %d",
        L"Ready",
        L"Loaded %d lines (skipped %d)",
        L"Repository file not found",
        L"Installing...",
        L"Successfully  ·  Installation complete",
        L"Error  ·  Check log",
        L"Failed to check for updates",
        L"You have the latest version",
        L"Update canceled",
        L"Failed to launch updater",
        L"Settings",
        L"Interface language:",
        L"Check for updates",
        L"About",
        L"Close",
        L"About",
        L"WinGET Launcher\nInstalled version: %ls\n\nGUI wrapper for Windows Package Manager (winget).",
        L"Search applications in winget",
        L"Search",
        L"Application",
        L"Add to repository",
        L"Close",
        L"Busy",
        L"Installation is already in progress.",
        L"Warning",
        L"No applications selected.",
        L"Logs",
        L"The logs folder has not been created yet.",
        L"Debug log file (debug.log) has not been created yet.",
        L"Error",
        L"File repositories.txt not found.",
        L"Confirmation",
        L"Installation is still running. Abort and exit?",
        L"Unsupported Windows version",
        L"Minimum supported Windows version is Windows 10 1809 (build %d).\n\nYour version: %lu.%lu (build %lu)\n\nThe launcher will now close.",
        L"winget not found",
        L"winget was not found.\n\nRun winget installation from source\\install_winget.bat?",
        L"Update",
        L"A new version is available!\n\nCurrent: %ls\nNew:     %ls\n\nThe launcher will close and setup.exe -update\nwill continue the update.\n\nContinue?",
        L"Please select an application from the list.",
        L"Information",
        L"This package is already in repositories.txt",
        L"Success",
        L"Added:\n%ls|%ls"
    }
};

static int g_LangIdx = 0;

// ============================================================
// Globals
// ============================================================
static HWND g_hWnd         = NULL;
static HWND g_hOutput      = NULL;
static HWND g_hStatus      = NULL;
static HWND g_hBtnInstall  = NULL;
static HWND g_hBtnExit     = NULL;
static HWND g_hBtnSelAll   = NULL;
static HWND g_hBtnSelNone  = NULL;
static HWND g_hBtnSearch   = NULL;
static HWND g_hBtnSettings = NULL;
static HWND g_hList        = NULL;
static HWND g_hProgTotal   = NULL;
static HWND g_hLblTotal    = NULL;

static HANDLE g_hProcess  = NULL;
static HANDLE g_hThread   = NULL;
static volatile LONG g_Running = 0;

static int g_TotalPackages = 0;
static int g_DonePackages  = 0;

static BOOL g_WaitingForResult = FALSE;
static BOOL g_BatFinished      = FALSE;
static int  g_BatFinalRc       = -1;
static BOOL g_StatusSuccess    = FALSE;

static HFONT  g_hFont        = NULL;
static HFONT  g_hFontBold    = NULL;
static HBRUSH g_hBkgBrush    = NULL;
static HBRUSH g_hPanelBrush  = NULL;
static HBRUSH g_hInputBrush  = NULL;

static wchar_t g_OSName[128] = L"";

static HWND g_hSearchWnd    = NULL;
static HWND g_hSearchEdit   = NULL;
static HWND g_hSearchList   = NULL;
static HWND g_hSearchBtn    = NULL;
static HWND g_hSearchProg   = NULL;
static WNDPROC g_OldEditProc = NULL;
static volatile LONG g_SearchRunning = 0;

static HWND g_hSettingsWnd  = NULL;
static volatile LONG g_UpdateRunning = 0;

// Debug mode & logging
static BOOL g_DebugMode = FALSE;
static CRITICAL_SECTION g_DebugCs;
static BOOL g_DebugCsInit = FALSE;
static wchar_t g_DebugLogPath[MAX_PATH] = {0};

// ============================================================
// Path Utilities
// ============================================================
static BOOL GetExeDir(wchar_t* out, size_t cch) {
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) return FALSE;
    wchar_t* slash = wcsrchr(exePath, L'\\');
    if (!slash) return FALSE;
    *(slash + 1) = L'\0';
    wcsncpy(out, exePath, cch);
    out[cch - 1] = L'\0';
    return TRUE;
}

static BOOL GetBatPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%sinstall_apps.bat", dir);
    return TRUE;
}

static BOOL GetRepoPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%ssource\\repositories.txt", dir);
    return TRUE;
}

static BOOL GetTmpRepoPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%ssource\\repositories.tmp.txt", dir);
    return TRUE;
}

static BOOL GetSearchPs1Path(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%ssource\\search_winget.ps1", dir);
    return TRUE;
}

static BOOL GetVersionPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%sversion.txt", dir);
    return TRUE;
}

// ============================================================
// Debug Logger (-debug)
// ============================================================
static void DebugLogInit(void) {
    if (!g_DebugMode) return;
    if (!g_DebugCsInit) {
        InitializeCriticalSection(&g_DebugCs);
        g_DebugCsInit = TRUE;
    }

    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return;

    wchar_t logsDir[MAX_PATH];
    _snwprintf(logsDir, MAX_PATH, L"%slogs", dir);
    CreateDirectoryW(logsDir, NULL);

    _snwprintf(g_DebugLogPath, MAX_PATH, L"%s\\debug.log", logsDir);

    EnterCriticalSection(&g_DebugCs);
    FILE* f = _wfopen(g_DebugLogPath, L"wb");
    if (f) {
        const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
        fwrite(bom, 1, 3, f);
        fclose(f);
    }
    LeaveCriticalSection(&g_DebugCs);
}

static void DebugLog(const wchar_t* category, const wchar_t* fmt, ...) {
    if (!g_DebugMode || !g_DebugLogPath[0]) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t msg[2048];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(msg, 2048, fmt, ap);
    msg[2047] = L'\0';
    va_end(ap);

    wchar_t fullLine[2304];
    _snwprintf(fullLine, 2304, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] [TID:%lu] [%ls] %ls\r\n",
               st.wYear, st.wMonth, st.wDay,
               st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
               GetCurrentThreadId(), category, msg);
    fullLine[2303] = L'\0';

    char utf8[6912];
    int len = WideCharToMultiByte(CP_UTF8, 0, fullLine, -1, utf8, sizeof(utf8), NULL, NULL);
    if (len > 1) {
        if (g_DebugCsInit) EnterCriticalSection(&g_DebugCs);
        FILE* f = _wfopen(g_DebugLogPath, L"ab");
        if (f) {
            fwrite(utf8, 1, (size_t)(len - 1), f);
            fclose(f);
        }
        if (g_DebugCsInit) LeaveCriticalSection(&g_DebugCs);
    }
}

static void DebugLogRawScriptOutput(const char* sourceTag, const char* oemText) {
    if (!g_DebugMode || !g_DebugLogPath[0] || !oemText || !*oemText) return;

    int wlen = MultiByteToWideChar(CP_OEMCP, 0, oemText, -1, NULL, 0);
    if (wlen <= 0) return;
    wchar_t* wbuf = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wbuf) return;
    MultiByteToWideChar(CP_OEMCP, 0, oemText, -1, wbuf, wlen);

    wchar_t* ctx = NULL;
    wchar_t* line = wcstok_s(wbuf, L"\r\n", &ctx);
    while (line) {
        if (*line) {
            wchar_t wTag[64] = {0};
            MultiByteToWideChar(CP_ACP, 0, sourceTag, -1, wTag, 64);
            DebugLog(wTag, L"%ls", line);
        }
        line = wcstok_s(NULL, L"\r\n", &ctx);
    }
    free(wbuf);
}

// ============================================================
// UI & ListView Utilities
// ============================================================
static void AppendOutput(const wchar_t* text) {
    if (!g_hOutput || !text) return;
    int len = GetWindowTextLengthW(g_hOutput);
    SendMessageW(g_hOutput, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_hOutput, EM_REPLACESEL, FALSE, (LPARAM)text);
}

static void SetStatus(const wchar_t* text) {
    if (g_hStatus) SetWindowTextW(g_hStatus, text);
    DebugLog(L"STATUS", L"%ls", text ? text : L"(null)");
}

static void EnableButtons(BOOL enable) {
    EnableWindow(g_hBtnInstall, enable);
    EnableWindow(g_hBtnSelAll, enable);
    EnableWindow(g_hBtnSelNone, enable);
    EnableWindow(g_hBtnSearch, enable);
    EnableWindow(g_hList, enable);
    if (g_hSettingsWnd) {
        EnableWindow(GetDlgItem(g_hSettingsWnd, ID_SET_BTN_REPO), enable);
        EnableWindow(GetDlgItem(g_hSettingsWnd, ID_SET_BTN_LOCAL_EXE), enable);
        EnableWindow(GetDlgItem(g_hSettingsWnd, ID_SET_BTN_LOGS), enable);
    }
}

static void ListView_GetItemTextW_Helper(HWND hList, int iItem, int iSubItem, wchar_t* outText, int cchTextMax) {
    LVITEMW lvi = {0};
    lvi.iSubItem = iSubItem;
    lvi.cchTextMax = cchTextMax;
    lvi.pszText = outText;
    SendMessageW(hList, LVM_GETITEMTEXTW, (WPARAM)iItem, (LPARAM)&lvi);
}

// ============================================================
// Real Windows version via RtlGetVersion
// ============================================================
typedef NTSTATUS (WINAPI *pRtlGetVersion)(PRTL_OSVERSIONINFOW);

static BOOL GetRealOSVersion(RTL_OSVERSIONINFOW* out) {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return FALSE;
    pRtlGetVersion fn = (pRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
    if (!fn) return FALSE;
    out->dwOSVersionInfoSize = sizeof(*out);
    return fn(out) == 0;
}

static BOOL IsWindowsSupported(RTL_OSVERSIONINFOW* outVer) {
    RTL_OSVERSIONINFOW osvi = {0};
    if (!GetRealOSVersion(&osvi)) {
        if (outVer) *outVer = osvi;
        DebugLog(L"SYS", L"RtlGetVersion failed!");
        return FALSE;
    }
    if (outVer) *outVer = osvi;

    DebugLog(L"SYS", L"Detected OS: %lu.%lu Build %lu (Min required build: %d)",
             osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, WIN10_MIN_BUILD);

    if (osvi.dwMajorVersion < 10) return FALSE;
    if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber < WIN10_MIN_BUILD) return FALSE;
    return TRUE;
}

static void DetectOS(void) {
    RTL_OSVERSIONINFOW osvi = {0};
    if (GetRealOSVersion(&osvi)) {
        _snwprintf(g_OSName, 128, L"Windows %lu.%lu (build %lu)",
                   osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    } else {
        wcscpy(g_OSName, L"Windows (unknown)");
    }
}

// ============================================================
// Check winget installed
// ============================================================
static BOOL IsWingetInstalled(void) {
    wchar_t cmdLine[] = L"winget.exe --version";
    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};

    wchar_t fullCmd[MAX_PATH * 2];
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    _snwprintf(fullCmd, MAX_PATH * 2, L"\"%s\\cmd.exe\" /c %s >nul 2>&1", sysDir, cmdLine);

    DebugLog(L"WINGET_CHECK", L"Executing: %ls", fullCmd);

    if (CreateProcessW(NULL, fullCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 5000);
        DWORD rc = 1;
        GetExitCodeProcess(pi.hProcess, &rc);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        DebugLog(L"WINGET_CHECK", L"Exit code: %lu (Installed: %ls)", rc, rc == 0 ? L"YES" : L"NO");
        return rc == 0;
    }
    DebugLog(L"WINGET_CHECK", L"CreateProcessW failed, err=%lu", GetLastError());
    return FALSE;
}

// ============================================================
// WinINet & Version
// ============================================================
static BOOL DownloadToStringW(const wchar_t* url, wchar_t* out, size_t outCch) {
    DebugLog(L"NET", L"Downloading URL: %ls", url);
    HINTERNET hNet = NULL, hUrl = NULL;
    hNet = InternetOpenW(L"WinGETLauncher/1.2", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) {
        DebugLog(L"NET", L"InternetOpenW failed, err=%lu", GetLastError());
        return FALSE;
    }
    hUrl = InternetOpenUrlW(hNet, url, NULL, 0,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        DebugLog(L"NET", L"InternetOpenUrlW failed, err=%lu", GetLastError());
        InternetCloseHandle(hNet);
        return FALSE;
    }

    char buf[8192];
    DWORD read = 0;
    char accum[16384] = {0};
    size_t pos = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read > 0) {
        if (pos + read >= sizeof(accum) - 1) break;
        memcpy(accum + pos, buf, read);
        pos += read;
    }
    accum[pos] = '\0';

    unsigned char* ub = (unsigned char*)accum;
    if (pos >= 3 && ub[0] == 0xEF && ub[1] == 0xBB && ub[2] == 0xBF) {
        memmove(accum, accum + 3, pos - 3 + 1);
        pos -= 3;
    }

    char* p = accum;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    char* end = p + strlen(p);
    while (end > p && (end[-1] == ' ' || end[-1] == '\t' ||
                      end[-1] == '\r' || end[-1] == '\n')) {
        *(--end) = '\0';
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, p, -1, out, (int)outCch);
    if (wlen <= 0) wlen = MultiByteToWideChar(CP_ACP, 0, p, -1, out, (int)outCch);
    BOOL ok = wlen > 0;

    DebugLog(L"NET", L"Downloaded %zu bytes -> '%ls'", pos, ok ? out : L"(decode error)");

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    return ok;
}

static BOOL ReadLocalVersion(wchar_t* out, size_t cch) {
    wchar_t path[MAX_PATH];
    if (GetVersionPath(path, MAX_PATH)) {
        FILE* f = _wfopen(path, L"rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (fsize > 0 && fsize <= 4096) {
                char raw[4096] = {0};
                fread(raw, 1, fsize, f);
                fclose(f);

                char* p = raw;
                if (fsize >= 3 &&
                    (unsigned char)p[0] == 0xEF &&
                    (unsigned char)p[1] == 0xBB &&
                    (unsigned char)p[2] == 0xBF) {
                    p += 3;
                }

                while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
                char* end = p + strlen(p);
                while (end > p && (end[-1] == '\r' || end[-1] == '\n' ||
                                  end[-1] == ' '  || end[-1] == '\t')) {
                    *(--end) = '\0';
                }
                if (*p != '\0') {
                    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, -1, NULL, 0);
                    if (wlen > 0) {
                        MultiByteToWideChar(CP_UTF8, 0, p, -1, out, (int)cch);
                    } else {
                        MultiByteToWideChar(CP_ACP, 0, p, -1, out, (int)cch);
                    }
                    DebugLog(L"VERSION", L"Read from version.txt: '%ls'", out);
                    return TRUE;
                }
            } else {
                fclose(f);
            }
        }
    }
    wcsncpy(out, APP_VERSION_STR, cch);
    out[cch - 1] = L'\0';
    DebugLog(L"VERSION", L"Using embedded APP_VERSION_STR: '%ls'", out);
    return TRUE;
}

// ============================================================
// Update via setup.exe -update
// ============================================================
static DWORD WINAPI UpdateThread(LPVOID param) {
    (void)param;
    DebugLog(L"UPDATE", L"Starting update check thread...");

    wchar_t remoteVer[256] = {0};
    wchar_t localVer[256]  = {0};

    ReadLocalVersion(localVer, 256);
    if (!DownloadToStringW(GH_VERSION, remoteVer, 256)) {
        DebugLog(L"UPDATE", L"Failed to fetch remote version.");
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 1, 0);
        return 0;
    }

    DebugLog(L"UPDATE", L"Compare versions: local='%ls' vs remote='%ls'", localVer, remoteVer);

    if (wcscmp(remoteVer, localVer) == 0) {
        DebugLog(L"UPDATE", L"Already up-to-date.");
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 2, 0);
        return 0;
    }

    const LangPack* L = &g_Lang[g_LangIdx];
    wchar_t msg[512];
    _snwprintf(msg, 512, L->msgUpdAvailFmt, localVer, remoteVer);

    if (MessageBoxW(g_hWnd, msg, L->msgUpdAvailTitle,
                    MB_YESNO | MB_ICONQUESTION) != IDYES) {
        DebugLog(L"UPDATE", L"User declined update.");
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 3, 0);
        return 0;
    }

    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) {
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 4, 0);
        return 0;
    }

    wchar_t setupPath[MAX_PATH];
    _snwprintf(setupPath, MAX_PATH, L"%ssetup.exe", dir);
    DebugLog(L"UPDATE", L"Launching setup updater: %ls -update", setupPath);

    if (GetFileAttributesW(setupPath) == INVALID_FILE_ATTRIBUTES) {
        DebugLog(L"UPDATE", L"setup.exe not found!");
        wchar_t emsg[512];
        _snwprintf(emsg, 512, L"setup.exe not found:\n%ls", setupPath);
        MessageBoxW(g_hWnd, emsg, L->msgErrTitle, MB_ICONERROR);
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 4, 0);
        return 0;
    }

    const wchar_t* setupArgs = g_DebugMode ? L"-update -debug" : L"-update";

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb       = L"open";
    sei.lpFile       = setupPath;
    sei.lpParameters = setupArgs;
    sei.lpDirectory  = dir;
    sei.nShow        = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        DebugLog(L"UPDATE", L"ShellExecuteExW failed for setup.exe -update, err=%lu", GetLastError());
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 4, 0);
        return 0;
    }

    if (sei.hProcess) CloseHandle(sei.hProcess);

    DebugLog(L"UPDATE", L"setup.exe -update started, closing launcher.");
    PostMessageW(g_hWnd, WM_CLOSE, 0, 0);
    return 0;
}

static void CheckForUpdates(void) {
    if (InterlockedCompareExchange(&g_UpdateRunning, 1, 0) != 0) return;
    HANDLE h = CreateThread(NULL, 0, UpdateThread, NULL, 0, NULL);
    if (h) CloseHandle(h);
}

// ============================================================
// Language switcher
// ============================================================
static void ApplyLanguage(int idx) {
    if (idx < 0 || idx > 1) return;
    g_LangIdx = idx;
    DebugLog(L"UI", L"Language switched to index %d (%ls)", idx, idx == 0 ? L"RU" : L"EN");
    const LangPack* L = &g_Lang[g_LangIdx];

    if (g_hBtnInstall)  SetWindowTextW(g_hBtnInstall,  L->btnInstall);
    if (g_hBtnSelAll)   SetWindowTextW(g_hBtnSelAll,   L->btnSelAll);
    if (g_hBtnSelNone)  SetWindowTextW(g_hBtnSelNone,  L->btnSelNone);
    if (g_hBtnSearch)   SetWindowTextW(g_hBtnSearch,   L->btnSearch);
    if (g_hBtnSettings) SetWindowTextW(g_hBtnSettings, L->btnSettings);
    if (g_hBtnExit)     SetWindowTextW(g_hBtnExit,     L->btnExit);

    if (g_hLblTotal) {
        wchar_t buf[256];
        _snwprintf(buf, 256, L->lblTotalFmt, g_DonePackages, g_TotalPackages);
        SetWindowTextW(g_hLblTotal, buf);
    }

    if (!g_Running && g_hStatus) {
        SetStatus(L->statusReady);
    }

    if (g_hSettingsWnd) {
        SetWindowTextW(g_hSettingsWnd, L->setTitle);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_LBL_LANG, L->setLangLabel);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_REPO, L->btnOpenRepo);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_LOCAL_EXE, L->btnLocalExe);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_LOGS, L->btnLogs);
        if (g_DebugMode) {
            SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_DBGLOG, L->btnFullLog);
        }
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_UPDATE, L->setBtnUpdate);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_ABOUT, L->setBtnAbout);
        SetDlgItemTextW(g_hSettingsWnd, ID_SET_BTN_CLOSE, L->setBtnClose);
        InvalidateRect(g_hSettingsWnd, NULL, TRUE);
    }

    InvalidateRect(g_hWnd, NULL, TRUE);
}

// ============================================================
// Progress
// ============================================================
static void SetTotalProgress(int current, int total) {
    if (!g_hProgTotal) return;
    if (total <= 0) total = 1;
    SendMessageW(g_hProgTotal, PBM_SETRANGE, 0, MAKELPARAM(0, total));
    SendMessageW(g_hProgTotal, PBM_SETPOS, (WPARAM)current, 0);

    if (g_hLblTotal) {
        wchar_t buf[256];
        _snwprintf(buf, 256, g_Lang[g_LangIdx].lblTotalFmt, current, total);
        SetWindowTextW(g_hLblTotal, buf);
    }
    DebugLog(L"PROGRESS", L"Total progress updated: %d / %d", current, total);
}

static void ParseWingetLine(const wchar_t* line) {
    if (wcsstr(line, L"[BAT_DONE]")) {
        const wchar_t* p = wcsstr(line, L"rc=");
        if (p) g_BatFinalRc = _wtoi(p + 3);
        g_BatFinished = TRUE;
        InterlockedExchange(&g_Running, 0);
        DebugLog(L"PARSER", L"Detected [BAT_DONE] with rc=%d", g_BatFinalRc);
        return;
    }
    if (wcsstr(line, L"[INSTALL]")) {
        g_WaitingForResult = TRUE;
        DebugLog(L"PARSER", L"Detected [INSTALL] marker: %ls", line);
        return;
    }
    if (!g_WaitingForResult) return;
    if (wcsstr(line, L"[OK]") || wcsstr(line, L"[SKIP]") ||
        wcsstr(line, L"[WARN]") || wcsstr(line, L"[FAIL]")) {
        DebugLog(L"PARSER", L"Detected package result marker: %ls", line);
        g_DonePackages++;
        SetTotalProgress(g_DonePackages, g_TotalPackages);
        g_WaitingForResult = FALSE;
    }
}

static void AppendOutputA(const char* text) {
    if (!text) return;
    DebugLogRawScriptOutput("BAT_STDOUT", text);

    int wlen = MultiByteToWideChar(CP_OEMCP, 0, text, -1, NULL, 0);
    if (wlen <= 0) return;
    wchar_t* wbuf = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    if (!wbuf) return;
    MultiByteToWideChar(CP_OEMCP, 0, text, -1, wbuf, wlen);
    AppendOutput(wbuf);

    wchar_t* ctx = NULL;
    wchar_t* nl = wcstok_s(wbuf, L"\r\n", &ctx);
    while (nl) { ParseWingetLine(nl); nl = wcstok_s(NULL, L"\r\n", &ctx); }
    free(wbuf);
}

// ============================================================
// Repository parser
// ============================================================
static int ReadRepoLine(FILE* f, wchar_t* out_id, size_t id_cch,
                                   wchar_t* out_name, size_t name_cch) {
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;

        char* end = p + strlen(p);
        while (end > p && (end[-1] == '\r' || end[-1] == '\n' ||
                          end[-1] == ' ' || end[-1] == '\t')) {
            *(--end) = '\0';
        }

        if (*p == '\0') continue;
        if (*p == '#') continue;

        if (*p == '[') {
            char* close = strchr(p, ']');
            if (close) {
                *close = '\0';
                char* cat = p + 1;
                while (*cat == ' ' || *cat == '\t') cat++;
                char* ce = cat + strlen(cat);
                while (ce > cat && (ce[-1] == ' ' || ce[-1] == '\t')) *(--ce) = '\0';

                if (*cat != '\0') {
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, cat, -1, out_name, (int)name_cch);
                    if (wlen <= 0) wlen = MultiByteToWideChar(CP_ACP, 0, cat, -1, out_name, (int)name_cch);
                    return 2;
                }
            }
            continue;
        }

        char* part1 = p;
        char* part2 = NULL;

        char* bar1 = strchr(p, '|');
        if (bar1) {
            *bar1 = '\0';
            part2 = bar1 + 1;
            char* bar2 = strchr(part2, '|');
            if (bar2) *bar2 = '\0';
        }

        #define TRIM(s) do { \
            while (*(s) == ' ' || *(s) == '\t') (s)++; \
            char* _e = (s) + strlen(s); \
            while (_e > (s) && (_e[-1] == ' ' || _e[-1] == '\t')) *(--_e) = '\0'; \
        } while(0)

        TRIM(part1);
        if (part2) TRIM(part2);
        if (*part1 == '\0') continue;

        int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part1, -1, NULL, 0);
        if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part1, -1, out_id, (int)id_cch);
        else          MultiByteToWideChar(CP_ACP, 0, part1, -1, out_id, (int)id_cch);

        if (part2 && *part2 != '\0') {
            wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part2, -1, NULL, 0);
            if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part2, -1, out_name, (int)name_cch);
            else          MultiByteToWideChar(CP_ACP, 0, part2, -1, out_name, (int)name_cch);
        } else {
            wcsncpy(out_name, out_id, name_cch);
            out_name[name_cch - 1] = L'\0';
        }
        return 1;
    }
    return 0;
}

// ============================================================
// LoadRepository
// ============================================================
static void LoadRepository(void) {
    wchar_t repoPath[MAX_PATH];
    if (!GetRepoPath(repoPath, MAX_PATH)) return;
    DebugLog(L"REPO", L"Loading repository from: %ls", repoPath);

    FILE* f = _wfopen(repoPath, L"rb");
    if (!f) {
        DebugLog(L"REPO", L"Failed to open repositories.txt!");
        AppendOutput(L"[ERROR] Cannot open source\\repositories.txt\r\n");
        SetStatus(g_Lang[g_LangIdx].statusRepoNotFound);
        return;
    }

    unsigned char bom[3];
    if (fread(bom, 1, 3, f) != 3 ||
        !(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
        rewind(f);
    }

    SendMessageW(g_hList, LVM_DELETEALLITEMS, 0, 0);

    wchar_t w_id[256], w_name[256];
    int idx = 0;
    int skipped = 0;
    int rc;

    while ((rc = ReadRepoLine(f, w_id, 256, w_name, 256)) != 0) {
        if (rc == 2) {
            wchar_t header[300];
            _snwprintf(header, 300, L"── %ls ──", w_name);

            LVITEMW item = {0};
            item.mask = LVIF_TEXT;
            item.iItem = idx;
            item.iSubItem = 0;
            item.pszText = header;
            int row = (int)SendMessageW(g_hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

            LVITEMW sub1 = {0};
            sub1.mask = LVIF_TEXT;
            sub1.iItem = row; sub1.iSubItem = 1;
            sub1.pszText = L"";
            SendMessageW(g_hList, LVM_SETITEMW, 0, (LPARAM)&sub1);

            ListView_SetCheckState(g_hList, row, FALSE);
            DebugLog(L"REPO", L"Category header [%d]: '%ls'", idx, w_name);
            idx++;
            continue;
        }

        if (!w_id[0]) { skipped++; continue; }

        LVITEMW item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = w_name;
        int row = (int)SendMessageW(g_hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

        LVITEMW sub1 = {0};
        sub1.mask = LVIF_TEXT;
        sub1.iItem = row; sub1.iSubItem = 1;
        sub1.pszText = w_id;
        SendMessageW(g_hList, LVM_SETITEMW, 0, (LPARAM)&sub1);

        ListView_SetCheckState(g_hList, row, TRUE);
        DebugLog(L"REPO", L"Item [%d]: ID='%ls', Name='%ls'", idx, w_id, w_name);
        idx++;
    }
    fclose(f);

    DebugLog(L"REPO", L"Repository load complete: %d rows, %d skipped", idx, skipped);

    wchar_t status[256];
    _snwprintf(status, 256, g_Lang[g_LangIdx].statusLoadedFmt, idx, skipped);
    SetStatus(status);
}

static int GetCheckedCount(void) {
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    int checked = 0;
    for (int i = 0; i < count; i++) {
        wchar_t wid[256] = {0};
        ListView_GetItemTextW_Helper(g_hList, i, 1, wid, 256);
        if (wid[0] == L'\0') continue;
        if (ListView_GetCheckState(g_hList, i)) checked++;
    }
    return checked;
}

static BOOL WriteTmpRepo(const wchar_t* path) {
    DebugLog(L"TMP_REPO", L"Writing temporary repo file: %ls", path);
    FILE* f = _wfopen(path, L"wb");
    if (!f) {
        DebugLog(L"TMP_REPO", L"Failed to open temp repo file for writing!");
        return FALSE;
    }

    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    int written = 0;
    for (int i = 0; i < count; i++) {
        if (!ListView_GetCheckState(g_hList, i)) continue;

        wchar_t wname[256]  = {0};
        wchar_t wid[256]    = {0};

        ListView_GetItemTextW_Helper(g_hList, i, 0, wname,  256);
        ListView_GetItemTextW_Helper(g_hList, i, 1, wid,    256);

        if (wid[0] == L'\0') continue;

        char utf8_wid[1024];
        char utf8_name[1024];

        WideCharToMultiByte(CP_UTF8, 0, wid,    -1, utf8_wid,   1024, NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, wname,  -1, utf8_name,  1024, NULL, NULL);

        char line[4096];
        _snprintf(line, sizeof(line), "%s|%s\n", utf8_wid, utf8_name);
        fwrite(line, 1, strlen(line), f);
        DebugLog(L"TMP_REPO", L"  Queued [%d]: %ls|%ls", written + 1, wid, wname);
        written++;
    }
    fclose(f);
    DebugLog(L"TMP_REPO", L"Total items written to temp repo: %d", written);
    return TRUE;
}

// ============================================================
// Run .bat
// ============================================================
static DWORD WINAPI RunBatThread(LPVOID param) {
    (void)param;
    DebugLog(L"BAT_EXEC", L"RunBatThread started.");

    wchar_t batPath[MAX_PATH], tmpRepo[MAX_PATH];
    if (!GetBatPath(batPath, MAX_PATH) || !GetTmpRepoPath(tmpRepo, MAX_PATH)) {
        DebugLog(L"BAT_EXEC", L"Failed to resolve paths.");
        AppendOutput(L"[ERROR] Cannot resolve paths.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }
    if (GetFileAttributesW(batPath) == INVALID_FILE_ATTRIBUTES) {
        DebugLog(L"BAT_EXEC", L"install_apps.bat not found at: %ls", batPath);
        AppendOutput(L"[ERROR] install_apps.bat not found next to launcher.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    DeleteFileW(tmpRepo);
    if (!WriteTmpRepo(tmpRepo)) {
        AppendOutput(L"[ERROR] Cannot write temporary repository file.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    SetEnvironmentVariableW(L"REPO_FILE", tmpRepo);
    DebugLog(L"BAT_EXEC", L"Set env REPO_FILE=%ls", tmpRepo);

    if (g_DebugMode) {
        SetEnvironmentVariableW(L"LAUNCHER_DEBUG", L"1");
        DebugLog(L"BAT_EXEC", L"Set env LAUNCHER_DEBUG=1");
    }

    wchar_t cmdLine[MAX_PATH * 2];
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    _snwprintf(cmdLine, MAX_PATH * 2, L"\"%s\\cmd.exe\" /c \"\"%s\"\"", sysDir, batPath);
    DebugLog(L"BAT_EXEC", L"Command line: %ls", cmdLine);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        DebugLog(L"BAT_EXEC", L"CreatePipe failed, err=%lu", GetLastError());
        AppendOutput(L"[ERROR] CreatePipe failed.\r\n");
        DeleteFileW(tmpRepo);
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite; si.hStdError = hWrite;

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hWrite);

    if (!ok) {
        DebugLog(L"BAT_EXEC", L"CreateProcessW failed, err=%lu", GetLastError());
        AppendOutput(L"[ERROR] CreateProcess failed.\r\n");
        CloseHandle(hRead); DeleteFileW(tmpRepo);
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    g_hProcess = pi.hProcess;
    DebugLog(L"BAT_EXEC", L"Child process created (PID=%lu)", pi.dwProcessId);

    char buf[4096]; DWORD read = 0;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        buf[read] = '\0';
        AppendOutputA(buf);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 0;
    GetExitCodeProcess(pi.hProcess, &rc);
    DebugLog(L"BAT_EXEC", L"Child process exited with code: %lu (g_BatFinished=%d, g_BatFinalRc=%d)",
             rc, g_BatFinished, g_BatFinalRc);

    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(hRead);
    g_hProcess = NULL;
    DeleteFileW(tmpRepo);
    DebugLog(L"BAT_EXEC", L"Deleted temporary repo file.");

    wchar_t msg[128];
    _snwprintf(msg, 128, L"\r\n=== Done. Exit code: %lu ===\r\n", rc);
    AppendOutput(msg);

    int finalRc = g_BatFinished ? g_BatFinalRc : (int)rc;
    if (finalRc == 0) { SetStatus(g_Lang[g_LangIdx].statusDoneOk); g_StatusSuccess = TRUE; }
    else              { SetStatus(g_Lang[g_LangIdx].statusDoneErr); g_StatusSuccess = FALSE; }

    InterlockedExchange(&g_Running, 0);
    EnableButtons(TRUE);
    InvalidateRect(g_hStatus, NULL, TRUE);
    return 0;
}

// ============================================================
// Handlers
// ============================================================
static void OnInstallClicked(void) {
    DebugLog(L"UI", L"Install button clicked.");
    const LangPack* L = &g_Lang[g_LangIdx];
    if (InterlockedCompareExchange(&g_Running, 1, 0) != 0) {
        MessageBoxW(g_hWnd, L->msgBusyText, L->msgBusyTitle, MB_ICONINFORMATION);
        return;
    }
    int selected = GetCheckedCount();
    DebugLog(L"UI", L"Selected packages count: %d", selected);
    if (selected == 0) {
        MessageBoxW(g_hWnd, L->msgNoApps, L->msgWarnTitle, MB_ICONWARNING);
        InterlockedExchange(&g_Running, 0); return;
    }

    g_TotalPackages = selected; g_DonePackages = 0;
    g_WaitingForResult = FALSE; g_BatFinished = FALSE;
    g_BatFinalRc = -1; g_StatusSuccess = FALSE;
    SetTotalProgress(0, g_TotalPackages);

    SetWindowTextW(g_hOutput, L"");
    wchar_t msg[128];
    _snwprintf(msg, 128, L"=== install_apps.bat (selected: %d) ===\r\n\r\n", selected);
    AppendOutput(msg);
    SetStatus(L->statusInstalling);
    EnableButtons(FALSE);

    g_hThread = CreateThread(NULL, 0, RunBatThread, NULL, 0, NULL);
    if (!g_hThread) {
        DebugLog(L"UI", L"CreateThread for RunBatThread failed, err=%lu", GetLastError());
        AppendOutput(L"[ERROR] CreateThread failed.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE);
    } else { CloseHandle(g_hThread); g_hThread = NULL; }
}

static void OnLogsClicked(void) {
    DebugLog(L"UI", L"Open logs button clicked.");
    const LangPack* L = &g_Lang[g_LangIdx];
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return;
    wchar_t logsDir[MAX_PATH];
    _snwprintf(logsDir, MAX_PATH, L"%slogs", dir);
    if (GetFileAttributesW(logsDir) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L->msgNoLogs, L->msgLogsTitle, MB_ICONINFORMATION);
        return;
    }
    ShellExecuteW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L"open", logsDir, NULL, NULL, SW_SHOWNORMAL);
}

static void OnDebugLogClicked(void) {
    DebugLog(L"UI", L"Open full debug log button clicked.");
    const LangPack* L = &g_Lang[g_LangIdx];
    if (!g_DebugLogPath[0] || GetFileAttributesW(g_DebugLogPath) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L->msgNoDebugLog, L->msgLogsTitle, MB_ICONINFORMATION);
        return;
    }
    ShellExecuteW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L"open", g_DebugLogPath, NULL, NULL, SW_SHOWNORMAL);
}

static void OnOpenRepoClicked(void) {
    DebugLog(L"UI", L"Open repository button clicked.");
    const LangPack* L = &g_Lang[g_LangIdx];
    wchar_t repoPath[MAX_PATH];
    if (!GetRepoPath(repoPath, MAX_PATH)) return;
    if (GetFileAttributesW(repoPath) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L->msgNoRepoFile, L->msgErrTitle, MB_ICONERROR);
        return;
    }
    ShellExecuteW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L"open", L"notepad.exe", repoPath, NULL, SW_SHOWNORMAL);
}

static void SetAllCheckState(BOOL state) {
    DebugLog(L"UI", L"SetAllCheckState(%d)", state);
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        wchar_t wid[256] = {0};
        ListView_GetItemTextW_Helper(g_hList, i, 1, wid, 256);
        if (wid[0] == L'\0') continue;
        ListView_SetCheckState(g_hList, i, state);
    }
}

// ============================================================
// Windows 8 Flat Button & Background Drawing
// ============================================================
static void DrawWin8Button(LPDRAWITEMSTRUCT dis, BOOL primaryAccent) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    BOOL pressed  = (dis->itemState & ODS_SELECTED) != 0;
    BOOL disabled = (dis->itemState & ODS_DISABLED) != 0;
    BOOL focused  = (dis->itemState & ODS_FOCUS) != 0;
    BOOL hot      = (dis->itemState & ODS_HOTLIGHT) != 0;

    COLORREF fillClr, borderClr, textClr;
    if (disabled) {
        fillClr = WIN8_CLR_BTN_DIS; borderClr = WIN8_CLR_BORDER; textClr = WIN8_CLR_TEXT_MUTED;
    } else if (pressed) {
        fillClr = WIN8_CLR_BTN_PRESS; borderClr = WIN8_CLR_BTN_PRESS; textClr = RGB(255, 255, 255);
    } else if (hot) {
        fillClr = WIN8_CLR_BTN_HOT; borderClr = WIN8_CLR_BTN_HOT_BDR; textClr = WIN8_CLR_TEXT;
    } else if (primaryAccent) {
        fillClr = WIN8_CLR_ACCENT_LIGHT; borderClr = WIN8_CLR_ACCENT; textClr = WIN8_CLR_TEXT;
    } else {
        fillClr = WIN8_CLR_BTN_FACE; borderClr = WIN8_CLR_BORDER; textClr = WIN8_CLR_TEXT;
    }

    HBRUSH hFill = CreateSolidBrush(fillClr);
    FillRect(hdc, &rc, hFill);
    DeleteObject(hFill);

    HPEN hPen = CreatePen(PS_SOLID, 1, borderClr);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);

    wchar_t text[128];
    GetWindowTextW(dis->hwndItem, text, 128);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textClr);
    HFONT hOldFont = (HFONT)SelectObject(hdc, primaryAccent ? g_hFontBold : g_hFont);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);

    if (focused && !disabled) {
        RECT rf = rc;
        InflateRect(&rf, -4, -4);
        DrawFocusRect(hdc, &rf);
    }
}

static void DrawFlatBackground(HWND hWnd, HDC hdc, COLORREF clr) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    HBRUSH hBr = CreateSolidBrush(clr);
    FillRect(hdc, &rc, hBr);
    DeleteObject(hBr);
}

static void OnLocalExeClicked(void) {
    wchar_t baseDir[MAX_PATH] = {0};
    GetExeDir(baseDir, MAX_PATH);
    wchar_t exeDir[MAX_PATH * 2];
    _snwprintf(exeDir, MAX_PATH * 2, L"%ssource\\exe", baseDir);

    DWORD attr = GetFileAttributesW(exeDir);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        CreateDirectoryW(exeDir, NULL);
    }

    DebugLog(L"LOCAL_EXE", L"Opening local installers folder: %ls", exeDir);
    ShellExecuteW(g_hSettingsWnd ? g_hSettingsWnd : g_hWnd, L"open", exeDir, NULL, NULL, SW_SHOWNORMAL);
}

// ============================================================
// Settings Window (Windows 8 Flat)
// ============================================================
static LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hSettingsWnd = hWnd;
        const LangPack* L = &g_Lang[g_LangIdx];

        HWND hLbl = CreateWindowW(L"STATIC", L->setLangLabel,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 16, 240, 20, hWnd, (HMENU)ID_SET_LBL_LANG, NULL, NULL);
        SendMessageW(hLbl, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        HWND hCombo = CreateWindowExW(0, L"COMBOBOX", NULL,
            CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            20, 38, 240, 100, hWnd, (HMENU)ID_SET_COMBO_LANG, NULL, NULL);
        SendMessageW(hCombo, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Русский (RUS)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"English (ENG)");
        SendMessageW(hCombo, CB_SETCURSEL, g_LangIdx, 0);

        DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;
        int y = 76;

        HWND hBtnRepo = CreateWindowW(L"BUTTON", L->btnOpenRepo,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_REPO, NULL, NULL);
        y += 40;

        HWND hBtnLocalExe = CreateWindowW(L"BUTTON", L->btnLocalExe,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_LOCAL_EXE, NULL, NULL);
        y += 40;

        HWND hBtnLogs = CreateWindowW(L"BUTTON", L->btnLogs,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_LOGS, NULL, NULL);
        y += 40;

        if (g_Running) {
            EnableWindow(hBtnRepo, FALSE);
            EnableWindow(hBtnLocalExe, FALSE);
            EnableWindow(hBtnLogs, FALSE);
        }

        if (g_DebugMode) {
            CreateWindowW(L"BUTTON", L->btnFullLog,
                btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_DBGLOG, NULL, NULL);
            y += 40;
        }

        CreateWindowW(L"BUTTON", L->setBtnUpdate,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_UPDATE, NULL, NULL);
        y += 40;

        CreateWindowW(L"BUTTON", L->setBtnAbout,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_ABOUT, NULL, NULL);
        y += 40;

        CreateWindowW(L"BUTTON", L->setBtnClose,
            btnStyle, 20, y, 240, 32, hWnd, (HMENU)ID_SET_BTN_CLOSE, NULL, NULL);

        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == ID_SET_COMBO_LANG && code == CBN_SELCHANGE) {
            int sel = (int)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
            ApplyLanguage(sel);
        } else if (id == ID_SET_BTN_REPO) {
            OnOpenRepoClicked();
        } else if (id == ID_SET_BTN_LOCAL_EXE) {
            OnLocalExeClicked();
        } else if (id == ID_SET_BTN_LOGS) {
            OnLogsClicked();
        } else if (id == ID_SET_BTN_DBGLOG) {
            OnDebugLogClicked();
        } else if (id == ID_SET_BTN_UPDATE) {
            CheckForUpdates();
        } else if (id == ID_SET_BTN_ABOUT) {
            const LangPack* L = &g_Lang[g_LangIdx];
            wchar_t ver[128] = {0};
            ReadLocalVersion(ver, 128);
            wchar_t aboutMsg[512];
            _snwprintf(aboutMsg, 512, L->aboutTextFmt, ver);
            MessageBoxW(hWnd, aboutMsg, L->aboutTitle, MB_OK | MB_ICONINFORMATION);
        } else if (id == ID_SET_BTN_CLOSE) {
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            BOOL isClose = (dis->CtlID == ID_SET_BTN_CLOSE);
            DrawWin8Button(dis, isClose);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hPanelBrush;
    }
    case WM_ERASEBKGND: {
        DrawFlatBackground(hWnd, (HDC)wParam, WIN8_CLR_PANEL);
        return 1;
    }
    case WM_DESTROY:
        g_hSettingsWnd = NULL;
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
// Search thread
// ============================================================
static DWORD WINAPI SearchThread(LPVOID param) {
    (void)param;
    DebugLog(L"SEARCH", L"SearchThread started.");
    if (!g_hSearchList) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }
    PostMessageW(g_hSearchList, LVM_DELETEALLITEMS, 0, 0);

    wchar_t query[256] = {0};
    GetWindowTextW(g_hSearchEdit, query, 256);
    DebugLog(L"SEARCH", L"Search query: '%ls'", query);
    if (!*query) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }

    wchar_t ps1Path[MAX_PATH];
    if (!GetSearchPs1Path(ps1Path, MAX_PATH)) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }
    if (GetFileAttributesW(ps1Path) == INVALID_FILE_ATTRIBUTES) {
        DebugLog(L"SEARCH", L"search_winget.ps1 not found at: %ls", ps1Path);
        MessageBoxW(g_hSearchWnd, L"search_winget.ps1 not found in source\\",
                    g_Lang[g_LangIdx].msgErrTitle, MB_ICONERROR);
        PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0);
        return 0;
    }

    wchar_t cmdLine[MAX_PATH * 4];
    _snwprintf(cmdLine, MAX_PATH * 4,
               L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%s\" -Query \"%s\"",
               ps1Path, query);
    DebugLog(L"SEARCH", L"Executing: %ls", cmdLine);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        DebugLog(L"SEARCH", L"CreatePipe failed, err=%lu", GetLastError());
        PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite; si.hStdError = hWrite;

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE,
                             CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hWrite);
    if (!ok) {
        DebugLog(L"SEARCH", L"CreateProcessW failed, err=%lu", GetLastError());
        CloseHandle(hRead); PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0;
    }

    char buf[4096]; DWORD read = 0;
    char lineBuf[8192]; int linePos = 0; int idx = 0;

    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        for (DWORD i = 0; i < read; i++) {
            char c = buf[i];
            if (c == '\n' || linePos >= (int)sizeof(lineBuf) - 1) {
                lineBuf[linePos] = '\0';
                int len = linePos;
                while (len > 0 && (lineBuf[len-1] == '\r' || lineBuf[len-1] == ' '))
                    lineBuf[--len] = '\0';

                if (len > 0) {
                    char* bar1 = strchr(lineBuf, '|');
                    if (bar1) {
                        *bar1 = '\0';
                        char* id = lineBuf;
                        char* name = bar1 + 1;
                        char* bar2 = strchr(name, '|');
                        if (bar2) *bar2 = '\0';

                        wchar_t wId[256] = {0}, wName[256] = {0};
                        MultiByteToWideChar(CP_UTF8, 0, id, -1, wId, 256);
                        MultiByteToWideChar(CP_UTF8, 0, name, -1, wName, 256);

                        DebugLog(L"SEARCH_OUT", L"Found [%d]: ID='%ls', Name='%ls'", idx, wId, wName);

                        LVITEMW item = {0};
                        item.mask = LVIF_TEXT;
                        item.iItem = idx; item.iSubItem = 0;
                        item.pszText = wName;
                        int row = (int)SendMessageW(g_hSearchList, LVM_INSERTITEMW, 0, (LPARAM)&item);

                        LVITEMW sub = {0};
                        sub.mask = LVIF_TEXT;
                        sub.iItem = row; sub.iSubItem = 1;
                        sub.pszText = wId;
                        SendMessageW(g_hSearchList, LVM_SETITEMW, 0, (LPARAM)&sub);
                        idx++;
                    } else {
                        wchar_t wRaw[512] = {0};
                        MultiByteToWideChar(CP_OEMCP, 0, lineBuf, -1, wRaw, 512);
                        DebugLog(L"SEARCH_RAW", L"%ls", wRaw);
                    }
                }
                linePos = 0;
            } else {
                lineBuf[linePos++] = c;
            }
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 0;
    GetExitCodeProcess(pi.hProcess, &rc);
    DebugLog(L"SEARCH", L"PowerShell search finished (exit=%lu, items=%d)", rc, idx);

    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(hRead);
    PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, (WPARAM)idx, 0);
    return 0;
}

static void AddSelectedToRepo(void) {
    const LangPack* L = &g_Lang[g_LangIdx];
    if (!g_hSearchList) return;
    int sel = (int)SendMessageW(g_hSearchList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(g_hSearchWnd, L->msgSelectProg, L->msgWarnTitle, MB_ICONWARNING);
        return;
    }
    wchar_t id[256] = {0}, name[256] = {0};
    ListView_GetItemTextW_Helper(g_hSearchList, sel, 1, id, 256);
    ListView_GetItemTextW_Helper(g_hSearchList, sel, 0, name, 256);

    char id_utf8[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, id, -1, id_utf8, 1024, NULL, NULL);

    wchar_t repoPath[MAX_PATH];
    if (!GetRepoPath(repoPath, MAX_PATH)) return;

    FILE* fCheck = _wfopen(repoPath, L"r, ccs=UTF-8");
    if (fCheck) {
        char line[512];
        while (fgets(line, sizeof(line), fCheck)) {
            if (strstr(line, id_utf8) != NULL) {
                fclose(fCheck);
                MessageBoxW(g_hSearchWnd, L->msgAlreadyInRepo,
                            L->msgInfoTitle, MB_ICONINFORMATION);
                return;
            }
        }
        fclose(fCheck);
    }

    FILE* f = _wfopen(repoPath, L"a, ccs=UTF-8");
    if (!f) {
        MessageBoxW(g_hSearchWnd, L->msgNoRepoFile, L->msgErrTitle, MB_ICONERROR);
        return;
    }
    fwprintf(f, L"%ls|%ls\n", id, name);
    fclose(f);

    DebugLog(L"REPO", L"Added new package from search: %ls|%ls", id, name);

    wchar_t msg[512];
    _snwprintf(msg, 512, L->msgAddedFmt, id, name);
    MessageBoxW(g_hSearchWnd, msg, L->msgSuccessTitle, MB_ICONINFORMATION);
}

static LRESULT CALLBACK SearchEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        HWND hParent = GetParent(hWnd);
        SendMessageW(hParent, WM_COMMAND, MAKEWPARAM(ID_SEARCH_BTN, BN_CLICKED), (LPARAM)g_hSearchBtn);
        return 0;
    }
    return CallWindowProcW(g_OldEditProc, hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK SearchWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hSearchWnd = hWnd;
        const LangPack* L = &g_Lang[g_LangIdx];

        g_hSearchEdit = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 10, 380, 26, hWnd, (HMENU)ID_SEARCH_EDIT, NULL, NULL);
        SendMessageW(g_hSearchEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        g_OldEditProc = (WNDPROC)SetWindowLongPtrW(g_hSearchEdit, GWLP_WNDPROC, (LONG_PTR)SearchEditProc);

        g_hSearchBtn = CreateWindowW(L"BUTTON", L->searchBtnFind,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            400, 10, 100, 26, hWnd, (HMENU)ID_SEARCH_BTN, NULL, NULL);

        g_hSearchList = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            10, 46, 490, 260, hWnd, (HMENU)ID_SEARCH_LIST, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hSearchList,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(g_hSearchList, WIN8_CLR_BG);
        ListView_SetTextBkColor(g_hSearchList, WIN8_CLR_BG);
        ListView_SetTextColor(g_hSearchList, WIN8_CLR_TEXT);

        LVCOLUMNW col0 = {0};
        col0.mask = LVCF_TEXT | LVCF_WIDTH;
        col0.pszText = (LPWSTR)L->searchColProg; col0.cx = 250;
        SendMessageW(g_hSearchList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col0);

        LVCOLUMNW col1 = {0};
        col1.mask = LVCF_TEXT | LVCF_WIDTH;
        col1.pszText = L"ID"; col1.cx = 230;
        SendMessageW(g_hSearchList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col1);
        SendMessageW(g_hSearchList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hSearchProg = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | PBS_MARQUEE,
            10, 312, 490, 14, hWnd, (HMENU)ID_SEARCH_PROGRESS, NULL, NULL);
        SendMessageW(g_hSearchProg, PBM_SETMARQUEE, TRUE, 30);
        SendMessageW(g_hSearchProg, PBM_SETBARCOLOR, 0, WIN8_CLR_ACCENT);

        CreateWindowW(L"BUTTON", L->searchBtnAdd,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            10, 335, 200, 30, hWnd, (HMENU)ID_SEARCH_ADD, NULL, NULL);

        CreateWindowW(L"BUTTON", L->searchBtnClose,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            410, 335, 90, 30, hWnd, (HMENU)ID_SEARCH_CLOSE, NULL, NULL);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_SEARCH_BTN: {
            if (InterlockedCompareExchange(&g_SearchRunning, 1, 0) != 0) break;
            ShowWindow(g_hSearchProg, SW_SHOW);
            EnableWindow(g_hSearchBtn, FALSE);
            EnableWindow(g_hSearchEdit, FALSE);
            HANDLE hThread = CreateThread(NULL, 0, SearchThread, NULL, 0, NULL);
            if (hThread) CloseHandle(hThread);
            break;
        }
        case ID_SEARCH_ADD: AddSelectedToRepo(); break;
        case ID_SEARCH_CLOSE: DestroyWindow(hWnd); break;
        }
        return 0;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            BOOL primary = (dis->CtlID == ID_SEARCH_BTN || dis->CtlID == ID_SEARCH_ADD);
            DrawWin8Button(dis, primary);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, WIN8_CLR_BG);
        SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hInputBrush;
    }
    case WM_ERASEBKGND: {
        DrawFlatBackground(hWnd, (HDC)wParam, WIN8_CLR_PANEL);
        return 1;
    }
    case WM_APP_SEARCH_DONE: {
        ShowWindow(g_hSearchProg, SW_HIDE);
        EnableWindow(g_hSearchBtn, TRUE);
        EnableWindow(g_hSearchEdit, TRUE);
        InterlockedExchange(&g_SearchRunning, 0);
        return 0;
    }
    case WM_DESTROY: {
        if (g_hSearchEdit && g_OldEditProc) {
            SetWindowLongPtrW(g_hSearchEdit, GWLP_WNDPROC, (LONG_PTR)g_OldEditProc);
            g_OldEditProc = NULL;
        }
        g_hSearchWnd = NULL; g_hSearchEdit = NULL; g_hSearchList = NULL;
        g_hSearchBtn = NULL; g_hSearchProg = NULL;
        if (g_hList) LoadRepository();
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
// WndProc (Windows 8 Flat Styling)
// ============================================================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hWnd;
        const LangPack* L = &g_Lang[g_LangIdx];

        LOGFONTW lf = {0};
        HDC hdcScr = GetDC(NULL);
        lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdcScr, LOGPIXELSY), 72);
        ReleaseDC(NULL, hdcScr);
        lf.lfWeight = FW_NORMAL;
        wcscpy(lf.lfFaceName, L"Segoe UI");
        g_hFont = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_SEMIBOLD;
        g_hFontBold = CreateFontIndirectW(&lf);

        g_hBkgBrush   = CreateSolidBrush(WIN8_CLR_BG);
        g_hPanelBrush = CreateSolidBrush(WIN8_CLR_PANEL);
        g_hInputBrush = CreateSolidBrush(WIN8_CLR_BG);

        DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;

        g_hBtnInstall = CreateWindowW(L"BUTTON", L->btnInstall,
            btnStyle, 10, 10, 210, 32, hWnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
        g_hBtnSelAll = CreateWindowW(L"BUTTON", L->btnSelAll,
            btnStyle, 228, 10, 145, 32, hWnd, (HMENU)ID_BTN_SELALL, NULL, NULL);
        g_hBtnSelNone = CreateWindowW(L"BUTTON", L->btnSelNone,
            btnStyle, 381, 10, 145, 32, hWnd, (HMENU)ID_BTN_SELNONE, NULL, NULL);
        g_hBtnSearch = CreateWindowW(L"BUTTON", L->btnSearch,
            btnStyle, 534, 10, 160, 32, hWnd, (HMENU)ID_BTN_SEARCH, NULL, NULL);
        g_hBtnSettings = CreateWindowW(L"BUTTON", L->btnSettings,
            btnStyle, 702, 10, 160, 32, hWnd, (HMENU)ID_BTN_SETTINGS, NULL, NULL);
        g_hBtnExit = CreateWindowW(L"BUTTON", L->btnExit,
            btnStyle, 870, 10, 160, 32, hWnd, (HMENU)ID_BTN_EXIT, NULL, NULL);

        g_hList = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS,
            10, 52, 330, 400, hWnd, (HMENU)ID_LISTVIEW, NULL, NULL);

        ListView_SetExtendedListViewStyle(g_hList,
            LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(g_hList, WIN8_CLR_BG);
        ListView_SetTextBkColor(g_hList, WIN8_CLR_BG);
        ListView_SetTextColor(g_hList, WIN8_CLR_TEXT);

        LVCOLUMNW col0 = {0};
        col0.mask = LVCF_TEXT | LVCF_WIDTH;
        col0.pszText = L"Программа"; col0.cx = 300;
        SendMessageW(g_hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col0);

        LVCOLUMNW col1 = {0};
        col1.mask = LVCF_TEXT | LVCF_WIDTH;
        col1.pszText = L"ID"; col1.cx = 0;
        SendMessageW(g_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col1);

        SendMessageW(g_hList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hOutput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY,
            350, 52, 680, 400, hWnd, (HMENU)ID_EDIT_OUTPUT, NULL, NULL);
        SendMessageW(g_hOutput, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hLblTotal = CreateWindowW(L"STATIC", L"Установка: 0 / 0",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 490, 1020, 18, hWnd, (HMENU)ID_LABEL_TOTAL, NULL, NULL);
        SendMessageW(g_hLblTotal, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hProgTotal = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            10, 512, 1020, 20, hWnd, (HMENU)ID_PROGRESS_TOTAL, NULL, NULL);
        SendMessageW(g_hProgTotal, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgTotal, PBM_SETPOS, 0, 0);
        SendMessageW(g_hProgTotal, PBM_SETBARCOLOR, 0, WIN8_CLR_ACCENT);
        SendMessageW(g_hProgTotal, PBM_SETBKCOLOR, 0, WIN8_CLR_PANEL);

        g_hStatus = CreateWindowW(L"STATIC", L->statusReady,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 545, 1020, 20, hWnd, (HMENU)ID_STATUSBAR, NULL, NULL);
        SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        LoadRepository();
        CheckForUpdates();
        return 0;
    }

    case WM_COMMAND: {
        const LangPack* L = &g_Lang[g_LangIdx];
        switch (LOWORD(wParam)) {
        case ID_BTN_INSTALL:  OnInstallClicked();       break;
        case ID_BTN_SELALL:   SetAllCheckState(TRUE);   break;
        case ID_BTN_SELNONE:  SetAllCheckState(FALSE);  break;
        case ID_BTN_SETTINGS: {
            if (g_hSettingsWnd) { SetForegroundWindow(g_hSettingsWnd); break; }
            RECT rc;
            GetWindowRect(hWnd, &rc);
            int w = 290;
            int h = g_DebugMode ? 410 : 370;
            int x = rc.left + ((rc.right - rc.left) - w) / 2;
            int y = rc.top + ((rc.bottom - rc.top) - h) / 2;

            g_hSettingsWnd = CreateWindowExW(
                WS_EX_DLGMODALFRAME, L"WingetSettingsWnd", L->setTitle,
                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                x, y, w, h,
                hWnd, NULL, GetModuleHandleW(NULL), NULL);
            break;
        }
        case ID_BTN_SEARCH: {
            if (g_hSearchWnd) { SetForegroundWindow(g_hSearchWnd); break; }
            g_hSearchWnd = CreateWindowExW(
                0, L"WingetSearchWnd",
                L->searchWndTitle,
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT, 530, 420,
                hWnd, NULL, GetModuleHandleW(NULL), NULL);
            break;
        }
        case ID_BTN_EXIT:
            if (g_BatFinished) { DestroyWindow(hWnd); break; }
            if (g_hProcess && WaitForSingleObject(g_hProcess, 0) == WAIT_OBJECT_0) {
                DestroyWindow(hWnd); break;
            }
            if (InterlockedCompareExchange(&g_Running, 1, 0) != 0) {
                if (MessageBoxW(hWnd, L->msgAbortExit,
                        L->msgConfirmTitle, MB_YESNO | MB_ICONQUESTION) == IDNO) {
                    InterlockedExchange(&g_Running, 0);
                    break;
                }
            }
            DestroyWindow(hWnd);
            break;
        }
        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            BOOL primary = (dis->CtlID == ID_BTN_INSTALL);
            DrawWin8Button(dis, primary);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        if (hCtl == g_hStatus && g_StatusSuccess) SetTextColor(hdc, WIN8_CLR_SUCCESS);
        else                                       SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hBkgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, WIN8_CLR_BG);
        SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hInputBrush;
    }

    case WM_ERASEBKGND: {
        DrawFlatBackground(hWnd, (HDC)wParam, WIN8_CLR_BG);
        return 1;
    }

    case WM_SIZE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        int topPanelH = 52, bottomPanelH = 110, listW = 330;
        int contentH = h - topPanelH - bottomPanelH;
        if (g_hList)   MoveWindow(g_hList,   10,         52, listW,           contentH, TRUE);
        if (g_hOutput) MoveWindow(g_hOutput, listW + 20, 52, w - listW - 30,  contentH, TRUE);
        int y = h - bottomPanelH;
        if (g_hLblTotal)  MoveWindow(g_hLblTotal,  10, y,      w - 20, 18, TRUE);
        if (g_hProgTotal) MoveWindow(g_hProgTotal, 10, y + 22, w - 20, 20, TRUE);
        if (g_hStatus)    MoveWindow(g_hStatus,    10, y + 54, w - 20, 20, TRUE);
        return 0;
    }

    case WM_APP_UPDATE_DONE: {
        const LangPack* L = &g_Lang[g_LangIdx];
        InterlockedExchange(&g_UpdateRunning, 0);
        switch (wParam) {
        case 1: SetStatus(L->statusUpdErr); break;
        case 2: SetStatus(L->statusUpdLatest); break;
        case 3: SetStatus(L->statusUpdCancel); break;
        case 4: SetStatus(L->statusUpdLaunchErr); break;
        }
        return 0;
    }

    case WM_DESTROY: {
        DebugLog(L"APP", L"Launcher shutting down.");
        if (g_hFont)       DeleteObject(g_hFont);
        if (g_hFontBold)   DeleteObject(g_hFontBold);
        if (g_hBkgBrush)   DeleteObject(g_hBkgBrush);
        if (g_hPanelBrush) DeleteObject(g_hPanelBrush);
        if (g_hInputBrush) DeleteObject(g_hInputBrush);
        if (g_DebugCsInit) DeleteCriticalSection(&g_DebugCs);
        PostQuitMessage(0);
        ExitProcess(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
// Parse command line arguments
// ============================================================
static void ParseCommandLineFlags(void) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;

    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-debug") == 0 ||
            _wcsicmp(argv[i], L"--debug") == 0 ||
            _wcsicmp(argv[i], L"/debug") == 0) {
            g_DebugMode = TRUE;
        }
    }
    LocalFree(argv);
}

// ============================================================
// WinMain
// ============================================================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;

    ParseCommandLineFlags();
    if (g_DebugMode) {
        DebugLogInit();
        DebugLog(L"APP", L"=== WinGET Launcher started with -debug flag ===");
        DebugLog(L"APP", L"Command line: %ls", GetCommandLineW());
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    RTL_OSVERSIONINFOW osvi = {0};
    if (!IsWindowsSupported(&osvi)) {
        const LangPack* L = &g_Lang[g_LangIdx];
        wchar_t msg[512];
        _snwprintf(msg, 512, L->msgUnsupportedFmt,
            WIN10_MIN_BUILD,
            osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

        MessageBoxW(NULL, msg, L->msgUnsupportedTitle, MB_OK | MB_ICONERROR);
        return 1;
    }

    DetectOS();

    // ---- Проверка winget ----
    if (!IsWingetInstalled()) {
        const LangPack* L = &g_Lang[g_LangIdx];
        int msgboxID = MessageBoxW(NULL, L->msgNoWingetText,
            L->msgNoWingetTitle, MB_YESNO | MB_ICONWARNING);

        if (msgboxID == IDYES) {
            wchar_t dir[MAX_PATH]; GetExeDir(dir, MAX_PATH);
            wchar_t batPath[MAX_PATH]; _snwprintf(batPath, MAX_PATH, L"%ssource\\install_winget.bat", dir);
            DebugLog(L"WINGET_INSTALL", L"Launching %ls", batPath);

            if (GetFileAttributesW(batPath) != INVALID_FILE_ATTRIBUTES) {
                SHELLEXECUTEINFOW sei = {0};
                sei.cbSize = sizeof(sei);
                sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
                sei.lpFile = batPath; sei.lpDirectory = dir; sei.nShow = SW_SHOWNORMAL;
                ShellExecuteExW(&sei);
                if (sei.hProcess) {
                    WaitForSingleObject(sei.hProcess, INFINITE);
                    CloseHandle(sei.hProcess);
                }

                wchar_t exePath[MAX_PATH]; GetModuleFileNameW(NULL, exePath, MAX_PATH);
                ShellExecuteW(NULL, L"open", exePath, g_DebugMode ? L"-debug" : NULL, dir, SW_SHOWNORMAL);
                return 0;
            } else {
                DebugLog(L"WINGET_INSTALL", L"install_winget.bat not found!");
                MessageBoxW(NULL, L"install_winget.bat not found.", L->msgErrTitle, MB_ICONERROR);
                return 1;
            }
        } else {
            return 1;
        }
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"WingetInstallerLauncher";
    wc.hIcon   = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    wc.hIconSm = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    if (!wc.hIcon)   wc.hIcon   = LoadIcon(NULL, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"RegisterClassEx failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    WNDCLASSEXW wcSearch = {0};
    wcSearch.cbSize = sizeof(wcSearch);
    wcSearch.style = CS_HREDRAW | CS_VREDRAW;
    wcSearch.lpfnWndProc = SearchWndProc;
    wcSearch.hInstance = hInst;
    wcSearch.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSearch.hbrBackground = NULL;
    wcSearch.lpszClassName = L"WingetSearchWnd";
    wcSearch.hIcon = wc.hIcon; wcSearch.hIconSm = wc.hIconSm;
    RegisterClassExW(&wcSearch);

    WNDCLASSEXW wcSettings = {0};
    wcSettings.cbSize = sizeof(wcSettings);
    wcSettings.style = CS_HREDRAW | CS_VREDRAW;
    wcSettings.lpfnWndProc = SettingsWndProc;
    wcSettings.hInstance = hInst;
    wcSettings.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcSettings.hbrBackground = NULL;
    wcSettings.lpszClassName = L"WingetSettingsWnd";
    wcSettings.hIcon = wc.hIcon; wcSettings.hIconSm = wc.hIconSm;
    RegisterClassExW(&wcSettings);

    wchar_t title[256];
    if (g_DebugMode) {
        _snwprintf(title, 256, L"WinGET Launcher [%ls] [DEBUG]", g_OSName);
    } else {
        _snwprintf(title, 256, L"WinGET Launcher [%ls]", g_OSName);
    }

    HWND hWnd = CreateWindowExW(
        0, wc.lpszClassName, title,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1050, 620,
        NULL, NULL, hInst, NULL);

    if (!hWnd) {
        MessageBoxW(NULL, L"CreateWindowEx failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}