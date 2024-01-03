/**
* @file
* @brief QWIN GUI facilities for building realistic embedded front panels
* @ingroup qwin
* @cond
******************************************************************************
* Last Updated for Version: 5.9.0
* Date of the Last Update:  2017-04-21
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* https://state-machine.com
* mailto:info@state-machine.com
******************************************************************************
* @endcond
*/


/* Implements *****************************************************************/
#include "qwin_gui.h"

#include <stdlib.h>
#include <CommCtrl.h>

#include "Resource.h"

static HINSTANCE l_hInst;
static HWND l_hWnd;
static HDC  l_hDC;

/*--------------------------------------------------------------------------*/

/**
 * @fn		RegisterDialogClass
 * ---------------------------------------------------------------------------*/
ATOM RegisterDialogClass (HINSTANCE hInst, WNDPROC lpfnWndProc, LPCTSTR lpWndClass, int idIcon, int idMenu)
{
    WNDCLASSEX wcex;

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = lpfnWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = DLGWINDOWEXTRA;
    wcex.hInstance     = hInst;
    wcex.hIcon         = idIcon ? LoadIcon(hInst, MAKEINTRESOURCE(idIcon)) : NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = idMenu ? MAKEINTRESOURCE(idMenu) : NULL;
    wcex.lpszClassName = lpWndClass;
    wcex.hIconSm       = NULL;

    return RegisterClassEx(&wcex);
}


/**
 * @fn		InitInstance
 * @brief	Save instance handle and create main window
 * @note	In this function, we save the instance handle in a global variable
 *			and create then display the main program window.
 * ---------------------------------------------------------------------------*/
BOOL InitInstance (HINSTANCE hInstance, int iDlg, HWND hParent, int nCmdShow)
{
	l_hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(iDlg), hParent, NULL);

	if (hWnd == NULL) {
		return FALSE;
	}

	l_hDC = GetDC(l_hWnd); /* the DC for the client area of the window */

	/* NOTE: WM_INITDIALOG provides stimulus for initializing dialog controls.
	* Dialog box procedures typically use this message to initialize controls
	* and carry out any other initialization tasks that affect the appearance
	* of the dialog box.                                                   */
	SendMessage(hWnd, WM_INITDIALOG, 0, 0);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}



/*--------------------------------------------------------------------------*/
void ODB_Init(OwnerDrawnButton * self,
                           UINT itemID,
                           HBITMAP hBitmapUp, HBITMAP hBitmapDwn,
                           HCURSOR hCursor)
{
    self->itemID      = itemID;
    self->hBitmapUp   = hBitmapUp;
    self->hBitmapDown = hBitmapDwn;
    self->hCursor     = hCursor;
    self->isDepressed = 0;
}
/*..........................................................................*/
void ODB_Xtor(OwnerDrawnButton * self) {
    DeleteObject(self->hBitmapUp);
    DeleteObject(self->hBitmapDown);
    DeleteObject(self->hCursor);
}
/*..........................................................................*/
enum OwnerDrawnButtonAction ODB_draw (OwnerDrawnButton * self, LPDRAWITEMSTRUCT lpdis)
{
    enum OwnerDrawnButtonAction action = BTN_NOACTION;

    if ((lpdis->itemAction & ODA_DRAWENTIRE) != 0U) {
        if (self->hCursor != NULL) {
           SetClassLongPtr(lpdis->hwndItem,
                           GCLP_HCURSOR, (LONG_PTR)self->hCursor);
           self->hCursor = NULL; /* mark the cursor set */
        }
        DrawBitmap(lpdis->hDC, self->hBitmapUp,
                   lpdis->rcItem.left, lpdis->rcItem.top);
        self->isDepressed = 0;
        action = BTN_PAINTED;
    }
    else if ((lpdis->itemAction & ODA_SELECT) != 0U) {
        if ((lpdis->itemState & ODS_SELECTED) != 0U) {
            DrawBitmap(lpdis->hDC, self->hBitmapDown,
                       lpdis->rcItem.left, lpdis->rcItem.top);
            self->isDepressed = !0;
            action = BTN_DEPRESSED;
        }
        else {
			/* NOTE: The bitmap for button "UP" look WAS drawn in the ODA_DRAWENTIRE action,
				however this had to stop when I added the ComboBox.
			*/
			DrawBitmap(lpdis->hDC, self->hBitmapUp,
				lpdis->rcItem.left, lpdis->rcItem.top);
			
			self->isDepressed = 0;
            action = BTN_RELEASED;
        }
    }
    return action;
}
/*..........................................................................*/
void ODB_Set(OwnerDrawnButton * self, int isDepressed)
{
    if (self->isDepressed != isDepressed) {
        HWND hItem = GetDlgItem(l_hWnd, self->itemID);
        self->isDepressed = isDepressed;
        if (isDepressed) {
            DrawBitmap(GetDC(hItem), self->hBitmapDown, 0, 0);
        }
        else {
            DrawBitmap(GetDC(hItem), self->hBitmapUp, 0, 0);
        }
    }
}
/*..........................................................................*/
BOOL ODB_IsDepressed(OwnerDrawnButton const * self) {
    return self->isDepressed;
}

/*--------------------------------------------------------------------------*/
void GraphicDisplay_init(GraphicDisplay * self,
                         UINT width,  UINT height,
                         UINT itemID, BYTE const bgColor[3])
{
    BITMAPINFO bi24BitInfo;
    RECT rect;

    self->src_width  = width;
    self->src_height = height;

    self->hItem      = GetDlgItem(l_hWnd, itemID);
    self->dst_hDC    = GetDC(self->hItem);
    GetWindowRect(self->hItem, &rect);
    self->dst_width  = rect.right - rect.left;
    self->dst_height = rect.bottom - rect.top;

    self->bgColor[0] = bgColor[0];
    self->bgColor[1] = bgColor[1];
    self->bgColor[2] = bgColor[2];

    bi24BitInfo.bmiHeader.biBitCount    = 3U*8U;  /* 3 RGB bytes */
    bi24BitInfo.bmiHeader.biCompression = BI_RGB; /* RGB color */
    bi24BitInfo.bmiHeader.biPlanes      = 1U;
    bi24BitInfo.bmiHeader.biSize        = sizeof(bi24BitInfo.bmiHeader);
    bi24BitInfo.bmiHeader.biWidth       = self->src_width;
    bi24BitInfo.bmiHeader.biHeight      = self->src_height;

    self->src_hDC = CreateCompatibleDC(self->dst_hDC);
    self->hBitmap = CreateDIBSection(self->src_hDC, &bi24BitInfo, DIB_RGB_COLORS,
                                    (void **)&self->bits, 0, 0);
	if (self->hBitmap) {
		SelectObject(self->src_hDC, self->hBitmap);
		GraphicDisplay_clear(self);
		GraphicDisplay_redraw(self);
	}
}
/*..........................................................................*/
void GraphicDisplay_xtor(GraphicDisplay * self)
{
    DeleteDC(self->src_hDC);
    DeleteObject(self->hBitmap);
    OutputDebugString("GraphicDisplay_xtor\n");
}
/*..........................................................................*/
void GraphicDisplay_clear (GraphicDisplay * self)
{
    UINT n;
    BYTE r = self->bgColor[0];
    BYTE g = self->bgColor[1];
    BYTE b = self->bgColor[2];
    BYTE *bits = self->bits;

    for (n = self->src_width * self->src_height; n != 0U; --n, bits += 3) {
        bits[0] = b;
        bits[1] = g;
        bits[2] = r;
    }
}
/*..........................................................................*/
void GraphicDisplay_redraw (GraphicDisplay * self)
{
    StretchBlt(self->dst_hDC, 0, 0, self->dst_width, self->dst_height,
               self->src_hDC, 0, 0, self->src_width, self->src_height, SRCCOPY);
}

/* SegmentDisplay ----------------------------------------------------------*/
void SegmentDisplay_init (SegmentDisplay * self, UINT segmentNum, UINT bitmapNum)
{
    UINT n;

    self->hSegment = (HWND *)calloc(segmentNum, sizeof(HWND));
	if (self->hSegment == NULL)
		return;

    self->segmentNum = segmentNum;
    for (n = 0U; n < segmentNum; ++n) {
        self->hSegment[n] = NULL;
    }
    self->hBitmap = (HBITMAP *)calloc(bitmapNum, sizeof(HBITMAP));
	if (self->hBitmap == NULL) {
		free(self->hSegment);
		return;
	}

    self->bitmapNum = bitmapNum;
    for (n = 0U; n < bitmapNum; ++n) {
        self->hBitmap[n] = NULL;
    }
}
/*..........................................................................*/
void SegmentDisplay_xtor (SegmentDisplay * self)
{
    UINT n;

    for (n = 0U; n < self->segmentNum; ++n) {
        DeleteObject(self->hSegment[n]);
    }

    for (n = 0U; n < self->bitmapNum; ++n) {
        DeleteObject(self->hBitmap[n]);
    }

	free(self->hBitmap);
	free(self->hSegment);
}
/*..........................................................................*/
BOOL SegmentDisplay_initSegment(SegmentDisplay * self, UINT segmentNum, UINT segmentID)
{
    if (segmentNum < self->segmentNum) {
        self->hSegment[segmentNum] = GetDlgItem(l_hWnd, segmentID);
        return self->hSegment[segmentNum] != NULL;
    }
    else {
        return FALSE;
    }
}
/*..........................................................................*/
BOOL SegmentDisplay_initBitmap(SegmentDisplay * self, UINT bitmapNum, HBITMAP hBitmap)
{
    if ((bitmapNum < self->bitmapNum) && (hBitmap != NULL)) {
        self->hBitmap[bitmapNum] = hBitmap;
        return TRUE;
    }
    else {
        return FALSE;
    }
}
/*..........................................................................*/
BOOL SegmentDisplay_setSegment(SegmentDisplay * self, UINT segmentNum, UINT bitmapNum)
{
    if ((segmentNum < self->segmentNum) && (bitmapNum < self->bitmapNum)) {
        SendMessage(self->hSegment[segmentNum],
		            STM_SETIMAGE,
		            IMAGE_BITMAP,
		            (LPARAM)self->hBitmap[bitmapNum]);
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/*--------------------------------------------------------------------------*/
/* DrawBitmap() function adapted from the book "Programming Windows" by
* Charles Petzold.
*/
void DrawBitmap (HDC hdc, HBITMAP hBitmap, int xStart, int yStart)
{
    BITMAP bm;
    POINT  ptSize, ptOrg;
    HDC    hdcMem = CreateCompatibleDC(hdc);

    SelectObject(hdcMem, hBitmap);
    SetMapMode(hdcMem, GetMapMode(hdc));

    GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.x = 0;
    ptOrg.y = 0;
    DPtoLP(hdcMem, &ptOrg, 1);

    BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y,
           hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);
    DeleteDC(hdcMem);
}


/**
/*--------------------------------------------------------------------------*/

/**
 * @fn		Draw_GradientDemo
 * ---------------------------------------------------------------------------*/
void GradientDemo (HDC hdc, PRECT pLCDrect)
{
#	define NR_BANDS 16
	RECT rect;
	HBRUSH hBrush;

	rect.left = pLCDrect->left;
	rect.top = pLCDrect->top;
	rect.right = rect.left + (pLCDrect->right - pLCDrect->left) / NR_BANDS;
	rect.bottom = pLCDrect->bottom;

	hBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));

	// Draw the fountain of grays
	for (UINT i = 0; i < NR_BANDS; i++) {
		rect.left = rect.right;
		rect.right += (pLCDrect->right - pLCDrect->left) / NR_BANDS;
		COLORREF bandColour = RGB (min (255, 255 * i / (NR_BANDS - 1)),
		                           min (255, 255 * i / (NR_BANDS - 1)),
		                           min (255, 255 * i / (NR_BANDS - 1)));
		SetDCBrushColor(hdc, bandColour);
//		PatBlt(hdc, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top), PATCOPY);
		FillRect (hdc, &rect, GetCurrentObject(hdc, OBJ_BRUSH));
	}
}


/*--------------------------------------------------------------------------*/

HWND ComboSelector_create (ComboSelector * self, HWND hParent, UINT id)
{
	self->itemID = id;
	self->hItem = GetDlgItem(hParent, id);
//		CreateWindow(WC_COMBOBOX, TEXT(""),
//		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_VISIBLE,
//		xpos, ypos, width, height,
//		hParent, (HMENU)id, l_hInst, NULL);

	return self->hItem;
}

void ComboSelector_init (ComboSelector * self, LPCTSTR const *s, UINT nrItems)
{
	self->nrItems = nrItems;

	for (UINT i = 0; i < nrItems; i++, s++) {
		SendMessage(self->hItem, CB_ADDSTRING, 0, (LPARAM)*s);
	}
	SendMessage(self->hItem, CB_SETCURSEL, 0, 0);
}


#if 0
/*--------------------------------------------------------------------------*/
/* Lets Look into this Subclassing Stuff Sometime                           */
/*--------------------------------------------------------------------------*/

// https://microsoft.public.win32.programmer.ui.narkive.com/cf3suaPo/cbutton-with-no-cs-dblclks-style-i-don-t-want-bn-doubleclicked-messages

char *szNoDblClickButtonClass = "NODBLCLICK_BUTTON";

BOOL CNoDlbClickButton::PreCreateWindow(CREATESTRUCT& cs)
{
	// Get the old class. If it doesn't exist, then return FALSE.
	WNDCLASS wndclass;

	if (!GetClassInfo(cs.hInstance, cs.lpszClass, &wndclass))
		return FALSE;

	// Compose a new class name
	wndclass.lpszClassName = szNoDblClickButtonClass;

	// Remove DblClick style
	wndclass.style &= ~CS_DBLCLKS;

	// Register the new class (if not already registered)
	if (!AfxRegisterClass(&wndclass))
	{
		return FALSE;
	}

	// Assign new defined class
	cs.lpszClass = wndclass.lpszClassName;

	return CButton::PreCreateWindow(cs);
}



https://www.codeproject.com/articles/19913/owner-draw-icon-buttons-in-plain-c-no-mfc
// ...
// that is why controls are created with dimensions zero:

// create a dummy button so I can change the button class style
hDummyButton = CreateWindow(_T("button"), NULL,
	WS_CHILD | BS_OWNERDRAW,
	0, 0, 0, 0,
	hWnd,
	(HMENU)0,
	hInst,
	NULL);
if (NULL == hDummyButton) {
	_stprintf(s, _T("! hDummyButton NULL"));
	myWriteToLog(s);
}
// we do not need the double clicks to be sent...
// AFAIK, set for owner drawn
// buttons automatically by the system)
dwValue = GetClassLong(hDummyButton, GCL_STYLE);
SetClassLong(hDummyButton, dwValue & ~CS_DBLCLKS);

// 1. First we create the background,
// ...

#endif
