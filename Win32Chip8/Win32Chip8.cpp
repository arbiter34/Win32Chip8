// Win32Chip8.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Win32Chip8.h"
#include "Chip8.h"
#include <conio.h>
#include <Windows.h>
#include <shobjidl.h> 
#include <mmsystem.h>

#define MAX_LOADSTRING 100

#define HERTZ 500
#define TIMER_HERTZ 60

MMRESULT cpuTimerId;
MMRESULT decrementTimerId;
MMRESULT updateScreenId;

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
Chip8 chip8;
PWSTR romPath;
BOOL paused = false;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, HWND*);
PWSTR LoadFile();
BOOL Run();
BOOL Pause();
BOOL Stop(HWND hWnd);

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void CALLBACK cpuCycle(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void CALLBACK decrementTimers(UINT uTimerID, UINT uMsg,	DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void CALLBACK updateScreen(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
void OnKeyDown(WPARAM wParam);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	chip8 = Chip8();

	MSG msg;
	HACCEL hAccelTable;
	HWND hWnd;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WIN32CHIP8, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, &hWnd))
	{
		return FALSE;
	}

	chip8.SetHwnd(hWnd);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32CHIP8));

	// Main message loop:
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				if (msg.message == WM_KEYDOWN) {
					chip8.OnKeyDown(msg.wParam);
				}
				else if (msg.message == WM_KEYUP) {
					chip8.OnKeyUp(msg.wParam);
				}
				else if (msg.message == WM_QUIT) {
					break;
				}
				else {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32CHIP8));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_WIN32CHIP8);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND *hWnd)
{
   hInst = hInstance; // Store instance handle in our global variable

   *hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!*hWnd)
   {
      return FALSE;
   }

   ShowWindow(*hWnd, nCmdShow);
   UpdateWindow(*hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
#define SECOND_TIMER 250
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_RUN:
			Run();
			break;
		case IDM_PAUSE:
			Pause();
			break;
		case IDM_STOP:
			Stop(hWnd);
			break;
		case IDM_ABOUT:			
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_OPEN:
			romPath = LoadFile();
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		chip8.DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

PWSTR LoadFile() {
	IFileOpenDialog *pFileOpen;
	PWSTR pszFilePath = NULL;

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr))
	{
		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

				pItem->Release();
			}
		}
		pFileOpen->Release();
	}
	return pszFilePath;
}

BOOL Run() {
	if (romPath == NULL) {
		romPath = LoadFile();
	}
	chip8.initialize();
	chip8.loadGame(romPath);
	cpuTimerId = timeSetEvent(1000 / HERTZ, 0, (LPTIMECALLBACK)&cpuCycle, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
	decrementTimerId = timeSetEvent(1000 / TIMER_HERTZ, 0, (LPTIMECALLBACK)&decrementTimers, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
	updateScreenId = timeSetEvent(1000 / TIMER_HERTZ, 0, (LPTIMECALLBACK)&updateScreen, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
	paused = false;
	return true;
}

BOOL Pause() {
	if (!paused) {
		timeKillEvent(cpuTimerId);
		timeKillEvent(decrementTimerId);
		timeKillEvent(updateScreenId);
	}
	else {
		cpuTimerId = timeSetEvent(1000 / HERTZ, 0, (LPTIMECALLBACK)&cpuCycle, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
		decrementTimerId = timeSetEvent(1000 / TIMER_HERTZ, 0, (LPTIMECALLBACK)&decrementTimers, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
		updateScreenId = timeSetEvent(1000 / TIMER_HERTZ, 0, (LPTIMECALLBACK)&updateScreen, NULL, TIME_PERIODIC | TIME_CALLBACK_FUNCTION | TIME_KILL_SYNCHRONOUS);
	}
	paused = !paused;
	return true;
}

BOOL Stop(HWND hWnd) {
	timeKillEvent(cpuTimerId);
	timeKillEvent(decrementTimerId);
	timeKillEvent(updateScreenId);
	chip8.initialize();
	InvalidateRect(hWnd, NULL, FALSE);
	return true;
}



// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


void CALLBACK cpuCycle(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	chip8.emulateCycle();
}

void CALLBACK decrementTimers(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	chip8.decrementTimers();
}

void CALLBACK updateScreen(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	chip8.UpdateScreen();
}
