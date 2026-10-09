#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#pragma comment(lib, "wininet.lib")

// ============================================================
// GitHub update source
// ============================================================
#define GH_RAW_BASE       L"https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
#define GH_VERSION        GH_RAW_BASE L"version.txt"
#define GH_INSTALL        GH_RAW_BASE L"install_apps.bat"
#define GH_REPO_TXT       GH_RAW_BASE L"source/repositories.txt"
#define GH_INSTALL_WINGET GH_RAW_BASE L"source/install_winget.bat"
#define GH_SEARCH_BAT     GH_RAW_BASE L"source/search_winget.bat"
#define GH_SEARCH_PS1     GH_RAW_BASE L"source/search_winget.ps1"

// ============================================================
// IDs
// ============================================================
#define ID_BTN_INSTALL    1001
#define ID_BTN_LOGS       1002
#define ID_BTN_EXIT       1003
#define ID_EDIT_OUTPUT    1004
#define ID_STATUSBAR      1005
#define ID_LISTVIEW       1006
#define ID_BTN_SELALL     1007
#define ID_BTN_SELNONE    1008
#define ID_PROGRESS_TOTAL 1009
#define ID_LABEL_TOTAL    1010
#define ID_BTN_SEARCH     1011
#define ID_BTN_OPENREPO   1012
#define ID_BTN_UPDATE     1013

#define ID_SEARCH_EDIT    2001
#define ID_SEARCH_BTN     2002
#define ID_SEARCH_LIST    2003
#define ID_SEARCH_ADD     2004
#define ID_SEARCH_CLOSE   2005
#define ID_SEARCH_PROGRESS 2006

#define WM_APP_SEARCH_DONE (WM_APP + 1)
#define WM_APP_UPDATE_DONE (WM_APP + 2)

// ============================================================
// Package manager
// ============================================================
typedef enum {
    PKG_MGR_NONE   = 0,
    PKG_MGR_WINGET = 1,
    PKG_MGR_CHOCO  = 2
} PkgMgrType;

// ============================================================
// Globals
// ============================================================
static HWND g_hWnd        = NULL;
static HWND g_hOutput     = NULL;
static HWND g_hStatus     = NULL;
static HWND g_hBtnInstall = NULL;
static HWND g_hBtnLogs    = NULL;
static HWND g_hBtnExit    = NULL;
static HWND g_hBtnSelAll  = NULL;
static HWND g_hBtnSelNone = NULL;
static HWND g_hBtnSearch  = NULL;
static HWND g_hBtnOpenRepo= NULL;
static HWND g_hBtnUpdate  = NULL;
static HWND g_hList       = NULL;
static HWND g_hProgTotal  = NULL;
static HWND g_hLblTotal   = NULL;

static HANDLE g_hProcess  = NULL;
static HANDLE g_hThread   = NULL;
static volatile LONG g_Running = 0;

static int g_TotalPackages = 0;
static int g_DonePackages  = 0;

static BOOL g_WaitingForResult = FALSE;
static BOOL g_BatFinished      = FALSE;
static int  g_BatFinalRc       = -1;
static BOOL g_StatusSuccess    = FALSE;

static HFONT  g_hFont     = NULL;
static HFONT  g_hFontBold = NULL;
static HBRUSH g_hBkgBrush = NULL;

static PkgMgrType g_PkgMgr = PKG_MGR_NONE;
static wchar_t    g_OSName[128] = L"";

static HWND g_hSearchWnd    = NULL;
static HWND g_hSearchEdit   = NULL;
static HWND g_hSearchList   = NULL;
static HWND g_hSearchBtn    = NULL;
static HWND g_hSearchProg   = NULL;
static WNDPROC g_OldEditProc = NULL;
static volatile LONG g_SearchRunning = 0;

static volatile LONG g_UpdateRunning = 0;

// ============================================================
// Utility
// ============================================================
static void AppendOutput(const wchar_t* text) {
    if (!g_hOutput || !text) return;
    int len = GetWindowTextLengthW(g_hOutput);
    SendMessageW(g_hOutput, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_hOutput, EM_REPLACESEL, FALSE, (LPARAM)text);
}

static void SetStatus(const wchar_t* text) {
    if (g_hStatus) SetWindowTextW(g_hStatus, text);
}

static void EnableButtons(BOOL enable) {
    EnableWindow(g_hBtnInstall, enable);
    EnableWindow(g_hBtnLogs, enable);
    EnableWindow(g_hBtnSelAll, enable);
    EnableWindow(g_hBtnSelNone, enable);
    EnableWindow(g_hBtnSearch, enable);
    EnableWindow(g_hBtnOpenRepo, enable);
    EnableWindow(g_hList, enable);
}

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

static BOOL GetSearchBatPathOut(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%ssource\\search_winget.bat", dir);
    return TRUE;
}

static BOOL GetInstallWingetPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%ssource\\install_winget.bat", dir);
    return TRUE;
}

static BOOL GetVersionPath(wchar_t* out, size_t cch) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return FALSE;
    _snwprintf(out, cch, L"%sversion.txt", dir);
    return TRUE;
}

// ============================================================
// Detect OS
// ============================================================
static void DetectOS(void) {
    OSVERSIONINFOW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionExW(&osvi)) {
        _snwprintf(g_OSName, 128, L"Windows %lu.%lu (build %lu)",
                   osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    } else {
        wcscpy(g_OSName, L"Windows (unknown)");
    }

    if (osvi.dwMajorVersion >= 10 && osvi.dwBuildNumber >= 17763) {
        g_PkgMgr = PKG_MGR_WINGET;
    } else {
        g_PkgMgr = PKG_MGR_CHOCO;
    }
}

// ============================================================
// WinINet
// ============================================================
static BOOL DownloadFileW(const wchar_t* url, const wchar_t* destPath) {
    HINTERNET hNet = NULL, hUrl = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    hNet = InternetOpenW(L"WinGETLauncher/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return FALSE;

    hUrl = InternetOpenUrlW(hNet, url, NULL, 0,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return FALSE; }

    hFile = CreateFileW(destPath, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(hUrl); InternetCloseHandle(hNet); return FALSE;
    }

    char buf[8192];
    DWORD read = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read > 0) {
        DWORD written = 0;
        if (!WriteFile(hFile, buf, read, &written, NULL) || written != read) {
            CloseHandle(hFile); DeleteFileW(destPath);
            InternetCloseHandle(hUrl); InternetCloseHandle(hNet);
            return FALSE;
        }
    }
    result = TRUE;
    CloseHandle(hFile);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    return result;
}

static BOOL DownloadToStringW(const wchar_t* url, wchar_t* out, size_t outCch) {
    HINTERNET hNet = NULL, hUrl = NULL;
    hNet = InternetOpenW(L"WinGETLauncher/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return FALSE;
    hUrl = InternetOpenUrlW(hNet, url, NULL, 0,
                            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return FALSE; }

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

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    return ok;
}

static BOOL ReadLocalVersion(wchar_t* out, size_t cch) {
    wchar_t path[MAX_PATH];
    if (!GetVersionPath(path, MAX_PATH)) return FALSE;

    FILE* f = _wfopen(path, L"rb");
    if (!f) return FALSE;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0 || fsize > 4096) { fclose(f); return FALSE; }

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
    if (*p == '\0') return FALSE;

    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, -1, NULL, 0);
    if (wlen > 0) {
        MultiByteToWideChar(CP_UTF8, 0, p, -1, out, (int)cch);
    } else {
        MultiByteToWideChar(CP_ACP, 0, p, -1, out, (int)cch);
    }
    return TRUE;
}

// Write version.txt WITHOUT BOM
static BOOL WriteLocalVersion(const wchar_t* ver) {
    wchar_t path[MAX_PATH];
    if (!GetVersionPath(path, MAX_PATH)) return FALSE;

    FILE* f = _wfopen(path, L"wb");
    if (!f) return FALSE;

    char utf8[1024] = {0};
    int len = WideCharToMultiByte(CP_UTF8, 0, ver, -1, utf8, 1024, NULL, NULL);
    if (len > 1) {
        fwrite(utf8, 1, len - 1, f);
        fwrite("\n", 1, 1, f);
    }
    fclose(f);
    return TRUE;
}

// ============================================================
// Update
// ============================================================
static DWORD WINAPI UpdateThread(LPVOID param) {
    (void)param;

    wchar_t remoteVer[256] = {0};
    wchar_t localVer[256]  = {0};

    if (!ReadLocalVersion(localVer, 256)) wcscpy(localVer, L"(unknown)");
    if (!DownloadToStringW(GH_VERSION, remoteVer, 256)) {
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 1, 0);
        return 0;
    }

    if (wcscmp(remoteVer, localVer) == 0) {
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 2, 0);
        return 0;
    }

    wchar_t msg[512];
    _snwprintf(msg, 512,
        L"Доступна новая версия!\n\nТекущая: %ls\nНовая:   %ls\n\n"
        L"Скачать обновление файлов скриптов?",
        localVer, remoteVer);

    if (MessageBoxW(g_hWnd, msg, L"Обновление",
                    MB_YESNO | MB_ICONQUESTION) != IDYES) {
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 3, 0);
        return 0;
    }

    wchar_t dir[MAX_PATH];
    GetExeDir(dir, MAX_PATH);

    wchar_t tmpInstall[MAX_PATH], tmpRepo[MAX_PATH];
    wchar_t tmpWingetBat[MAX_PATH], tmpSearchBat[MAX_PATH], tmpSearchPs1[MAX_PATH];
    _snwprintf(tmpInstall,   MAX_PATH, L"%sinstall_apps.new.bat",           dir);
    _snwprintf(tmpRepo,      MAX_PATH, L"%ssource\\repositories.new.txt",   dir);
    _snwprintf(tmpWingetBat, MAX_PATH, L"%ssource\\install_winget.new.bat", dir);
    _snwprintf(tmpSearchBat, MAX_PATH, L"%ssource\\search_winget.new.bat",  dir);
    _snwprintf(tmpSearchPs1, MAX_PATH, L"%ssource\\search_winget.new.ps1",  dir);

    BOOL okInstall    = DownloadFileW(GH_INSTALL,        tmpInstall);
    BOOL okRepo       = DownloadFileW(GH_REPO_TXT,       tmpRepo);
    BOOL okWingetBat  = DownloadFileW(GH_INSTALL_WINGET, tmpWingetBat);
    BOOL okSearchBat  = DownloadFileW(GH_SEARCH_BAT,     tmpSearchBat);
    BOOL okSearchPs1  = DownloadFileW(GH_SEARCH_PS1,     tmpSearchPs1);

    if (!okInstall && !okRepo && !okWingetBat && !okSearchBat && !okSearchPs1) {
        PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 4, 0);
        return 0;
    }

    wchar_t realInstall[MAX_PATH], realRepo[MAX_PATH];
    wchar_t realWingetBat[MAX_PATH], realSearchBat[MAX_PATH], realSearchPs1[MAX_PATH];
    GetBatPath(realInstall, MAX_PATH);
    GetRepoPath(realRepo, MAX_PATH);
    GetInstallWingetPath(realWingetBat, MAX_PATH);
    GetSearchBatPathOut(realSearchBat, MAX_PATH);
    GetSearchPs1Path(realSearchPs1, MAX_PATH);

    #define REPLACE_FILE(ok, tmp, real) \
        if (ok) { DeleteFileW(real); MoveFileW(tmp, real); }

    REPLACE_FILE(okInstall,   tmpInstall,   realInstall);
    REPLACE_FILE(okRepo,      tmpRepo,      realRepo);
    REPLACE_FILE(okWingetBat, tmpWingetBat, realWingetBat);
    REPLACE_FILE(okSearchBat, tmpSearchBat, realSearchBat);
    REPLACE_FILE(okSearchPs1, tmpSearchPs1, realSearchPs1);

    WriteLocalVersion(remoteVer);

    PostMessageW(g_hWnd, WM_APP_UPDATE_DONE, 0, 0);
    return 0;
}

static void CheckForUpdates(void) {
    if (InterlockedCompareExchange(&g_UpdateRunning, 1, 0) != 0) return;
    HANDLE h = CreateThread(NULL, 0, UpdateThread, NULL, 0, NULL);
    if (h) CloseHandle(h);
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
        _snwprintf(buf, 256, L"Установка: %d / %d", current, total);
        SetWindowTextW(g_hLblTotal, buf);
    }
}

static void ParseWingetLine(const wchar_t* line) {
    if (wcsstr(line, L"[BAT_DONE]")) {
        const wchar_t* p = wcsstr(line, L"rc=");
        if (p) g_BatFinalRc = _wtoi(p + 3);
        g_BatFinished = TRUE;
        InterlockedExchange(&g_Running, 0);
        return;
    }
    if (wcsstr(line, L"[INSTALL]")) { g_WaitingForResult = TRUE; return; }
    if (!g_WaitingForResult) return;
    if (wcsstr(line, L"[OK]") || wcsstr(line, L"[SKIP]") ||
        wcsstr(line, L"[WARN]") || wcsstr(line, L"[FAIL]")) {
        g_DonePackages++;
        SetTotalProgress(g_DonePackages, g_TotalPackages);
        g_WaitingForResult = FALSE;
    }
}

static void AppendOutputA(const char* text) {
    if (!text) return;
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
// Returns: 1 = package, 2 = category, 0 = EOF
// ============================================================
static int ReadRepoLine(FILE* f, wchar_t* out_id, size_t id_cch,
                                   wchar_t* out_choco, size_t choco_cch,
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
        char* part3 = NULL;
        char* bar1 = strchr(p, '|');
        if (bar1) {
            *bar1 = '\0';
            part2 = bar1 + 1;
            char* bar2 = strchr(part2, '|');
            if (bar2) { *bar2 = '\0'; part3 = bar2 + 1; }
        }

        #define TRIM(s) do { \
            while (*(s) == ' ' || *(s) == '\t') (s)++; \
            char* _e = (s) + strlen(s); \
            while (_e > (s) && (_e[-1] == ' ' || _e[-1] == '\t')) *(--_e) = '\0'; \
        } while(0)

        TRIM(part1);
        if (part2) TRIM(part2);
        if (part3) TRIM(part3);
        if (*part1 == '\0') continue;

        int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part1, -1, NULL, 0);
        if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part1, -1, out_id, (int)id_cch);
        else          MultiByteToWideChar(CP_ACP, 0, part1, -1, out_id, (int)id_cch);

        if (part3) {
            wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part2, -1, NULL, 0);
            if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part2, -1, out_choco, (int)choco_cch);
            else          MultiByteToWideChar(CP_ACP, 0, part2, -1, out_choco, (int)choco_cch);

            wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part3, -1, NULL, 0);
            if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part3, -1, out_name, (int)name_cch);
            else          MultiByteToWideChar(CP_ACP, 0, part3, -1, out_name, (int)name_cch);
        } else if (part2 && *part2 != '\0') {
            out_choco[0] = L'\0';
            wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, part2, -1, NULL, 0);
            if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, part2, -1, out_name, (int)name_cch);
            else          MultiByteToWideChar(CP_ACP, 0, part2, -1, out_name, (int)name_cch);
        } else {
            out_choco[0] = L'\0';
            wcsncpy(out_name, out_id, name_cch);
            out_name[name_cch - 1] = L'\0';
        }
        return 1;
    }
    return 0;
}

// ============================================================
// LoadRepository — категории как обычные строки
// ============================================================
static void LoadRepository(void) {
    wchar_t repoPath[MAX_PATH];
    if (!GetRepoPath(repoPath, MAX_PATH)) return;

    FILE* f = _wfopen(repoPath, L"rb");
    if (!f) {
        AppendOutput(L"[ERROR] Cannot open source\\repositories.txt\r\n");
        SetStatus(L"Файл репозитория не найден");
        return;
    }

    unsigned char bom[3];
    if (fread(bom, 1, 3, f) != 3 ||
        !(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
        rewind(f);
    }

    SendMessageW(g_hList, LVM_DELETEALLITEMS, 0, 0);

    wchar_t w_id[256], w_choco[256], w_name[256];
    int idx = 0;
    int skipped = 0;
    int rc;

    while ((rc = ReadRepoLine(f, w_id, 256, w_choco, 256, w_name, 256)) != 0) {
        if (rc == 2) {
            // ---- Категория как строка с префиксом ----
            wchar_t header[300];
            _snwprintf(header, 300, L"── %ls ──", w_name);

            LVITEMW item = {0};
            item.mask = LVIF_TEXT;
            item.iItem = idx;
            item.iSubItem = 0;
            item.pszText = header;
            int row = (int)SendMessageW(g_hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

            // Пустая колонка 1 — значит это не пакет
            LVITEMW sub = {0};
            sub.mask = LVIF_TEXT;
            sub.iItem = row;
            sub.iSubItem = 1;
            sub.pszText = L"";
            SendMessageW(g_hList, LVM_SETITEMW, 0, (LPARAM)&sub);

            // Снимаем галочку (чекбокс у категории не нужен)
            ListView_SetCheckState(g_hList, row, FALSE);
            idx++;
            continue;
        }

        // ---- Пакет ----
        const wchar_t* active_id = NULL;
        if (g_PkgMgr == PKG_MGR_WINGET) active_id = w_id;
        else if (g_PkgMgr == PKG_MGR_CHOCO) active_id = w_choco;

        if (!active_id || active_id[0] == L'\0') { skipped++; continue; }

        LVITEMW item = {0};
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = w_name;
        int row = (int)SendMessageW(g_hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

        LVITEMW sub = {0};
        sub.mask = LVIF_TEXT;
        sub.iItem = row;
        sub.iSubItem = 1;
        sub.pszText = (LPWSTR)active_id;
        SendMessageW(g_hList, LVM_SETITEMW, 0, (LPARAM)&sub);

        ListView_SetCheckState(g_hList, row, TRUE);
        idx++;
    }
    fclose(f);

    wchar_t status[256];
    const wchar_t* mgr = (g_PkgMgr == PKG_MGR_WINGET) ? L"winget" : L"choco";
    _snwprintf(status, 256,
               L"Загружено %d строк (менеджер: %ls, пропущено %d)",
               idx, mgr, skipped);
    SetStatus(status);
}

static int GetCheckedCount(void) {
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    int checked = 0;
    for (int i = 0; i < count; i++) {
        wchar_t idbuf[256] = {0};
        ListView_GetItemText(g_hList, i, 1, idbuf, 256);
        if (idbuf[0] == L'\0') continue;  // категория

        if (ListView_GetCheckState(g_hList, i)) checked++;
    }
    return checked;
}

static BOOL WriteTmpRepo(const wchar_t* path) {
    FILE* f = _wfopen(path, L"wb");
    if (!f) return FALSE;

    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        wchar_t buf[256] = {0};
        ListView_GetItemText(g_hList, i, 1, buf, 256);
        if (buf[0] == L'\0') continue;  // категория

        if (!ListView_GetCheckState(g_hList, i)) continue;

        char utf8[1024];
        int len = WideCharToMultiByte(CP_UTF8, 0, buf, -1, utf8, 1024, NULL, NULL);
        if (len > 1) {
            fwrite(utf8, 1, len - 1, f);
            fwrite("\n", 1, 1, f);
        }
    }
    fclose(f);
    return TRUE;
}

// ============================================================
// Run .bat
// ============================================================
static DWORD WINAPI RunBatThread(LPVOID param) {
    (void)param;

    wchar_t batPath[MAX_PATH], tmpRepo[MAX_PATH];
    if (!GetBatPath(batPath, MAX_PATH) || !GetTmpRepoPath(tmpRepo, MAX_PATH)) {
        AppendOutput(L"[ERROR] Cannot resolve paths.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }
    if (GetFileAttributesW(batPath) == INVALID_FILE_ATTRIBUTES) {
        AppendOutput(L"[ERROR] install_apps.bat not found next to launcher.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    DeleteFileW(tmpRepo);
    if (!WriteTmpRepo(tmpRepo)) {
        AppendOutput(L"[ERROR] Cannot write temporary repository file.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    SetEnvironmentVariableW(L"REPO_FILE", tmpRepo);
    SetEnvironmentVariableW(L"PKG_MGR",
        g_PkgMgr == PKG_MGR_WINGET ? L"winget" : L"choco");

    wchar_t cmdLine[MAX_PATH * 2];
    _snwprintf(cmdLine, MAX_PATH * 2, L"cmd.exe /c \"\"%s\"\"", batPath);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
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
        AppendOutput(L"[ERROR] CreateProcess failed.\r\n");
        CloseHandle(hRead); DeleteFileW(tmpRepo);
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE); return 1;
    }

    g_hProcess = pi.hProcess;

    char buf[4096]; DWORD read = 0;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        buf[read] = '\0';
        AppendOutputA(buf);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 0;
    GetExitCodeProcess(pi.hProcess, &rc);

    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(hRead);
    g_hProcess = NULL;
    DeleteFileW(tmpRepo);

    wchar_t msg[128];
    _snwprintf(msg, 128, L"\r\n=== Готово. Код выхода: %lu ===\r\n", rc);
    AppendOutput(msg);

    int finalRc = g_BatFinished ? g_BatFinalRc : (int)rc;
    if (finalRc == 0) { SetStatus(L"Successfully  ·  Установка завершена"); g_StatusSuccess = TRUE; }
    else              { SetStatus(L"Ошибка  ·  Смотрите лог"); g_StatusSuccess = FALSE; }

    InterlockedExchange(&g_Running, 0);
    EnableButtons(TRUE);
    InvalidateRect(g_hStatus, NULL, TRUE);
    return 0;
}

// ============================================================
// Handlers
// ============================================================
static void OnInstallClicked(void) {
    if (InterlockedCompareExchange(&g_Running, 1, 0) != 0) {
        MessageBoxW(g_hWnd, L"Установка уже выполняется.", L"Занято", MB_ICONINFORMATION);
        return;
    }
    int selected = GetCheckedCount();
    if (selected == 0) {
        MessageBoxW(g_hWnd, L"Не отмечено ни одного приложения.", L"Внимание", MB_ICONWARNING);
        InterlockedExchange(&g_Running, 0); return;
    }

    g_TotalPackages = selected; g_DonePackages = 0;
    g_WaitingForResult = FALSE; g_BatFinished = FALSE;
    g_BatFinalRc = -1; g_StatusSuccess = FALSE;
    SetTotalProgress(0, g_TotalPackages);

    SetWindowTextW(g_hOutput, L"");
    wchar_t msg[128];
    _snwprintf(msg, 128, L"=== Запуск install_apps.bat (выбрано: %d, менеджер: %ls) ===\r\n\r\n",
               selected, g_PkgMgr == PKG_MGR_WINGET ? L"winget" : L"choco");
    AppendOutput(msg);
    SetStatus(L"Установка...");
    EnableButtons(FALSE);

    g_hThread = CreateThread(NULL, 0, RunBatThread, NULL, 0, NULL);
    if (!g_hThread) {
        AppendOutput(L"[ERROR] CreateThread failed.\r\n");
        InterlockedExchange(&g_Running, 0); EnableButtons(TRUE);
    } else { CloseHandle(g_hThread); g_hThread = NULL; }
}

static void OnLogsClicked(void) {
    wchar_t dir[MAX_PATH];
    if (!GetExeDir(dir, MAX_PATH)) return;
    wchar_t logsDir[MAX_PATH];
    _snwprintf(logsDir, MAX_PATH, L"%slogs", dir);
    if (GetFileAttributesW(logsDir) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hWnd, L"Папка logs ещё не создана.", L"Логи", MB_ICONINFORMATION);
        return;
    }
    ShellExecuteW(g_hWnd, L"open", logsDir, NULL, NULL, SW_SHOWNORMAL);
}

static void OnOpenRepoClicked(void) {
    wchar_t repoPath[MAX_PATH];
    if (!GetRepoPath(repoPath, MAX_PATH)) return;
    if (GetFileAttributesW(repoPath) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hWnd, L"Файл repositories.txt не найден.", L"Ошибка", MB_ICONERROR);
        return;
    }
    ShellExecuteW(g_hWnd, L"open", L"notepad.exe", repoPath, NULL, SW_SHOWNORMAL);
}

static void SetAllCheckState(BOOL state) {
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        wchar_t idbuf[256] = {0};
        ListView_GetItemText(g_hList, i, 1, idbuf, 256);
        if (idbuf[0] == L'\0') continue;  // категория
        ListView_SetCheckState(g_hList, i, state);
    }
}

// ============================================================
// Search thread
// ============================================================
static DWORD WINAPI SearchThread(LPVOID param) {
    (void)param;
    if (!g_hSearchList) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }
    PostMessageW(g_hSearchList, LVM_DELETEALLITEMS, 0, 0);

    wchar_t query[256] = {0};
    GetWindowTextW(g_hSearchEdit, query, 256);
    if (!*query) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }

    wchar_t ps1Path[MAX_PATH];
    if (!GetSearchPs1Path(ps1Path, MAX_PATH)) { PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }
    if (GetFileAttributesW(ps1Path) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_hSearchWnd, L"Файл search_winget.ps1 не найден в source\\",
                    L"Ошибка", MB_ICONERROR);
        PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0);
        return 0;
    }

    wchar_t cmdLine[MAX_PATH * 4];
    _snwprintf(cmdLine, MAX_PATH * 4,
               L"powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"%s\" -Query \"%s\"",
               ps1Path, query);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
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
    if (!ok) { CloseHandle(hRead); PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, 0, 0); return 0; }

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
                    }
                }
                linePos = 0;
            } else {
                lineBuf[linePos++] = c;
            }
        }
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(hRead);
    PostMessageW(g_hSearchWnd, WM_APP_SEARCH_DONE, (WPARAM)idx, 0);
    return 0;
}

static void AddSelectedToRepo(void) {
    if (!g_hSearchList) return;
    int sel = (int)SendMessageW(g_hSearchList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(g_hSearchWnd, L"Выберите программу в списке.", L"Внимание", MB_ICONWARNING);
        return;
    }
    wchar_t id[256] = {0}, name[256] = {0};
    ListView_GetItemText(g_hSearchList, sel, 1, id, 256);
    ListView_GetItemText(g_hSearchList, sel, 0, name, 256);

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
                MessageBoxW(g_hSearchWnd, L"Этот пакет уже есть в repositories.txt",
                            L"Информация", MB_ICONINFORMATION);
                return;
            }
        }
        fclose(fCheck);
    }

    FILE* f = _wfopen(repoPath, L"a, ccs=UTF-8");
    if (!f) {
        MessageBoxW(g_hSearchWnd, L"Не удалось открыть repositories.txt", L"Ошибка", MB_ICONERROR);
        return;
    }
    fwprintf(f, L"%ls||%ls\n", id, name);
    fclose(f);

    wchar_t msg[512];
    _snwprintf(msg, 512, L"Добавлено:\n%ls|%ls", id, name);
    MessageBoxW(g_hSearchWnd, msg, L"Успех", MB_ICONINFORMATION);
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
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        g_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 10, 380, 26, hWnd, (HMENU)ID_SEARCH_EDIT, NULL, NULL);
        SendMessageW(g_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        g_OldEditProc = (WNDPROC)SetWindowLongPtrW(g_hSearchEdit, GWLP_WNDPROC, (LONG_PTR)SearchEditProc);

        g_hSearchBtn = CreateWindowW(L"BUTTON", L"Найти",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            400, 10, 100, 26, hWnd, (HMENU)ID_SEARCH_BTN, NULL, NULL);
        SendMessageW(g_hSearchBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hSearchList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            10, 46, 490, 260, hWnd, (HMENU)ID_SEARCH_LIST, NULL, NULL);
        ListView_SetExtendedListViewStyle(g_hSearchList,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNW col0 = {0};
        col0.mask = LVCF_TEXT | LVCF_WIDTH;
        col0.pszText = L"Программа"; col0.cx = 250;
        SendMessageW(g_hSearchList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col0);

        LVCOLUMNW col1 = {0};
        col1.mask = LVCF_TEXT | LVCF_WIDTH;
        col1.pszText = L"ID"; col1.cx = 230;
        SendMessageW(g_hSearchList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col1);
        SendMessageW(g_hSearchList, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_hSearchProg = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | PBS_MARQUEE,
            10, 312, 490, 14, hWnd, (HMENU)ID_SEARCH_PROGRESS, NULL, NULL);
        SendMessageW(g_hSearchProg, PBM_SETMARQUEE, TRUE, 30);

        CreateWindowW(L"BUTTON", L"Добавить в репозиторий",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 335, 200, 30, hWnd, (HMENU)ID_SEARCH_ADD, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Закрыть",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
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
// Owner-draw button
// ============================================================
static void DrawAeroButton(LPDRAWITEMSTRUCT dis) {
    HDC hdc = dis->hDC; RECT rc = dis->rcItem;
    BOOL pressed  = (dis->itemState & ODS_SELECTED) != 0;
    BOOL disabled = (dis->itemState & ODS_DISABLED) != 0;
    BOOL focused  = (dis->itemState & ODS_FOCUS) != 0;
    BOOL hot      = (dis->itemState & ODS_HOTLIGHT) != 0;

    FillRect(hdc, &rc, g_hBkgBrush);

    COLORREF top, bottom, border;
    if (disabled) { top=RGB(240,240,240); bottom=RGB(220,220,220); border=RGB(180,180,180); }
    else if (pressed) { top=RGB(180,210,240); bottom=RGB(140,180,220); border=RGB(80,120,170); }
    else if (hot) { top=RGB(250,252,255); bottom=RGB(210,230,250); border=RGB(90,140,200); }
    else { top=RGB(250,250,250); bottom=RGB(225,230,240); border=RGB(140,150,170); }

    int radius = 4;
    HRGN hRgn = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius*2, radius*2);
    SelectClipRgn(hdc, hRgn);

    TRIVERTEX vert[2];
    vert[0].x = rc.left; vert[0].y = rc.top;
    vert[0].Red=(COLOR16)(GetRValue(top)<<8); vert[0].Green=(COLOR16)(GetGValue(top)<<8); vert[0].Blue=(COLOR16)(GetBValue(top)<<8); vert[0].Alpha=0;
    vert[1].x = rc.right; vert[1].y = rc.bottom;
    vert[1].Red=(COLOR16)(GetRValue(bottom)<<8); vert[1].Green=(COLOR16)(GetGValue(bottom)<<8); vert[1].Blue=(COLOR16)(GetBValue(bottom)<<8); vert[1].Alpha=0;
    GRADIENT_RECT gr = {0, 1};
    GradientFill(hdc, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    HPEN hPenLight = CreatePen(PS_SOLID, 1, RGB(255,255,255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenLight);
    MoveToEx(hdc, rc.left + 2, rc.top + 1, NULL);
    LineTo(hdc, rc.right - 2, rc.top + 1);

    HPEN hPenBorder = CreatePen(PS_SOLID, 1, border);
    SelectObject(hdc, hPenBorder);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius*2, radius*2);

    SelectObject(hdc, hOldPen); SelectObject(hdc, hOldBrush);
    DeleteObject(hPenLight); DeleteObject(hPenBorder);
    SelectClipRgn(hdc, NULL); DeleteObject(hRgn);

    wchar_t text[128];
    GetWindowTextW(dis->hwndItem, text, 128);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, disabled ? RGB(140,140,140) : RGB(30,30,30));
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);

    if (focused && !disabled) { RECT rf = rc; InflateRect(&rf, -3, -3); DrawFocusRect(hdc, &rf); }
}

static void DrawWindowBackground(HWND hWnd, HDC hdc) {
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    TRIVERTEX vert[2];
    vert[0].x=0; vert[0].y=0; vert[0].Red=0xE0<<8; vert[0].Green=0xEE<<8; vert[0].Blue=0xFF<<8; vert[0].Alpha=0;
    vert[1].x=w; vert[1].y=h; vert[1].Red=0xFF<<8; vert[1].Green=0xFF<<8; vert[1].Blue=0xFF<<8; vert[1].Alpha=0;
    GRADIENT_RECT gr = {0, 1};
    GradientFill(hdc, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);
}

// ============================================================
// WndProc
// ============================================================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hWnd;

        LOGFONTW lf = {0};
        lf.lfHeight = -MulDiv(9, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
        lf.lfWeight = FW_NORMAL;
        wcscpy(lf.lfFaceName, L"Segoe UI");
        g_hFont = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_BOLD;
        g_hFontBold = CreateFontIndirectW(&lf);
        g_hBkgBrush = CreateSolidBrush(RGB(0xEC, 0xF3, 0xFA));

        DWORD btnStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;

        g_hBtnInstall = CreateWindowW(L"BUTTON", L"Установить выбранное",
            btnStyle, 10, 10, 170, 32, hWnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
        g_hBtnSelAll = CreateWindowW(L"BUTTON", L"Выбрать все",
            btnStyle, 185, 10, 105, 32, hWnd, (HMENU)ID_BTN_SELALL, NULL, NULL);
        g_hBtnSelNone = CreateWindowW(L"BUTTON", L"Снять все",
            btnStyle, 295, 10, 95, 32, hWnd, (HMENU)ID_BTN_SELNONE, NULL, NULL);
        g_hBtnSearch = CreateWindowW(L"BUTTON", L"Поиск winget",
            btnStyle, 395, 10, 110, 32, hWnd, (HMENU)ID_BTN_SEARCH, NULL, NULL);
        g_hBtnOpenRepo = CreateWindowW(L"BUTTON", L"Открыть репозиторий",
            btnStyle, 510, 10, 140, 32, hWnd, (HMENU)ID_BTN_OPENREPO, NULL, NULL);
        g_hBtnUpdate = CreateWindowW(L"BUTTON", L"Проверить обновления",
            btnStyle, 655, 10, 150, 32, hWnd, (HMENU)ID_BTN_UPDATE, NULL, NULL);
        g_hBtnLogs = CreateWindowW(L"BUTTON", L"Открыть логи",
            btnStyle, 810, 10, 110, 32, hWnd, (HMENU)ID_BTN_LOGS, NULL, NULL);
        g_hBtnExit = CreateWindowW(L"BUTTON", L"Выход",
            btnStyle, 925, 10, 90, 32, hWnd, (HMENU)ID_BTN_EXIT, NULL, NULL);

        g_hList = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS,
            10, 52, 330, 400, hWnd, (HMENU)ID_LISTVIEW, NULL, NULL);

        ListView_SetExtendedListViewStyle(g_hList,
            LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

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
        SendMessageW(g_hLblTotal, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hProgTotal = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            10, 512, 1020, 20, hWnd, (HMENU)ID_PROGRESS_TOTAL, NULL, NULL);
        SendMessageW(g_hProgTotal, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgTotal, PBM_SETPOS, 0, 0);

        g_hStatus = CreateWindowW(L"STATIC", L"Готов",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 545, 1020, 20, hWnd, (HMENU)ID_STATUSBAR, NULL, NULL);
        SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        LoadRepository();
        CheckForUpdates();
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_INSTALL:  OnInstallClicked();       break;
        case ID_BTN_SELALL:   SetAllCheckState(TRUE);   break;
        case ID_BTN_SELNONE:  SetAllCheckState(FALSE);  break;
        case ID_BTN_LOGS:     OnLogsClicked();          break;
        case ID_BTN_OPENREPO: OnOpenRepoClicked();      break;
        case ID_BTN_UPDATE:   CheckForUpdates();        break;
        case ID_BTN_SEARCH: {
            if (g_hSearchWnd) { SetForegroundWindow(g_hSearchWnd); break; }
            g_hSearchWnd = CreateWindowExW(
                0, L"WingetSearchWnd",
                L"Поиск приложений в winget",
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
                if (MessageBoxW(hWnd,
                        L"Установка ещё идёт. Прервать и выйти?",
                        L"Подтверждение", MB_YESNO | MB_ICONQUESTION) == IDNO) {
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
        if (dis->CtlType == ODT_BUTTON) { DrawAeroButton(dis); return TRUE; }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        if (hCtl == g_hStatus && g_StatusSuccess) SetTextColor(hdc, RGB(0, 128, 0));
        else                                       SetTextColor(hdc, RGB(30, 30, 30));
        return (LRESULT)g_hBkgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(30, 30, 30));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        DrawWindowBackground(hWnd, hdc);
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
        InterlockedExchange(&g_UpdateRunning, 0);
        switch (wParam) {
        case 0:
            SetStatus(L"Обновление установлено");
            MessageBoxW(g_hWnd,
                L"Файлы обновлены!\nПерезапустите лаунчер, чтобы применить изменения.",
                L"Обновление", MB_ICONINFORMATION);
            break;
        case 1: SetStatus(L"Не удалось проверить обновления"); break;
        case 2: SetStatus(L"У вас последняя версия"); break;
        case 3: SetStatus(L"Обновление отменено"); break;
        case 4:
            SetStatus(L"Ошибка загрузки");
            MessageBoxW(g_hWnd, L"Не удалось скачать обновление.", L"Ошибка", MB_ICONERROR);
            break;
        }
        return 0;
    }

    case WM_DESTROY: {
        if (g_hFont)     DeleteObject(g_hFont);
        if (g_hFontBold) DeleteObject(g_hFontBold);
        if (g_hBkgBrush) DeleteObject(g_hBkgBrush);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ============================================================
// WinMain
// ============================================================
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    DetectOS();

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
    wcSearch.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcSearch.lpszClassName = L"WingetSearchWnd";
    wcSearch.hIcon = wc.hIcon; wcSearch.hIconSm = wc.hIconSm;
    RegisterClassExW(&wcSearch);

    wchar_t title[256];
    _snwprintf(title, 256, L"WinGET Launcher — %ls [%ls]",
               g_PkgMgr == PKG_MGR_WINGET ? L"winget" : L"Chocolatey",
               g_OSName);

    HWND hWnd = CreateWindowExW(
        0, wc.lpszClassName, title,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 660,
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