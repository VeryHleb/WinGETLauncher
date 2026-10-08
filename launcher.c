#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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
    // ---- Маркер завершения .bat ----
    if (wcsstr(line, L"[BAT_DONE]")) {
        const wchar_t* p = wcsstr(line, L"rc=");
        if (p) g_BatFinalRc = _wtoi(p + 3);
        g_BatFinished = TRUE;
        InterlockedExchange(&g_Running, 0);
        return;
    }

    // ---- Один счётчик на пакет ----
    if (wcsstr(line, L"[INSTALL]")) {
        g_WaitingForResult = TRUE;
        return;
    }

    if (!g_WaitingForResult) return;

    if (wcsstr(line, L"[OK]")   ||
        wcsstr(line, L"[SKIP]") ||
        wcsstr(line, L"[WARN]") ||
        wcsstr(line, L"[FAIL]"))
    {
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
    while (nl) {
        ParseWingetLine(nl);
        nl = wcstok_s(NULL, L"\r\n", &ctx);
    }
    free(wbuf);
}

// ============================================================
// Repository parser
// ============================================================
static BOOL ReadRepoLine(FILE* f, wchar_t* out_id, size_t id_cch,
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

        char* sep = strchr(p, '|');
        char* id_part   = p;
        char* name_part = NULL;

        if (sep) {
            *sep = '\0';
            name_part = sep + 1;
            while (*name_part == ' ' || *name_part == '\t') name_part++;
        } else {
            char* c = strpbrk(id_part, ",;");
            if (c) {
                *c = '\0';
                char* right = c + 1;
                while (*right == ' ' || *right == '\t') right++;
                if (strchr(right, '.')) id_part = right;
            }
        }

        char* id_end = id_part + strlen(id_part);
        while (id_end > id_part && (id_end[-1] == ' ' || id_end[-1] == '\t')) {
            *(--id_end) = '\0';
        }

        if (*id_part == '\0') continue;

        int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, id_part, -1, NULL, 0);
        if (wlen > 0) {
            MultiByteToWideChar(CP_UTF8, 0, id_part, -1, out_id, (int)id_cch);
        } else {
            MultiByteToWideChar(CP_ACP, 0, id_part, -1, out_id, (int)id_cch);
        }

        if (name_part && *name_part != '\0') {
            wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name_part, -1, NULL, 0);
            if (wlen > 0) {
                MultiByteToWideChar(CP_UTF8, 0, name_part, -1, out_name, (int)name_cch);
            } else {
                MultiByteToWideChar(CP_ACP, 0, name_part, -1, out_name, (int)name_cch);
            }
        } else {
            wcsncpy(out_name, out_id, name_cch);
            out_name[name_cch - 1] = L'\0';
        }

        return TRUE;
    }
    return FALSE;
}

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

    wchar_t id[256];
    wchar_t name[256];
    int idx = 0;

    while (ReadRepoLine(f, id, 256, name, 256)) {
        LVITEMW item = {0};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = idx;
        item.iSubItem = 0;
        item.pszText = name;
        item.lParam = (LPARAM)_wcsdup(id);
        int row = (int)SendMessageW(g_hList, LVM_INSERTITEMW, 0, (LPARAM)&item);

        LVITEMW sub = {0};
        sub.mask = LVIF_TEXT;
        sub.iItem = row;
        sub.iSubItem = 1;
        sub.pszText = id;
        SendMessageW(g_hList, LVM_SETITEMW, 0, (LPARAM)&sub);

        ListView_SetCheckState(g_hList, row, TRUE);
        idx++;
    }
    fclose(f);

    wchar_t status[128];
    _snwprintf(status, 128, L"Загружено %d приложений", idx);
    SetStatus(status);
}

static int GetCheckedCount(void) {
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    int checked = 0;
    for (int i = 0; i < count; i++) {
        if (ListView_GetCheckState(g_hList, i)) checked++;
    }
    return checked;
}

// ---- WriteTmpRepo: БЕЗ BOM ----
static BOOL WriteTmpRepo(const wchar_t* path) {
    FILE* f = _wfopen(path, L"wb");
    if (!f) return FALSE;

    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        if (!ListView_GetCheckState(g_hList, i)) continue;

        wchar_t buf[256];
        ListView_GetItemText(g_hList, i, 1, buf, 256);
        if (buf[0] == L'\0') {
            ListView_GetItemText(g_hList, i, 0, buf, 256);
        }

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
// Run .bat thread
// ============================================================
static DWORD WINAPI RunBatThread(LPVOID param) {
    (void)param;

    wchar_t batPath[MAX_PATH];
    wchar_t tmpRepo[MAX_PATH];

    if (!GetBatPath(batPath, MAX_PATH) || !GetTmpRepoPath(tmpRepo, MAX_PATH)) {
        AppendOutput(L"[ERROR] Cannot resolve paths.\r\n");
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
        return 1;
    }

    if (GetFileAttributesW(batPath) == INVALID_FILE_ATTRIBUTES) {
        AppendOutput(L"[ERROR] install_apps.bat not found next to launcher.\r\n");
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
        return 1;
    }

    DeleteFileW(tmpRepo);

    if (!WriteTmpRepo(tmpRepo)) {
        AppendOutput(L"[ERROR] Cannot write temporary repository file.\r\n");
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
        return 1;
    }

    SetEnvironmentVariableW(L"REPO_FILE", tmpRepo);

    wchar_t cmdLine[MAX_PATH * 2];
    _snwprintf(cmdLine, MAX_PATH * 2, L"cmd.exe /c \"\"%s\"\"", batPath);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE hRead = NULL, hWrite = NULL;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        AppendOutput(L"[ERROR] CreatePipe failed.\r\n");
        DeleteFileW(tmpRepo);
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
        return 1;
    }
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(
        NULL, cmdLine, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(hWrite);

    if (!ok) {
        AppendOutput(L"[ERROR] CreateProcess failed.\r\n");
        CloseHandle(hRead);
        DeleteFileW(tmpRepo);
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
        return 1;
    }

    g_hProcess = pi.hProcess;

    char buf[4096];
    DWORD read = 0;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
        buf[read] = '\0';
        AppendOutputA(buf);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD rc = 0;
    GetExitCodeProcess(pi.hProcess, &rc);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hRead);
    g_hProcess = NULL;

    DeleteFileW(tmpRepo);

    wchar_t msg[128];
    _snwprintf(msg, 128, L"\r\n=== Готово. Код выхода: %lu ===\r\n", rc);
    AppendOutput(msg);

    int finalRc = g_BatFinished ? g_BatFinalRc : (int)rc;

    if (finalRc == 0) {
        SetStatus(L"Successfully  ·  Установка завершена");
        g_StatusSuccess = TRUE;
    } else {
        SetStatus(L"Ошибка  ·  Смотрите лог");
        g_StatusSuccess = FALSE;
    }

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
        InterlockedExchange(&g_Running, 0);
        return;
    }

    g_TotalPackages    = selected;
    g_DonePackages     = 0;
    g_WaitingForResult = FALSE;
    g_BatFinished      = FALSE;
    g_BatFinalRc       = -1;
    g_StatusSuccess    = FALSE;

    SetTotalProgress(0, g_TotalPackages);

    SetWindowTextW(g_hOutput, L"");
    wchar_t msg[128];
    _snwprintf(msg, 128, L"=== Запуск install_apps.bat (выбрано: %d) ===\r\n\r\n", selected);
    AppendOutput(msg);
    SetStatus(L"Установка...");
    EnableButtons(FALSE);

    g_hThread = CreateThread(NULL, 0, RunBatThread, NULL, 0, NULL);
    if (!g_hThread) {
        AppendOutput(L"[ERROR] CreateThread failed.\r\n");
        InterlockedExchange(&g_Running, 0);
        EnableButtons(TRUE);
    } else {
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
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

static void SetAllCheckState(BOOL state) {
    int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        ListView_SetCheckState(g_hList, i, state);
    }
}

// ============================================================
// Owner-draw buttons (Aero style)
// ============================================================
static void DrawAeroButton(LPDRAWITEMSTRUCT dis) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    BOOL pressed  = (dis->itemState & ODS_SELECTED) != 0;
    BOOL disabled = (dis->itemState & ODS_DISABLED) != 0;
    BOOL focused  = (dis->itemState & ODS_FOCUS) != 0;
    BOOL hot      = (dis->itemState & ODS_HOTLIGHT) != 0;

    FillRect(hdc, &rc, g_hBkgBrush);

    COLORREF top, bottom, border;
    if (disabled) {
        top = RGB(240, 240, 240); bottom = RGB(220, 220, 220); border = RGB(180, 180, 180);
    } else if (pressed) {
        top = RGB(180, 210, 240); bottom = RGB(140, 180, 220); border = RGB(80, 120, 170);
    } else if (hot) {
        top = RGB(250, 252, 255); bottom = RGB(210, 230, 250); border = RGB(90, 140, 200);
    } else {
        top = RGB(250, 250, 250); bottom = RGB(225, 230, 240); border = RGB(140, 150, 170);
    }

    int radius = 4;
    HRGN hRgn = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1,
                                   radius * 2, radius * 2);
    SelectClipRgn(hdc, hRgn);

    TRIVERTEX vert[2];
    vert[0].x = rc.left;   vert[0].y = rc.top;
    vert[0].Red   = (COLOR16)(GetRValue(top)    << 8);
    vert[0].Green = (COLOR16)(GetGValue(top)    << 8);
    vert[0].Blue  = (COLOR16)(GetBValue(top)    << 8);
    vert[0].Alpha = 0;

    vert[1].x = rc.right;  vert[1].y = rc.bottom;
    vert[1].Red   = (COLOR16)(GetRValue(bottom) << 8);
    vert[1].Green = (COLOR16)(GetGValue(bottom) << 8);
    vert[1].Blue  = (COLOR16)(GetBValue(bottom) << 8);
    vert[1].Alpha = 0;

    GRADIENT_RECT gr = {0, 1};
    GradientFill(hdc, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    HPEN hPenLight = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenLight);
    MoveToEx(hdc, rc.left + 2, rc.top + 1, NULL);
    LineTo(hdc, rc.right - 2, rc.top + 1);

    HPEN hPenBorder = CreatePen(PS_SOLID, 1, border);
    SelectObject(hdc, hPenBorder);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius * 2, radius * 2);

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPenLight);
    DeleteObject(hPenBorder);
    SelectClipRgn(hdc, NULL);
    DeleteObject(hRgn);

    wchar_t text[128];
    GetWindowTextW(dis->hwndItem, text, 128);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, disabled ? RGB(140, 140, 140) : RGB(30, 30, 30));
    HFONT hOldFont = (HFONT)SelectObject(hdc, g_hFont);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);

    if (focused && !disabled) {
        RECT rf = rc;
        InflateRect(&rf, -3, -3);
        DrawFocusRect(hdc, &rf);
    }
}

// ============================================================
// Window background (gradient)
// ============================================================
static void DrawWindowBackground(HWND hWnd, HDC hdc) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    TRIVERTEX vert[2];
    vert[0].x = 0; vert[0].y = 0;
    vert[0].Red   = 0xE0 << 8;
    vert[0].Green = 0xEE << 8;
    vert[0].Blue  = 0xFF << 8;
    vert[0].Alpha = 0;

    vert[1].x = w; vert[1].y = h;
    vert[1].Red   = 0xFF << 8;
    vert[1].Green = 0xFF << 8;
    vert[1].Blue  = 0xFF << 8;
    vert[1].Alpha = 0;

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
            btnStyle, 10, 10, 180, 32, hWnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
        g_hBtnSelAll = CreateWindowW(L"BUTTON", L"Выбрать все",
            btnStyle, 200, 10, 130, 32, hWnd, (HMENU)ID_BTN_SELALL, NULL, NULL);
        g_hBtnSelNone = CreateWindowW(L"BUTTON", L"Снять все",
            btnStyle, 340, 10, 130, 32, hWnd, (HMENU)ID_BTN_SELNONE, NULL, NULL);
        g_hBtnLogs = CreateWindowW(L"BUTTON", L"Открыть логи",
            btnStyle, 480, 10, 120, 32, hWnd, (HMENU)ID_BTN_LOGS, NULL, NULL);
        g_hBtnExit = CreateWindowW(L"BUTTON", L"Выход",
            btnStyle, 610, 10, 90, 32, hWnd, (HMENU)ID_BTN_EXIT, NULL, NULL);

        g_hList = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS,
            10, 52, 330, 400, hWnd, (HMENU)ID_LISTVIEW, NULL, NULL);

        ListView_SetExtendedListViewStyle(g_hList,
            LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNW col0 = {0};
        col0.mask = LVCF_TEXT | LVCF_WIDTH;
        col0.pszText = L"Программа";
        col0.cx = 300;
        SendMessageW(g_hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col0);

        LVCOLUMNW col1 = {0};
        col1.mask = LVCF_TEXT | LVCF_WIDTH;
        col1.pszText = L"ID";
        col1.cx = 0;
        SendMessageW(g_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col1);

        SendMessageW(g_hList, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hOutput = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE |
            ES_AUTOVSCROLL | ES_READONLY,
            350, 52, 380, 400, hWnd, (HMENU)ID_EDIT_OUTPUT, NULL, NULL);
        SendMessageW(g_hOutput, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hLblTotal = CreateWindowW(L"STATIC", L"Установка: 0 / 0",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 490, 720, 18, hWnd, (HMENU)ID_LABEL_TOTAL, NULL, NULL);
        SendMessageW(g_hLblTotal, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hProgTotal = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            10, 512, 720, 20, hWnd, (HMENU)ID_PROGRESS_TOTAL, NULL, NULL);
        SendMessageW(g_hProgTotal, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessageW(g_hProgTotal, PBM_SETPOS, 0, 0);

        g_hStatus = CreateWindowW(L"STATIC", L"Готов",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 545, 720, 20, hWnd, (HMENU)ID_STATUSBAR, NULL, NULL);
        SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        LoadRepository();
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_INSTALL:  OnInstallClicked();       break;
        case ID_BTN_SELALL:   SetAllCheckState(TRUE);   break;
        case ID_BTN_SELNONE:  SetAllCheckState(FALSE);  break;
        case ID_BTN_LOGS:     OnLogsClicked();          break;
        case ID_BTN_EXIT:
            if (g_BatFinished) {
                DestroyWindow(hWnd);
                break;
            }
            if (g_hProcess && WaitForSingleObject(g_hProcess, 0) == WAIT_OBJECT_0) {
                DestroyWindow(hWnd);
                break;
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
        if (dis->CtlType == ODT_BUTTON) {
            DrawAeroButton(dis);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtl = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);

        if (hCtl == g_hStatus && g_StatusSuccess) {
            SetTextColor(hdc, RGB(0, 128, 0));
        } else {
            SetTextColor(hdc, RGB(30, 30, 30));
        }
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
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        int topPanelH    = 52;
        int bottomPanelH = 110;
        int listW        = 330;

        int contentH = h - topPanelH - bottomPanelH;

        if (g_hList)   MoveWindow(g_hList,   10,         52, listW,           contentH, TRUE);
        if (g_hOutput) MoveWindow(g_hOutput, listW + 20, 52, w - listW - 30,  contentH, TRUE);

        int y = h - bottomPanelH;

        if (g_hLblTotal)  MoveWindow(g_hLblTotal,  10, y,      w - 20, 18, TRUE);
        if (g_hProgTotal) MoveWindow(g_hProgTotal, 10, y + 22, w - 20, 20, TRUE);
        if (g_hStatus)    MoveWindow(g_hStatus,    10, y + 54, w - 20, 20, TRUE);
        return 0;
    }

    case WM_DESTROY: {
        int count = (int)SendMessageW(g_hList, LVM_GETITEMCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            LVITEMW item = {0};
            item.mask = LVIF_PARAM;
            item.iItem = i;
            if (SendMessageW(g_hList, LVM_GETITEMW, 0, (LPARAM)&item)) {
                if (item.lParam) free((void*)item.lParam);
            }
        }
        if (g_hFont)      DeleteObject(g_hFont);
        if (g_hFontBold)  DeleteObject(g_hFontBold);
        if (g_hBkgBrush)  DeleteObject(g_hBkgBrush);
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

    HWND hWnd = CreateWindowExW(
        0, wc.lpszClassName,
        L"Winget Installer",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 660,
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