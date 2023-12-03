#include <windows.h>
#include <zlib.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>
#include <zip.h>


#define ID_COMPRESS_BUTTON 101
#define ID_EXTRACT_BUTTON 102

HINSTANCE hInst;
HWND hMainWindow;
HWND hArchivePathEdit;
HWND hAddFileButton;
HWND hCompressButton;
HWND hExtractButton;

// Преобразование строк из wchar_t в char
char* ConvertWCharToChar(const wchar_t* wstr) {
    int length = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* str = new char[length];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, length, NULL, NULL);
    return str;
}

// Преобразование строк из char в wchar_t
wchar_t* ConvertCharToWChar(const char* str) {
    int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[length];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, length);
    return wstr;
}

void CompressFiles(const wchar_t* archivePath, const wchar_t* filesToCompress[], int fileCount) {
    char* archivePathChar = ConvertWCharToChar(archivePath);

    struct zip* archive = zip_open(archivePathChar, ZIP_CREATE | ZIP_TRUNCATE, NULL);
    if (archive) {
        for (int i = 0; i < fileCount; i++) {
            char* fileToCompressChar = ConvertWCharToChar(filesToCompress[i]);
            zip_source_t* source = zip_source_file(archive, fileToCompressChar, 0, -1);
            zip_file_add(archive, fileToCompressChar, source, 0);
            zip_source_free(source);
            delete[] fileToCompressChar;
        }
        zip_close(archive);
    }
    delete[] archivePathChar;
}

void ExtractFiles(const wchar_t* archivePath, const wchar_t* outputPath) {
    char* archivePathChar = ConvertWCharToChar(archivePath);
    char* outputPathChar = ConvertWCharToChar(outputPath);

    struct zip* archive = zip_open(archivePathChar, 0, NULL);
    if (archive) {
        zip_int64_t num_entries = zip_get_num_entries(archive, 0);
        for (zip_int64_t i = 0; i < num_entries; i++) {
            struct zip_stat stat;
            zip_stat_init(&stat);
            zip_stat(archive, i, 0, &stat);
            wchar_t entryName[MAX_PATH];
            char* entryNameChar = ConvertWCharToChar(entryName);

            zip_file_t* file = zip_fopen_index(archive, i, 0);
            if (file) {
                char* buffer = new char[stat.size];
                zip_fread(file, buffer, stat.size);
                zip_fclose(file);

                wchar_t outputPathFile[MAX_PATH];
                wsprintf(outputPathFile, L"%s\\%s", outputPath, entryName);
                // Преобразование строки из wchar_t в char
                char* outputPathFileChar = ConvertWCharToChar(outputPathFile);

                HANDLE outputFile = CreateFile(outputPathFileChar, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (outputFile != INVALID_HANDLE_VALUE) {
                    DWORD bytesWritten;
                    WriteFile(outputFile, buffer, stat.size, &bytesWritten, NULL);
                    CloseHandle(outputFile);
                }
                delete[] buffer;
                delete[] outputPathFileChar;
                delete[] entryNameChar;
            }
        }
        zip_close(archive);
    }
    delete[] archivePathChar;
    delete[] outputPathChar;
}

LRESULT CALLBACK CompressButton_Click(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND && LOWORD(wParam) == ID_COMPRESS_BUTTON) {
        wchar_t archivePath[MAX_PATH];
        GetWindowText(hArchivePathEdit, archivePath, MAX_PATH);

        const wchar_t* filesToCompress[] = { L"file1.txt", L"file2.txt" };
        int fileCount = sizeof(filesToCompress) / sizeof(filesToCompress[0]);

        CompressFiles(archivePath, filesToCompress, fileCount);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


LRESULT CALLBACK CompressButton_Click(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND && LOWORD(wParam) == ID_COMPRESS_BUTTON) {
        wchar_t archivePath[MAX_PATH];
        GetWindowText(hArchivePathEdit, archivePath, MAX_PATH);

        const wchar_t* filesToCompress[] = { L"file1.txt", L"file2.txt" };
        int fileCount = sizeof(filesToCompress) / sizeof(filesToCompress[0]);

        CompressFiles(archivePath, filesToCompress, fileCount);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_COMPRESS_BUTTON) {
            // Обработчик события для кнопки "Сжать"
            return CompressButton_Click(hwnd, msg, wParam, lParam);
        }
        else if (LOWORD(wParam) == ID_EXTRACT_BUTTON) {
            // Обработчик события для кнопки "Распаковать"
            return ExtractButton_Click(hwnd, msg, wParam, lParam);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

LRESULT CALLBACK ExtractButton_Click(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND && LOWORD(wParam) == ID_EXTRACT_BUTTON) {
        wchar_t archivePath[MAX_PATH];
        GetWindowText(hArchivePathEdit, archivePath, MAX_PATH);

        wchar_t outputPath[MAX_PATH];
        // Укажите каталог для извлечения файлов

        ExtractFiles(archivePath, outputPath);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    WNDCLASS wc = { 0 };
    wc.lpszClassName = L"ZipApp";
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    RegisterClass(&wc);

    hMainWindow = CreateWindow(wc.lpszClassName, L"Zip Application", WS_OVERLAPPEDWINDOW, 100, 100, 600, 400, NULL, NULL, hInstance, NULL);

    hArchivePathEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 10, 10, 400, 30, hMainWindow, NULL, hInstance, NULL);
    hAddFileButton = CreateWindow(L"BUTTON", L"Add Files", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 420, 10, 150, 30, hMainWindow, (HMENU)ID_COMPRESS_BUTTON, hInstance, NULL);
    hCompressButton = CreateWindow(L"BUTTON", L"Compress", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 50, 150, 30, hMainWindow, (HMENU)ID_COMPRESS_BUTTON, hInstance, NULL);
    hExtractButton = CreateWindow(L"BUTTON", L"Extract", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 50, 150, 30, hMainWindow, (HMENU)ID_EXTRACT_BUTTON, hInstance, NULL);

    ShowWindow(hMainWindow, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
