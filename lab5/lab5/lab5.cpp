#include <windows.h>
#include <string>
#include <vector>

// Объявление функций
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::vector<std::wstring> GetObsoleteRegistryEntries();
void EnumerateRegistryEntries(HKEY hKey, const std::wstring& subKey, std::vector<std::wstring>& obsoleteEntries);
bool IsObsoleteRegistryEntry(HKEY hKey, const std::wstring& valueName);
void AnalyzeRegistry(HWND hwnd);

// Глобальная переменная для хранения дескриптора кнопки
HWND g_AnalyzeButton;
HWND g_ListBox;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {
    // Регистрация класса окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"RegistryAnalyzer";

    RegisterClass(&wc);

    // Создание окна
    HWND hwnd = CreateWindowEx(
        0, L"RegistryAnalyzer", L"Registry Analyzer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 300, NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window creation failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // Создание кнопки "Проанализировать"
    g_AnalyzeButton = CreateWindow(
        L"BUTTON", L"Проанализировать", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 10, 150, 30, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL
    );

    // Отображение окна
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Основной цикл сообщений
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            // Обработка нажатия кнопки
            if (reinterpret_cast<HWND>(lParam) == g_AnalyzeButton) {
                AnalyzeRegistry(hwnd);
            }
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void AnalyzeRegistry(HWND hwnd) {
    std::vector<std::wstring> obsoleteEntries = GetObsoleteRegistryEntries();

    // Если список уже создан, удалим его
    if (g_ListBox) {
        DestroyWindow(g_ListBox);
    }

    // Создаем список
    g_ListBox = CreateWindow(
        L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | LBS_STANDARD | LBS_NOTIFY,
        10, 50, 480, 200, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL
    );

    if (!obsoleteEntries.empty()) {
        // Если есть устаревшие записи, добавляем их в список
        for (const auto& entry : obsoleteEntries) {
            SendMessage(g_ListBox, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
        }
    }
    else {
        // Если устаревших записей нет, добавляем сообщение
        SendMessage(g_ListBox, LB_ADDSTRING, 0, (LPARAM)L"Ничего не найдено");
    }
}

std::vector<std::wstring> GetObsoleteRegistryEntries() {
    std::vector<std::wstring> obsoleteEntries;

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        EnumerateRegistryEntries(hKey, L"", obsoleteEntries);
        RegCloseKey(hKey);
    }

    return obsoleteEntries;
}

void EnumerateRegistryEntries(HKEY hKey, const std::wstring& subKey, std::vector<std::wstring>& obsoleteEntries) {
    HKEY hSubKey;
    if (RegOpenKeyEx(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        constexpr DWORD bufferSize = 256;
        wchar_t valueName[bufferSize];
        DWORD valueNameSize = bufferSize;

        while (RegEnumValue(hSubKey, index, valueName, &valueNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            std::wstring fullKey = subKey.empty() ? valueName : subKey + L"\\" + valueName;
            if (IsObsoleteRegistryEntry(hSubKey, valueName)) {
                obsoleteEntries.push_back(fullKey);
            }

            // Сброс размера буфера перед следующей итерацией
            valueNameSize = bufferSize;
            index++;
        }

        // Рекурсивный вызов для подразделов
        DWORD subIndex = 0;
        constexpr DWORD subKeySize = 256;
        wchar_t subKeyName[subKeySize];
        DWORD subKeyNameSize = subKeySize;

        while (RegEnumKeyEx(hSubKey, subIndex, subKeyName, &subKeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            std::wstring fullSubKey = subKey.empty() ? subKeyName : subKey + L"\\" + subKeyName;
            EnumerateRegistryEntries(hSubKey, fullSubKey, obsoleteEntries);

            // Сброс размера буфера перед следующей итерацией
            subKeyNameSize = subKeySize;
            subIndex++;
        }

        RegCloseKey(hSubKey);
    }
}

bool IsObsoleteRegistryEntry(HKEY hKey, const std::wstring& valueName) {
    constexpr DWORD bufferSize = 256;
    wchar_t data[bufferSize];
    DWORD dataSize = bufferSize;
    DWORD dataType;

    if (RegQueryValueEx(hKey, valueName.c_str(), NULL, &dataType, reinterpret_cast<BYTE*>(data), &dataSize) == ERROR_SUCCESS) {
        if (dataType == REG_SZ || dataType == REG_EXPAND_SZ) {
            // Проверка, начинается ли значение с "Obsolete"
            if (wcsncmp(data, L"Obsolete", wcslen(L"Obsolete")) == 0) {
                return true;
            }
        }
    }

    return false;
}

