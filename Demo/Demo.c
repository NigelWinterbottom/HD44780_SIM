//-----------------------------------------------------------------------------*
// Copyright © 2023 Nigel Winterbottom.
// Instrument	: General Windows Project
// FILE			: Demo.c (HD44780_SIM)
// ENCODING		: ISO 8859-1
// DESCRIPTION	: LCD WIN32 Main Program.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*

/* Dependencies ***************************************************************/
#include "framework.h"
#include "Demo.h"
#include "qwin_gui.h"
#include "../HD44780/SegmentView.h"

#define MAX_LOADSTRING	96
#define IDC_TIMER_25	1 /* My 25ms Windows Timer */

// Global Variables:
//HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


/* local variables ---------------------------------------------------------*/
static HINSTANCE l_hInst;   /* this application instance */
static HWND      l_hWnd;    /* main window handle */
static LPSTR     l_cmdLine; /* the command line string */
static HBITMAP   l_hbmBinaryDot;

static OwnerDrawnButton l_BtnStart;
static OwnerDrawnButton l_BtnSetup, l_BtnUp, l_BtnDown, l_BtnEnter;

static BOOL      l_PIC_IRQ_Enabled;
static unsigned  l_PerfTicks_1us;
static void (*keyboard_callback) (void);
static void (*timer_25_callback) (void);

static SegmentView lcdview = {

//	.idbPattern256 = IDB_LCD_PIXELS,
//	.idcLCDcontrol = IDC_CHARLCD,

	.controllerDuty = 16,

	/* Basic Glass Layout. There must be some uniformity howeever. */
	.nGlasslines     = 2,
	.nGlasslineChars = 20,
	.ndCharWidth     = 5,
	.ndCharHeight    = 7,
	.nGlassSegments  = (20 * 5),

	/* Details for Positioning each Character Cell */
	.pxMargin_LHS  = 4,
	.pxMargin_TOP  = 4,
	.pxCharWidth   = 14,	// Width of Character (5 Columns) (Monitor Pixels)
	.pxCharHeight  = 28,	// Height of Character (7, 8, 11 Rows) (Monitor Pixels)
	.pxCharSpacing = 20,	// Spacing between Character Centres.
	.pxLineSpacing = 48,	// Spacing between Line Centres.

};



// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About  (HWND, UINT, WPARAM, LPARAM);

static DWORD WINAPI appThread (LPVOID par);
static DWORD WINAPI irqThread (LPVOID par);


/**
 * @fn	WinMain
 * -------------------------------------------------------------------------- - */
int APIENTRY WinMain (_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPTSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
	MSG  msg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SIMPLELCD, szWindowClass, MAX_LOADSTRING);
	l_hbmBinaryDot = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_LCD_PIXELS));

	// Perform application initialization:
	RegisterDialogClass(hInstance, &WndProc, TEXT("LCDDEMO"), IDI_SIMPLELCD, IDC_SIMPLELCD);

    if (!(InitInstance(hInstance, IDD_APPDIALOG, NULL, nCmdShow))) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLELCD));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	DeleteObject(hAccelTable);
	DeleteObject(l_hbmBinaryDot);
	return (int) msg.wParam;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {

	case VK_BACK: break;

	/* Perform initialization after all child windows have been created */
	case WM_INITDIALOG: {

		segview_CreateSegmentView(&lcdview, GetDlgItem(hWnd, IDC_CHARLCD), l_hbmBinaryDot);
		segview_TestSegmentView (&lcdview);

		SetTimer(hWnd, IDC_TIMER_25, 25, NULL); /* Create TIMER_25mS */

#		if 0
		/* --> Spawn the Interruot thread */
		CreateThread(NULL, 0, &irqThread, NULL, 0, NULL);
#		endif

		/* --> Spawn the application thread to run main_gui() */
		CreateThread(NULL, 0, &appThread, NULL, 0, NULL);
		return 0;
	}

	case WM_COMMAND: {
		int idCtrl = LOWORD(wParam);

		/* Parse the menu selections: */
		switch (idCtrl) {
		case IDM_ABOUT:
			DialogBox(l_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		
		case IDCANCEL:
		case IDM_EXIT:
			segview_DestroySegmentView(&lcdview);
			DestroyWindow(hWnd);
			break;
		case IDC_SETUP_BTN:
			segview_UpdateSegmentView(&lcdview);
			break;

		case IDC_START_BTN:
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		segview_FillGlassBackground(&lcdview);
		EndPaint(hWnd, &ps);
	}
	break;

	/* OwnerDrawn Buttons */
	case WM_DRAWITEM:
	{
		enum OwnerDrawnButtonAction drawAction = BTN_NOACTION;

		LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

		switch (pdis->CtlID) {
		case IDC_SETUP_BTN: drawAction = ODB_draw(&l_BtnSetup, pdis);  break;
		case IDC_START_BTN: drawAction = ODB_draw(&l_BtnStart, pdis);  break;
		case IDOK:          drawAction = ODB_draw(&l_BtnEnter, pdis);  break;
		case IDC_UP_BTN:    drawAction = ODB_draw(&l_BtnUp,    pdis);  break;
		case IDC_DOWN_BTN:
			drawAction = ODB_draw(&l_BtnDown, pdis);
			break;
		}
		if ((drawAction == BTN_DEPRESSED) || (drawAction == BTN_RELEASED)) {
			if (keyboard_callback)
				keyboard_callback();
		}

		return 0;
	}

	case WM_DESTROY:
		KillTimer(hWnd, IDC_TIMER_25);
		PostQuitMessage(0);
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}



/*..........................................................................*/
/* thread function for running the application main_gui() */
static DWORD WINAPI appThread (LPVOID par) {
    (void)par;         /* unused parameter */
    return 0; /* run the application */
//	return main_gui(); /* run the application */
}

/* Thread function for running the 25ms Interrupt */
/*..........................................................................*/
static DWORD WINAPI irqThread (LPVOID par) {
	(void)par;			/* unused parameter */
	for (;;) {
		if (l_PIC_IRQ_Enabled) {
//			Timer3Interrupt();
		}
	}
	return 0;
}




// Message handler for about box.
INT_PTR CALLBACK About (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
