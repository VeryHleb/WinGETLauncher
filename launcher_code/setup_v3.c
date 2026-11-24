#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdarg.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uuid.lib")

static const CLSID WGL_CLSID_ShellLink    = { 0x00021401, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const IID   WGL_IID_IShellLinkW    = { 0x000214F9, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const IID   WGL_IID_IPersistFile   = { 0x0000010b, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

#define GH_API_LATEST    L"https://api.github.com/repos/VeryHleb/WinGETLauncher/releases/latest"
#define GH_RAW_BASE      L"https://raw.githubusercontent.com/VeryHleb/WinGETLauncher/main/"
#define GH_FILES_LIST    GH_RAW_BASE L"files.txt"
#define GH_RELEASE_DL    L"https://github.com/VeryHleb/WinGETLauncher/releases/latest/download/"

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

#define ID_LBL_TITLE     4001
#define ID_LBL_DIR       4002
#define ID_EDIT_DIR      4003
#define ID_BTN_BROWSE    4004
#define ID_CHK_DESKTOP   4005
#define ID_CHK_STARTMENU 4006
#define ID_BTN_ACTION    4007
#define ID_BTN_CANCEL    4008
#define ID_PROGRESS      4009
#define ID_EDIT_LOG      4010
#define ID_LBL_STATUS    4011

#define WM_APP_SETUP_DONE (WM_APP + 10)
#define WM_APP_LOG_LINE   (WM_APP + 11)
#define WM_APP_PROGRESS   (WM_APP + 12)

static HWND g_hWnd          = NULL;
static HWND g_hEditDir      = NULL;
static HWND g_hBtnBrowse    = NULL;
static HWND g_hChkDesktop   = NULL;
static HWND g_hChkStartMenu = NULL;
static HWND g_hBtnAction    = NULL;
static HWND g_hBtnCancel    = NULL;
static HWND g_hProgress     = NULL;
static HWND g_hEditLog      = NULL;
static HWND g_hLblStatus    = NULL;

static HFONT  g_hFont      = NULL;
static HFONT  g_hFontBold  = NULL;
static HFONT  g_hFontTitle = NULL;
static HBRUSH g_hBgBrush   = NULL;

static BOOL g_UpdateMode   = FALSE;
static BOOL g_DebugMode    = FALSE;
static volatile LONG g_Busy = 0;

static wchar_t g_StartDir[MAX_PATH] = {0};

static void AppendLog(const wchar_t* fmt, ...) {
    wchar_t msg[2048];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(msg, 2048, fmt, ap);
    msg[2047] = L'\0';
    va_end(ap);

    size_t len = wcslen(msg);
    wchar_t* line = (wchar_t*)malloc((len + 4) * sizeof(wchar_t));
    if (!line) return;
    wcscpy(line, msg);
    wcscat(line, L"\r\n");

    if (g_hWnd && GetCurrentThreadId() != GetWindowThreadProcessId(g_hWnd, NULL)) {
        PostMessageW(g_hWnd, WM_APP_LOG_LINE, 0, (LPARAM)line);
    } else if (g_hEditLog) {
        int curLen = GetWindowTextLengthW(g_hEditLog);
        SendMessageW(g_hEditLog, EM_SETSEL, (WPARAM)curLen, (LPARAM)curLen);
        SendMessageW(g_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)line);
        free(line);
    } else {
        free(line);
    }
}

static void SetStatusText(const wchar_t* text) {
    if (g_hLblStatus) SetWindowTextW(g_hLblStatus, text);
}

static void SetProgressValue(int cur, int total) {
    if (!g_hProgress) return;
    if (total <= 0) total = 100;
    if (GetCurrentThreadId() != GetWindowThreadProcessId(g_hWnd, NULL)) {
        PostMessageW(g_hWnd, WM_APP_PROGRESS, (WPARAM)cur, (LPARAM)total);
    } else {
        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, total));
        SendMessageW(g_hProgress, PBM_SETPOS, (WPARAM)cur, 0);
    }
}

static void GetDefaultInstallDir(wchar_t* out, size_t cch) {
    wchar_t localAppData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppData))) {
        _snwprintf(out, cch, L"%s\\WinGETLauncher", localAppData);
    } else {
        GetCurrentDirectoryW((DWORD)cch, out);
    }
}

static void GetExeDir(wchar_t* out, size_t cch) {
    wchar_t exePath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        out[0] = L'\0';
        return;
    }
    wchar_t* slash = wcsrchr(exePath, L'\\');
    if (slash) *(slash + 1) = L'\0';
    wcsncpy(out, exePath, cch);
    out[cch - 1] = L'\0';
}

static BOOL EnsureDirectoryExists(const wchar_t* fullPath) {
    wchar_t tmp[MAX_PATH * 2];
    wcsncpy(tmp, fullPath, MAX_PATH * 2);
    tmp[MAX_PATH * 2 - 1] = L'\0';

    size_t len = wcslen(tmp);
    while (len > 0 && (tmp[len - 1] == L'\\' || tmp[len - 1] == L'/')) {
        tmp[--len] = L'\0';
    }
    if (len == 0) return FALSE;

    for (size_t i = 0; i < len; i++) {
        if (tmp[i] == L'\\' || tmp[i] == L'/') {
            wchar_t ch = tmp[i];
            tmp[i] = L'\0';
            if (wcslen(tmp) > 2 || (wcslen(tmp) == 2 && tmp[1] != L':')) {
                CreateDirectoryW(tmp, NULL);
            }
            tmp[i] = ch;
        }
    }
    CreateDirectoryW(tmp, NULL);
    DWORD attr = GetFileAttributesW(tmp);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

static BOOL EnsureParentDirForFile(const wchar_t* filePath) {
    wchar_t dir[MAX_PATH * 2];
    wcsncpy(dir, filePath, MAX_PATH * 2);
    dir[MAX_PATH * 2 - 1] = L'\0';
    wchar_t* slash = wcsrchr(dir, L'\\');
    if (!slash) slash = wcsrchr(dir, L'/');
    if (!slash) return TRUE;
    *slash = L'\0';
    if (!*dir) return TRUE;
    return EnsureDirectoryExists(dir);
}

static void CleanupOldFilesInDir(const wchar_t* dirPath) {
    wchar_t pattern[MAX_PATH * 2];
    _snwprintf(pattern, MAX_PATH * 2, L"%s\\*.old", dirPath);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        wchar_t full[MAX_PATH * 2];
        _snwprintf(full, MAX_PATH * 2, L"%s\\%ls", dirPath, ffd.cFileName);

        BOOL deleted = FALSE;
        for (int attempt = 0; attempt < 8; attempt++) {
            SetFileAttributesW(full, FILE_ATTRIBUTE_NORMAL);
            if (DeleteFileW(full)) {
                deleted = TRUE;
                break;
            }
            Sleep(150);
        }
        if (!deleted) {
            MoveFileExW(full, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

static BOOL IsWinGetInstalled(void) {
    wchar_t localAppData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppData))) {
        wchar_t wingetAlias[MAX_PATH];
        _snwprintf(wingetAlias, MAX_PATH, L"%s\\Microsoft\\WindowsApps\\winget.exe", localAppData);
        if (GetFileAttributesW(wingetAlias) != INVALID_FILE_ATTRIBUTES) {
            return TRUE;
        }
    }

    wchar_t foundPath[MAX_PATH] = {0};
    if (SearchPathW(NULL, L"winget.exe", NULL, MAX_PATH, foundPath, NULL) > 0) {
        return TRUE;
    }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    wchar_t cmd[] = L"cmd.exe /c winget --version >nul 2>&1";
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 8000);
        DWORD rc = 1;
        GetExitCodeProcess(pi.hProcess, &rc);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        if (rc == 0) return TRUE;
    }
    return FALSE;
}

static BOOL FindWinGetInstallerScript(const wchar_t* installDir, wchar_t* outScript, size_t cch) {
    const wchar_t* candidates[] = {
        L"winget_install.bat",
        L"source\\winget_install.bat",
        L"scripts\\winget_install.bat",
        L"winget_install.cmd",
        L"source\\winget_install.cmd"
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        _snwprintf(outScript, cch, L"%s\\%ls", installDir, candidates[i]);
        if (GetFileAttributesW(outScript) != INVALID_FILE_ATTRIBUTES) {
            return TRUE;
        }
    }

    return FALSE;
}

static void EnsureWinGetInstalledInBackground(const wchar_t* installDir) {
    AppendLog(L"[INFO] Проверка наличия winget в системе...");
    if (IsWinGetInstalled()) {
        AppendLog(L"[OK] winget уже установлен.");
        return;
    }

    AppendLog(L"[INFO] winget не обнаружен. Поиск winget_install.bat в %ls...", installDir);
    wchar_t scriptPath[MAX_PATH] = {0};
    if (!FindWinGetInstallerScript(installDir, scriptPath, MAX_PATH)) {
        AppendLog(L"[WARN] Файл winget_install.bat не найден в %ls.", installDir);
        return;
    }

    wchar_t cmdLine[MAX_PATH * 4];
    _snwprintf(cmdLine, MAX_PATH * 4, L"cmd.exe /c \"\"%ls\"\"", scriptPath);

    AppendLog(L"[INFO] Фоновая установка winget: %ls", scriptPath);
    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, installDir, &si, &pi)) {
        AppendLog(L"[WARN] Не удалось запустить winget_install.bat (код %lu).", GetLastError());
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 1;
    GetExitCodeProcess(pi.hProcess, &rc);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (IsWinGetInstalled()) {
        AppendLog(L"[OK] winget успешно установлен в фоновом режиме.");
    } else {
        AppendLog(L"[WARN] winget_install.bat завершился с кодом %lu.", rc);
    }
}

static BOOL CreateShortcutW(const wchar_t* lnkPath, const wchar_t* targetPath,
                            const wchar_t* workDir, const wchar_t* args,
                            const wchar_t* iconPath, int iconIndex,
                            const wchar_t* desc) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    BOOL needUninit = SUCCEEDED(hr);

    IShellLinkW* psl = NULL;
    hr = CoCreateInstance(&WGL_CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &WGL_IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr) || !psl) {
        if (needUninit) CoUninitialize();
        return FALSE;
    }

    psl->lpVtbl->SetPath(psl, targetPath);
    if (workDir && *workDir) psl->lpVtbl->SetWorkingDirectory(psl, workDir);
    if (args && *args)       psl->lpVtbl->SetArguments(psl, args);
    if (desc && *desc)       psl->lpVtbl->SetDescription(psl, desc);
    if (iconPath && *iconPath) {
        psl->lpVtbl->SetIconLocation(psl, iconPath, iconIndex);
    }

    IPersistFile* ppf = NULL;
    hr = psl->lpVtbl->QueryInterface(psl, &WGL_IID_IPersistFile, (void**)&ppf);
    BOOL ok = FALSE;
    if (SUCCEEDED(hr) && ppf) {
        hr = ppf->lpVtbl->Save(ppf, lnkPath, TRUE);
        ok = SUCCEEDED(hr);
        ppf->lpVtbl->Release(ppf);
    }
    psl->lpVtbl->Release(psl);
    if (needUninit) CoUninitialize();
    return ok;
}

static void CreateShortcutsIfRequested(const wchar_t* installDir, BOOL desktop, BOOL startMenu) {
    wchar_t exePath[MAX_PATH];
    _snwprintf(exePath, MAX_PATH, L"%s\\launcher.exe", installDir);

    if (desktop) {
        wchar_t desktopDir[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, desktopDir))) {
            wchar_t lnk[MAX_PATH];
            _snwprintf(lnk, MAX_PATH, L"%s\\WinGET Launcher.lnk", desktopDir);
            if (CreateShortcutW(lnk, exePath, installDir, NULL, exePath, 0, L"WinGET Launcher")) {
                AppendLog(L"[OK] Создан ярлык на рабочем столе: %ls", lnk);
            } else {
                AppendLog(L"[WARN] Не удалось создать ярлык на рабочем столе.");
            }
        }
    }

    if (startMenu) {
        wchar_t programsDir[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, SHGFP_TYPE_CURRENT, programsDir))) {
            wchar_t startFolder[MAX_PATH];
            _snwprintf(startFolder, MAX_PATH, L"%s\\WinGET Launcher", programsDir);
            EnsureDirectoryExists(startFolder);

            wchar_t lnk[MAX_PATH];
            _snwprintf(lnk, MAX_PATH, L"%s\\WinGET Launcher.lnk", startFolder);
            if (CreateShortcutW(lnk, exePath, installDir, NULL, exePath, 0, L"WinGET Launcher")) {
                AppendLog(L"[OK] Создан ярлык в меню «Пуск»: %ls", lnk);
            } else {
                AppendLog(L"[WARN] Не удалось создать ярлык в меню «Пуск».");
            }
        }
    }
}

static BOOL HttpGetBytes(const wchar_t* url, char** outData, size_t* outSize) {
    *outData = NULL;
    *outSize = 0;

    HINTERNET hNet = InternetOpenW(L"WinGETSetup/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hNet) return FALSE;

    HINTERNET hUrl = InternetOpenUrlW(hNet, url,
        L"Accept: application/vnd.github+json\r\nUser-Agent: WinGETSetup\r\n",
        (DWORD)-1L,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hUrl) {
        hUrl = InternetOpenUrlW(hNet, url, NULL, 0,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    }
    if (!hUrl) {
        InternetCloseHandle(hNet);
        return FALSE;
    }

    DWORD statusCode = 0;
    DWORD statusLen = sizeof(statusCode);
    if (HttpQueryInfoW(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusLen, NULL)) {
        if (statusCode >= 400) {
            AppendLog(L"[ERROR] HTTP %lu при запросе: %ls", statusCode, url);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hNet);
            return FALSE;
        }
    }

    size_t cap = 65536;
    size_t size = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hNet);
        return FALSE;
    }

    char chunk[8192];
    DWORD read = 0;
    while (InternetReadFile(hUrl, chunk, sizeof(chunk), &read) && read > 0) {
        if (size + read + 1 > cap) {
            while (size + read + 1 > cap) cap *= 2;
            char* nb = (char*)realloc(buf, cap);
            if (!nb) {
                free(buf);
                InternetCloseHandle(hUrl);
                InternetCloseHandle(hNet);
                return FALSE;
            }
            buf = nb;
        }
        memcpy(buf + size, chunk, read);
        size += read;
    }
    buf[size] = '\0';

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);

    *outData = buf;
    *outSize = size;
    return TRUE;
}

static BOOL DownloadUrlToFileW(const wchar_t* url, const wchar_t* destPath) {
    if (!EnsureParentDirForFile(destPath)) return FALSE;

    char* data = NULL;
    size_t size = 0;
    if (!HttpGetBytes(url, &data, &size)) {
        if (data) free(data);
        return FALSE;
    }

    FILE* f = _wfopen(destPath, L"wb");
    if (!f) {
        free(data);
        return FALSE;
    }
    if (size > 0) fwrite(data, 1, size, f);
    fclose(f);
    free(data);
    return size > 0;
}

static BOOL IsValidZipFile(const wchar_t* path) {
    FILE* f = _wfopen(path, L"rb");
    if (!f) return FALSE;
    unsigned char hdr[4] = {0};
    size_t n = fread(hdr, 1, 4, f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return (n == 4 && hdr[0] == 'P' && hdr[1] == 'K' &&
            ((hdr[2] == 0x03 && hdr[3] == 0x04) ||
             (hdr[2] == 0x05 && hdr[3] == 0x06)) && sz > 22);
}

static BOOL IsValidPeExeFile(const wchar_t* path) {
    FILE* f = _wfopen(path, L"rb");
    if (!f) return FALSE;
    unsigned char hdr[2] = {0};
    size_t n = fread(hdr, 1, 2, f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return (n == 2 && hdr[0] == 'M' && hdr[1] == 'Z' && sz > 4096);
}

static BOOL ExtractZipArchive(const wchar_t* zipPath, const wchar_t* outDir) {
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    wchar_t tarExe[MAX_PATH];
    _snwprintf(tarExe, MAX_PATH, L"%s\\tar.exe", sysDir);

    wchar_t cmdLine[MAX_PATH * 4];
    if (GetFileAttributesW(tarExe) != INVALID_FILE_ATTRIBUTES) {
        _snwprintf(cmdLine, MAX_PATH * 4, L"\"%s\" -xf \"%s\" -C \"%s\"", tarExe, zipPath, outDir);
    } else {
        _snwprintf(cmdLine, MAX_PATH * 4,
            L"powershell.exe -NoProfile -Command \"Expand-Archive -LiteralPath '%s' -DestinationPath '%s' -Force\"",
            zipPath, outDir);
    }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 1;
    GetExitCodeProcess(pi.hProcess, &rc);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (rc == 0);
}

static BOOL ExtractJsonStringField(const char* json, const char* key, char* out, size_t outCch) {
    char pattern[128];
    _snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return FALSE;
    p += strlen(pattern);
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (*p != ':') return FALSE;
    p++;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (*p != '"') return FALSE;
    p++;

    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outCch) {
        if (*p == '\\' && p[1]) {
            p++;
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i > 0;
}

static BOOL EndsWithIgnoreCaseA(const char* str, const char* suffix) {
    if (!str || !suffix) return FALSE;
    size_t slen = strlen(str);
    size_t suflen = strlen(suffix);
    if (slen < suflen) return FALSE;
    return (_stricmp(str + slen - suflen, suffix) == 0);
}

static BOOL FindLatestReleaseInfo(wchar_t* outTag, size_t tagCch,
                                  wchar_t* outAssetUrl, size_t urlCch,
                                  wchar_t* outAssetName, size_t assetCch,
                                  BOOL* outIsZip) {
    if (outIsZip) *outIsZip = FALSE;

    char* json = NULL;
    size_t len = 0;
    if (!HttpGetBytes(GH_API_LATEST, &json, &len) || !json || len == 0) {
        if (json) free(json);
        return FALSE;
    }

    char tagBuf[128] = {0};
    if (ExtractJsonStringField(json, "tag_name", tagBuf, sizeof(tagBuf))) {
        MultiByteToWideChar(CP_UTF8, 0, tagBuf, -1, outTag, (int)tagCch);
    } else {
        wcsncpy(outTag, L"latest", tagCch);
        outTag[tagCch - 1] = L'\0';
    }

    const char* cursor = json;
    char zipUrl[1024] = {0};
    char exeUrl[1024] = {0};
    char firstUrl[1024] = {0};

    while ((cursor = strstr(cursor, "\"browser_download_url\"")) != NULL) {
        char urlCandidate[1024] = {0};
        if (ExtractJsonStringField(cursor, "browser_download_url", urlCandidate, sizeof(urlCandidate))) {
            if (!firstUrl[0]) {
                strncpy(firstUrl, urlCandidate, sizeof(firstUrl) - 1);
            }
            if (!zipUrl[0] && EndsWithIgnoreCaseA(urlCandidate, ".zip")) {
                strncpy(zipUrl, urlCandidate, sizeof(zipUrl) - 1);
            }
            if (!exeUrl[0] && EndsWithIgnoreCaseA(urlCandidate, ".exe")) {
                strncpy(exeUrl, urlCandidate, sizeof(exeUrl) - 1);
            }
        }
        cursor += 22;
    }

    free(json);

    const char* chosen = zipUrl[0] ? zipUrl : (exeUrl[0] ? exeUrl : firstUrl);
    if (chosen && chosen[0]) {
        MultiByteToWideChar(CP_UTF8, 0, chosen, -1, outAssetUrl, (int)urlCch);
        const char* slash = strrchr(chosen, '/');
        const char* fname = slash ? (slash + 1) : "release.zip";
        MultiByteToWideChar(CP_UTF8, 0, fname, -1, outAssetName, (int)assetCch);
        if (outIsZip) *outIsZip = EndsWithIgnoreCaseA(chosen, ".zip");
        return TRUE;
    }

    _snwprintf(outAssetUrl, urlCch, L"%sWinGETLauncher.zip", GH_RELEASE_DL);
    wcsncpy(outAssetName, L"WinGETLauncher.zip", assetCch);
    outAssetName[assetCch - 1] = L'\0';
    if (outIsZip) *outIsZip = TRUE;
    return TRUE;
}

typedef struct {
    wchar_t relPath[MAX_PATH];
} FileEntry;

static int LoadFilesListFromGitHub(FileEntry** outList) {
    *outList = NULL;
    char* raw = NULL;
    size_t len = 0;
    if (!HttpGetBytes(GH_FILES_LIST, &raw, &len) || !raw || len == 0) {
        if (raw) free(raw);
        return 0;
    }

    char* p = raw;
    if (len >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) {
        p += 3;
    }

    int capacity = 64;
    int count = 0;
    FileEntry* list = (FileEntry*)calloc((size_t)capacity, sizeof(FileEntry));
    if (!list) {
        free(raw);
        return 0;
    }

    char* line = strtok(p, "\r\n");
    while (line) {
        while (*line == ' ' || *line == '\t') line++;
        char* end = line + strlen(line);
        while (end > line && (end[-1] == ' ' || end[-1] == '\t')) *(--end) = '\0';

        if (*line && *line != '#') {
            if (count >= capacity) {
                capacity *= 2;
                FileEntry* nl = (FileEntry*)realloc(list, (size_t)capacity * sizeof(FileEntry));
                if (!nl) break;
                list = nl;
            }
            wchar_t wRel[MAX_PATH] = {0};
            MultiByteToWideChar(CP_UTF8, 0, line, -1, wRel, MAX_PATH);
            for (wchar_t* s = wRel; *s; s++) {
                if (*s == L'/') *s = L'\\';
            }
            if (wRel[0]) {
                wcsncpy(list[count].relPath, wRel, MAX_PATH);
                list[count].relPath[MAX_PATH - 1] = L'\0';
                count++;
            }
        }
        line = strtok(NULL, "\r\n");
    }

    free(raw);
    *outList = list;
    return count;
}

static BOOL IsScriptOrRepoFile(const wchar_t* relPath) {
    const wchar_t* ext = wcsrchr(relPath, L'.');
    if (!ext) return FALSE;
    return (_wcsicmp(ext, L".bat") == 0 ||
            _wcsicmp(ext, L".ps1") == 0 ||
            _wcsicmp(ext, L".cmd") == 0 ||
            _wcsicmp(ext, L".txt") == 0);
}

static BOOL ReplaceLauncherSafely(const wchar_t* stagedLauncher, const wchar_t* destLauncher, const wchar_t* installDir) {
    if (!IsValidPeExeFile(stagedLauncher)) {
        AppendLog(L"[ERROR] Извлечённый/скачанный launcher.exe повреждён или не является валидным PE (.exe).");
        return FALSE;
    }

    CleanupOldFilesInDir(installDir);

    for (int attempt = 0; attempt < 5; attempt++) {
        if (GetFileAttributesW(destLauncher) != INVALID_FILE_ATTRIBUTES) {
            wchar_t oldLauncher[MAX_PATH];
            _snwprintf(oldLauncher, MAX_PATH, L"%s\\launcher.exe.old", installDir);
            DeleteFileW(oldLauncher);
            MoveFileW(destLauncher, oldLauncher);
        }

        if (MoveFileW(stagedLauncher, destLauncher) || CopyFileW(stagedLauncher, destLauncher, FALSE)) {
            DeleteFileW(stagedLauncher);
            if (IsValidPeExeFile(destLauncher)) {
                CleanupOldFilesInDir(installDir);
                return TRUE;
            }
        }
        Sleep(250);
    }

    AppendLog(L"[ERROR] Не удалось заменить %ls (возможно, процесс всё ещё активен).", destLauncher);
    return FALSE;
}

static void RemoveDirRecursiveW(const wchar_t* dirPath) {
    wchar_t pattern[MAX_PATH * 2];
    _snwprintf(pattern, MAX_PATH * 2, L"%s\\*", dirPath);
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        RemoveDirectoryW(dirPath);
        return;
    }
    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;
        wchar_t full[MAX_PATH * 2];
        _snwprintf(full, MAX_PATH * 2, L"%s\\%ls", dirPath, ffd.cFileName);
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            RemoveDirRecursiveW(full);
        } else {
            DeleteFileW(full);
        }
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);
    RemoveDirectoryW(dirPath);
}

static void CopyExtractedTree(const wchar_t* srcDir, const wchar_t* dstDir, const wchar_t* destLauncher) {
    wchar_t pattern[MAX_PATH * 2];
    _snwprintf(pattern, MAX_PATH * 2, L"%s\\*", srcDir);
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pattern, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;
        wchar_t srcItem[MAX_PATH * 2];
        wchar_t dstItem[MAX_PATH * 2];
        _snwprintf(srcItem, MAX_PATH * 2, L"%s\\%ls", srcDir, ffd.cFileName);
        _snwprintf(dstItem, MAX_PATH * 2, L"%s\\%ls", dstDir, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            EnsureDirectoryExists(dstItem);
            CopyExtractedTree(srcItem, dstItem, destLauncher);
        } else if (_wcsicmp(ffd.cFileName, L"launcher.exe") == 0) {
            ReplaceLauncherSafely(srcItem, destLauncher, dstDir);
        } else {
            EnsureParentDirForFile(dstItem);
            CopyFileW(srcItem, dstItem, FALSE);
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

static DWORD WINAPI SetupWorkerThread(LPVOID param) {
    (void)param;

    wchar_t installDir[MAX_PATH] = {0};
    GetWindowTextW(g_hEditDir, installDir, MAX_PATH);

    size_t dlen = wcslen(installDir);
    while (dlen > 3 && (installDir[dlen - 1] == L'\\' || installDir[dlen - 1] == L'/')) {
        installDir[--dlen] = L'\0';
    }

    if (!installDir[0]) {
        AppendLog(L"[ERROR] Не указана директория установки.");
        PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
        return 1;
    }

    BOOL chkDesktop   = (SendMessageW(g_hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED);
    BOOL chkStartMenu = (SendMessageW(g_hChkStartMenu, BM_GETCHECK, 0, 0) == BST_CHECKED);

    AppendLog(L"Целевая директория: %ls", installDir);
    AppendLog(L"Режим запуска: %ls", g_UpdateMode ? L"Обновление (-update)" : L"Установка");

    if (!EnsureDirectoryExists(installDir)) {
        AppendLog(L"[ERROR] Не удалось создать директорию: %ls", installDir);
        PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
        return 1;
    }

    CleanupOldFilesInDir(installDir);

    SetProgressValue(10, 100);
    AppendLog(L"[INFO] Поиск последнего релиза на GitHub...");

    wchar_t tag[128] = {0};
    wchar_t relUrl[1024] = {0};
    wchar_t assetName[256] = {0};
    BOOL isZip = FALSE;
    BOOL gotRelease = FindLatestReleaseInfo(tag, 128, relUrl, 1024, assetName, 256, &isZip);

    if (gotRelease) {
        AppendLog(L"[INFO] Найден релиз: %ls (%ls)", tag, assetName);
    } else {
        AppendLog(L"[WARN] Не удалось получить данные release через GitHub API, используем архив релиза по умолчанию.");
        _snwprintf(relUrl, 1024, L"%sWinGETLauncher.zip", GH_RELEASE_DL);
        wcscpy(assetName, L"WinGETLauncher.zip");
        isZip = TRUE;
    }

    if (g_UpdateMode) {
        Sleep(800);
    }

    SetProgressValue(25, 100);

    wchar_t destLauncher[MAX_PATH];
    _snwprintf(destLauncher, MAX_PATH, L"%s\\launcher.exe", installDir);

    if (isZip) {
        wchar_t tmpZip[MAX_PATH];
        _snwprintf(tmpZip, MAX_PATH, L"%s\\_update_release.zip", installDir);
        wchar_t extractDir[MAX_PATH];
        _snwprintf(extractDir, MAX_PATH, L"%s\\_update_unpack", installDir);

        DeleteFileW(tmpZip);
        RemoveDirRecursiveW(extractDir);
        EnsureDirectoryExists(extractDir);

        AppendLog(L"[INFO] Скачивание архива релиза: %ls", relUrl);
        if (!DownloadUrlToFileW(relUrl, tmpZip) || !IsValidZipFile(tmpZip)) {
            DeleteFileW(tmpZip);
            RemoveDirRecursiveW(extractDir);
            AppendLog(L"[ERROR] Не удалось скачать или проверить ZIP-архив релиза.");
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }

        AppendLog(L"[INFO] Распаковка архива релиза...");
        if (!ExtractZipArchive(tmpZip, extractDir)) {
            DeleteFileW(tmpZip);
            RemoveDirRecursiveW(extractDir);
            AppendLog(L"[ERROR] Ошибка распаковки архива релиза.");
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }

        wchar_t unpackedLauncher[MAX_PATH * 2];
        _snwprintf(unpackedLauncher, MAX_PATH * 2, L"%s\\launcher.exe", extractDir);
        if (GetFileAttributesW(unpackedLauncher) == INVALID_FILE_ATTRIBUTES) {
            DeleteFileW(tmpZip);
            RemoveDirRecursiveW(extractDir);
            AppendLog(L"[ERROR] В архиве релиза не найден launcher.exe.");
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }

        if (!ReplaceLauncherSafely(unpackedLauncher, destLauncher, installDir)) {
            DeleteFileW(tmpZip);
            RemoveDirRecursiveW(extractDir);
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }
        AppendLog(L"[OK] Обновлён: %ls", destLauncher);

        CopyExtractedTree(extractDir, installDir, destLauncher);
        DeleteFileW(tmpZip);
        RemoveDirRecursiveW(extractDir);
    } else {
        wchar_t tmpLauncher[MAX_PATH];
        _snwprintf(tmpLauncher, MAX_PATH, L"%s\\launcher.exe.new", installDir);

        AppendLog(L"[INFO] Скачивание лаунчера из релиза: %ls", relUrl);
        if (!DownloadUrlToFileW(relUrl, tmpLauncher)) {
            DeleteFileW(tmpLauncher);
            AppendLog(L"[ERROR] Не удалось скачать лаунчер из релиза.");
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }

        if (!ReplaceLauncherSafely(tmpLauncher, destLauncher, installDir)) {
            DeleteFileW(tmpLauncher);
            PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 1, 0);
            return 1;
        }
        AppendLog(L"[OK] Сохранён: %ls", destLauncher);
    }

    SetProgressValue(45, 100);

    AppendLog(L"[INFO] Загрузка списка файлов из files.txt...");
    FileEntry* entries = NULL;
    int fileCount = LoadFilesListFromGitHub(&entries);

    if (fileCount > 0) {
        AppendLog(L"[INFO] Найдено записей в files.txt: %d", fileCount);
        int downloaded = 0;
        for (int i = 0; i < fileCount; i++) {
            const wchar_t* rel = entries[i].relPath;

            if (g_UpdateMode && !IsScriptOrRepoFile(rel)) {
                if (g_DebugMode) AppendLog(L"[DEBUG] Пропуск файла в -update: %ls", rel);
                continue;
            }

            const wchar_t* base = wcsrchr(rel, L'\\');
            base = base ? (base + 1) : rel;
            if (_wcsicmp(base, L"launcher.exe") == 0) continue;

            wchar_t rawUrl[1024];
            wchar_t urlRel[MAX_PATH];
            wcsncpy(urlRel, rel, MAX_PATH);
            urlRel[MAX_PATH - 1] = L'\0';
            for (wchar_t* s = urlRel; *s; s++) {
                if (*s == L'\\') *s = L'/';
            }
            _snwprintf(rawUrl, 1024, L"%s%ls", GH_RAW_BASE, urlRel);

            wchar_t localFile[MAX_PATH];
            _snwprintf(localFile, MAX_PATH, L"%s\\%ls", installDir, rel);

            if (DownloadUrlToFileW(rawUrl, localFile)) {
                downloaded++;
                AppendLog(L"[OK] Загружен: %ls", rel);
            } else {
                AppendLog(L"[WARN] Не удалось загрузить: %ls", rel);
            }

            int pct = 45 + ((i + 1) * 30) / fileCount;
            SetProgressValue(pct, 100);
        }
        AppendLog(L"[INFO] Скачано файлов из files.txt: %d", downloaded);
        free(entries);
    } else {
        AppendLog(L"[WARN] files.txt пуст или недоступен.");
    }

    SetProgressValue(80, 100);
    EnsureWinGetInstalledInBackground(installDir);

    SetProgressValue(92, 100);

    if (chkDesktop || chkStartMenu) {
        AppendLog(L"[INFO] Создание ярлыков...");
        CreateShortcutsIfRequested(installDir, chkDesktop, chkStartMenu);
    }

    CleanupOldFilesInDir(installDir);
    SetProgressValue(100, 100);
    AppendLog(L"[SUCCESS] Операция завершена успешно!");

    if (g_UpdateMode) {
        AppendLog(L"[INFO] Запуск обновлённого launcher.exe...");
        ShellExecuteW(NULL, L"open", destLauncher, g_DebugMode ? L"-debug" : NULL, installDir, SW_SHOWNORMAL);
    }

    PostMessageW(g_hWnd, WM_APP_SETUP_DONE, 0, 0);
    return 0;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    (void)lParam;
    if (uMsg == BFFM_INITIALIZED && lpData) {
        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
    }
    return 0;
}

static void OnBrowseFolder(void) {
    wchar_t curDir[MAX_PATH] = {0};
    GetWindowTextW(g_hEditDir, curDir, MAX_PATH);

    BROWSEINFOW bi = {0};
    bi.hwndOwner = g_hWnd;
    bi.lpszTitle = L"Выберите директорию для установки / обновления WinGET Launcher:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)curDir;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH] = {0};
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(g_hEditDir, path);
        }
        CoTaskMemFree(pidl);
    }
}

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

static void SetControlsEnabled(BOOL enabled) {
    EnableWindow(g_hEditDir, enabled);
    EnableWindow(g_hBtnBrowse, enabled);
    EnableWindow(g_hChkDesktop, enabled);
    EnableWindow(g_hChkStartMenu, enabled);
    EnableWindow(g_hBtnAction, enabled);
}

static void StartSetupOrUpdate(void) {
    if (InterlockedCompareExchange(&g_Busy, 1, 0) != 0) return;
    SetWindowTextW(g_hEditLog, L"");
    SetProgressValue(0, 100);
    SetStatusText(g_UpdateMode ? L"Обновление компонентов..." : L"Установка WinGET Launcher...");
    SetControlsEnabled(FALSE);

    HANDLE hTh = CreateThread(NULL, 0, SetupWorkerThread, NULL, 0, NULL);
    if (hTh) {
        CloseHandle(hTh);
    } else {
        InterlockedExchange(&g_Busy, 0);
        SetControlsEnabled(TRUE);
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hWnd;

        HDC hdc = GetDC(NULL);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);

        LOGFONTW lf = {0};
        lf.lfHeight = -MulDiv(9, dpiY, 72);
        lf.lfWeight = FW_NORMAL;
        wcscpy(lf.lfFaceName, L"Segoe UI");
        g_hFont = CreateFontIndirectW(&lf);

        lf.lfWeight = FW_SEMIBOLD;
        g_hFontBold = CreateFontIndirectW(&lf);

        lf.lfHeight = -MulDiv(13, dpiY, 72);
        lf.lfWeight = FW_SEMIBOLD;
        g_hFontTitle = CreateFontIndirectW(&lf);

        g_hBgBrush = CreateSolidBrush(WIN8_CLR_BG);

        const wchar_t* headerTxt = g_UpdateMode
            ? L"WinGET Launcher — Обновление (-update)"
            : L"WinGET Launcher — Установка";

        HWND hTitle = CreateWindowW(L"STATIC", headerTxt,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            16, 14, 540, 26, hWnd, (HMENU)ID_LBL_TITLE, NULL, NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        HWND hLblDir = CreateWindowW(L"STATIC", L"Директория установки / обновления:",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            16, 48, 400, 20, hWnd, (HMENU)ID_LBL_DIR, NULL, NULL);
        SendMessageW(hLblDir, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        wchar_t defaultDir[MAX_PATH] = {0};
        if (g_UpdateMode && g_StartDir[0]) {
            wcsncpy(defaultDir, g_StartDir, MAX_PATH);
            defaultDir[MAX_PATH - 1] = L'\0';
            size_t l = wcslen(defaultDir);
            if (l > 3 && defaultDir[l - 1] == L'\\') defaultDir[l - 1] = L'\0';
        } else {
            GetDefaultInstallDir(defaultDir, MAX_PATH);
        }

        g_hEditDir = CreateWindowExW(0, L"EDIT", defaultDir,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            16, 70, 435, 26, hWnd, (HMENU)ID_EDIT_DIR, NULL, NULL);
        SendMessageW(g_hEditDir, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hBtnBrowse = CreateWindowW(L"BUTTON", L"Обзор...",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            459, 70, 97, 26, hWnd, (HMENU)ID_BTN_BROWSE, NULL, NULL);

        g_hChkDesktop = CreateWindowW(L"BUTTON", L"Создать ярлык на рабочем столе",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            16, 106, 260, 22, hWnd, (HMENU)ID_CHK_DESKTOP, NULL, NULL);
        SendMessageW(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);

        g_hChkStartMenu = CreateWindowW(L"BUTTON", L"Создать ярлык в меню «Пуск»",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            285, 106, 270, 22, hWnd, (HMENU)ID_CHK_STARTMENU, NULL, NULL);
        SendMessageW(g_hChkStartMenu, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            16, 138, 540, 18, hWnd, (HMENU)ID_PROGRESS, NULL, NULL);
        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgress, PBM_SETBARCOLOR, 0, WIN8_CLR_ACCENT);

        g_hEditLog = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            16, 164, 540, 185, hWnd, (HMENU)ID_EDIT_LOG, NULL, NULL);
        SendMessageW(g_hEditLog, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hLblStatus = CreateWindowW(L"STATIC",
            g_UpdateMode ? L"Запуск обновления..." : L"Готово к установке",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            16, 356, 540, 20, hWnd, (HMENU)ID_LBL_STATUS, NULL, NULL);
        SendMessageW(g_hLblStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        const wchar_t* actionCaption = g_UpdateMode ? L"Обновить" : L"Установить";
        g_hBtnAction = CreateWindowW(L"BUTTON", actionCaption,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            346, 384, 100, 30, hWnd, (HMENU)ID_BTN_ACTION, NULL, NULL);

        g_hBtnCancel = CreateWindowW(L"BUTTON", L"Закрыть",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            456, 384, 100, 30, hWnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);

        if (g_UpdateMode) {
            StartSetupOrUpdate();
        }
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_BROWSE:
            OnBrowseFolder();
            break;
        case ID_BTN_ACTION:
            StartSetupOrUpdate();
            break;
        case ID_BTN_CANCEL:
            if (InterlockedCompareExchange(&g_Busy, 0, 0) != 0) {
                if (MessageBoxW(hWnd, L"Операция ещё выполняется. Закрыть установщик?",
                        L"Подтверждение", MB_YESNO | MB_ICONQUESTION) == IDNO) {
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
            BOOL primary = (dis->CtlID == ID_BTN_ACTION);
            DrawWin8Button(dis, primary);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hBgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, WIN8_CLR_BG);
        SetTextColor(hdc, WIN8_CLR_TEXT);
        return (LRESULT)g_hBgBrush;
    }

    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect((HDC)wParam, &rc, g_hBgBrush);
        return 1;
    }

    case WM_APP_LOG_LINE: {
        wchar_t* line = (wchar_t*)lParam;
        if (line && g_hEditLog) {
            int curLen = GetWindowTextLengthW(g_hEditLog);
            SendMessageW(g_hEditLog, EM_SETSEL, (WPARAM)curLen, (LPARAM)curLen);
            SendMessageW(g_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)line);
            free(line);
        }
        return 0;
    }

    case WM_APP_PROGRESS: {
        int cur = (int)wParam;
        int tot = (int)lParam;
        if (tot <= 0) tot = 100;
        SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, tot));
        SendMessageW(g_hProgress, PBM_SETPOS, (WPARAM)cur, 0);
        return 0;
    }

    case WM_APP_SETUP_DONE: {
        InterlockedExchange(&g_Busy, 0);
        SetControlsEnabled(TRUE);
        if (wParam == 0) {
            SetStatusText(g_UpdateMode ? L"Обновление завершено" : L"Установка завершена");
            if (g_UpdateMode) {
                DestroyWindow(hWnd);
            }
        } else {
            SetStatusText(L"Завершено с ошибками");
        }
        return 0;
    }

    case WM_DESTROY: {
        if (g_hFont)      DeleteObject(g_hFont);
        if (g_hFontBold)  DeleteObject(g_hFontBold);
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hBgBrush)   DeleteObject(g_hBgBrush);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void ParseArgs(void) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return;

    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-update") == 0 ||
            _wcsicmp(argv[i], L"--update") == 0 ||
            _wcsicmp(argv[i], L"/update") == 0) {
            g_UpdateMode = TRUE;
        } else if (_wcsicmp(argv[i], L"-debug") == 0 ||
                   _wcsicmp(argv[i], L"--debug") == 0 ||
                   _wcsicmp(argv[i], L"/debug") == 0) {
            g_DebugMode = TRUE;
        }
    }
    LocalFree(argv);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;

    ParseArgs();
    GetExeDir(g_StartDir, MAX_PATH);
    if (g_StartDir[0]) {
        wchar_t cleanDir[MAX_PATH];
        wcsncpy(cleanDir, g_StartDir, MAX_PATH);
        cleanDir[MAX_PATH - 1] = L'\0';
        size_t l = wcslen(cleanDir);
        if (l > 3 && cleanDir[l - 1] == L'\\') cleanDir[l - 1] = L'\0';
        CleanupOldFilesInDir(cleanDir);
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"WinGETSetupWnd";
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;

    if (!RegisterClassExW(&wc)) return 1;

    HWND hWnd = CreateWindowExW(
        0, wc.lpszClassName,
        g_UpdateMode ? L"WinGET Launcher Setup (-update)" : L"WinGET Launcher Setup",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 580, 465,
        NULL, NULL, hInst, NULL);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}