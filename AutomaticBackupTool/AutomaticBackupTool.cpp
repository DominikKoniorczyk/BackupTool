#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Comctl32.lib")

namespace fs = std::filesystem;
std::wstring outputFolder;

// -------- GLOBALS --------
HWND hListBox;
HWND hProgressBar;
std::vector<std::wstring> paths;
std::wofstream backupLog;
fs::path backupRoot;
size_t currentFile = 0;
size_t totalFiles = 0;

// -------- CONFIG --------
const std::wstring CONFIG_FILE = L"config.txt";


COLORREF GetWindowsAccentColor() {
    DWORD color = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
        return RGB(GetRValue(color), GetGValue(color), GetBValue(color));
    }
    return GetSysColor(COLOR_WINDOW);
}

// -------- UTILS --------
void LoadConfig() {
    paths.clear();
    std::wifstream f(CONFIG_FILE);
    std::wstring line;
    bool firstLine = true;
    while (std::getline(f, line)) {
        if (firstLine) {
            if (!line.empty()) outputFolder = line;
            firstLine = false;
            continue;
        }
        if (!line.empty()) paths.push_back(line);
    }
}

void SaveConfig() {
    std::wofstream f(CONFIG_FILE);
    f << outputFolder << L"\n";
    for (auto& p : paths) f << p << L"\n";
}

std::wstring BrowseFolder(HWND hwnd) {
    std::wstring folder;
    IFileDialog* pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD options;
        pfd->GetOptions(&options);
        pfd->SetOptions(options | FOS_PICKFOLDERS);
        if (SUCCEEDED(pfd->Show(hwnd))) {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    folder = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return folder;
}

std::wstring BrowseFile(HWND hwnd) {
    std::wstring file;
    IFileDialog* pfd = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD options;
        pfd->GetOptions(&options);
        pfd->SetOptions(options | FOS_FILEMUSTEXIST);
        if (SUCCEEDED(pfd->Show(hwnd))) {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    file = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return file;
}

// -------- BACKUP --------
size_t countTotalFiles() {
    size_t total = 0;
    for (auto& p : paths) {
        if (fs::exists(p)) {
            if (fs::is_directory(p))
                total += std::distance(fs::recursive_directory_iterator(p), fs::recursive_directory_iterator{});
            else
                total += 1;
        }
    }
    return total == 0 ? 1 : total;
}

void UpdateProgressBar() {
    int percent = static_cast<int>(currentFile * 100 / totalFiles);
    SendMessage(hProgressBar, PBM_SETPOS, percent, 0);
}

void copyFolder(const fs::path& src, const fs::path& dst) {
    if (!fs::exists(src)) return;
    for (auto& p : fs::recursive_directory_iterator(src)) {
        if (fs::is_directory(p)) continue;
        fs::path relative = fs::relative(p.path(), src);
        fs::path targetPath = dst / src.filename() / relative;
        fs::create_directories(targetPath.parent_path());
        try {
            fs::copy_file(p.path(), targetPath, fs::copy_options::overwrite_existing);
            backupLog << L"[OK] " << p.path().wstring() << L" -> " << targetPath.wstring() << L"\n";
        }
        catch (fs::filesystem_error& e) {
            backupLog << L"[ERROR] " << e.what() << L"\n";
        }
        ++currentFile;
        UpdateProgressBar();
    }
}

void copyFile(const fs::path& src, const fs::path& dst) {
    if (!fs::exists(src)) {
        backupLog << L"[WARN] File didn´t exists: " << src.wstring() << L"\n";
        return;
    }
    try {
        fs::copy_file(src, dst / src.filename(), fs::copy_options::overwrite_existing);
        backupLog << L"[OK] " << src.wstring() << L" -> " << (dst / src.filename()).wstring() << L"\n";
    }
    catch (fs::filesystem_error& e) {
        backupLog << L"[ERROR] " << e.what() << L"\n";
    }
    ++currentFile;
    UpdateProgressBar();
}

void RunBackup() {
    if (paths.empty()) return;

    auto t = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tm; localtime_s(&tm, &tt);
    wchar_t dateStr[11]; std::wcsftime(dateStr, sizeof(dateStr), L"%Y-%m-%d", &tm);

    fs::path root = fs::path(outputFolder) / (L"Backup_" + std::wstring(dateStr));
    fs::create_directories(root);
    backupRoot = root;

    backupLog.open(L"BackupLog.txt", std::ios::app);
    backupLog << L"===== Backup started " << dateStr << L" =====\n";

    currentFile = 0;
    totalFiles = countTotalFiles();

    for (auto& p : paths) {
        if (fs::is_directory(p)) copyFolder(p, root);
        else if (fs::is_regular_file(p)) copyFile(p, root);
    }

    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    backupLog << L"===== Backup finished =====\n\n";
    backupLog.close();

    MessageBox(NULL, L"Backup finished!", L"Backup", MB_OK);
}

// ------- DELETE PATH --------
bool DeletePath(const fs::path& path) {
    try {
        if (!fs::exists(path)) return false;
        fs::remove_all(path);
        return true;
    }
    catch (fs::filesystem_error& e) {
        MessageBox(nullptr, std::wstring(L"Error deleting path: " + path.wstring() + L"\n" + std::wstring(e.what(), e.what() + strlen(e.what()))).c_str(), L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

// -------- GUI --------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    switch (msg) {
    case WM_CREATE: {
        hListBox = CreateWindowEx(0, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | LBS_STANDARD,
            10, 10, 650, 200, hwnd, nullptr, nullptr, nullptr);
        SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButton1 = CreateWindow(L"BUTTON", L"Select Target Folder", WS_CHILD | WS_VISIBLE,
            670, 10, 150, 30, hwnd, (HMENU)4, nullptr, nullptr);
        SendMessage(hButton1, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButton2 = CreateWindow(L"BUTTON", L"Add folder for backup", WS_CHILD | WS_VISIBLE,
            670, 50, 150, 30, hwnd, (HMENU)1, nullptr, nullptr);
        SendMessage(hButton2, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButton3 = CreateWindow(L"BUTTON", L"Add file for backup", WS_CHILD | WS_VISIBLE,
            670, 90, 150, 30, hwnd, (HMENU)2, nullptr, nullptr);
        SendMessage(hButton3, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButton4 = CreateWindow(L"BUTTON", L"Start backup progress", WS_CHILD | WS_VISIBLE,
            670, 220, 150, 30, hwnd, (HMENU)3, nullptr, nullptr);
        SendMessage(hButton4, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hButtonDelete = CreateWindow( L"BUTTON", L"Delete selected path", WS_CHILD | WS_VISIBLE, 
            670, 130, 150, 30,  hwnd, (HMENU)5, nullptr, nullptr );
        SendMessage(hButtonDelete, WM_SETFONT, (WPARAM)hFont, TRUE);

        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS };
        InitCommonControlsEx(&icex);
        hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE, 10, 220, 650, 30, hwnd, nullptr, nullptr, nullptr);
        SendMessage(hProgressBar, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

        for (auto& p : paths) SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)p.c_str());
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: {
            std::wstring folder = BrowseFolder(hwnd);
            if (!folder.empty()) {
                paths.push_back(folder);
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)folder.c_str());
                SaveConfig();
            }
            break;
        }
        case 2: {
            std::wstring file = BrowseFile(hwnd);
            if (!file.empty()) {
                paths.push_back(file);
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)file.c_str());
                SaveConfig();
            }
            break;
        }
        case 3: {
            if (outputFolder.empty() || !fs::exists(outputFolder)) {
                MessageBox(hwnd, L"Target folder not reachable! Please select.", L"Error", MB_OK | MB_ICONERROR);
                break;
            }
            RunBackup();
            break;
        }
        case 4: {
            std::wstring folder = BrowseFolder(hwnd);
            if (!folder.empty()) {
                outputFolder = folder;
                SaveConfig();
                MessageBox(hwnd, L"Target folder saved!", L"Info", MB_OK);
            }
            break;
        }
        case 5: { // Delete selected path
            int selIndex = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (selIndex == LB_ERR) {
                MessageBox(hwnd, L"Please select a path in the list to delete.", L"Info", MB_OK);
                break;
            }

            wchar_t buffer[1024];
            SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)buffer);
            std::wstring pathToDelete = buffer;

            if (MessageBox(hwnd, (L"Are you sure you want to delete:\n" + pathToDelete).c_str(), L"Confirm Delete", MB_YESNO | MB_ICONWARNING) == IDYES) {
                if (DeletePath(pathToDelete)) {
                    SendMessage(hListBox, LB_DELETESTRING, selIndex, 0); // aus der ListBox entfernen
                    auto it = std::find(paths.begin(), paths.end(), pathToDelete);
                    if (it != paths.end()) paths.erase(it); // aus dem Pfad-Vektor entfernen
                    SaveConfig(); // Config aktualisieren
                }
            }
            break;
        }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// -------- MAIN --------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitialize(nullptr);
    LoadConfig();

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"BackupToolClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"BackupToolClass", L"Backup Tool", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 850, 300,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
