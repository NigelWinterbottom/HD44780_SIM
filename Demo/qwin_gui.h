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

#ifndef qwin_gui_h
#define qwin_gui_h

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  /* Win32 API */

#ifdef __cplusplus
extern "C" {
#endif

/** Create the Custom Dialog hosting the embedded front panel ..............*/
ATOM RegisterDialogClass (HINSTANCE hInstance, WNDPROC lpfnWndProc, LPCTSTR lpWndClass, int idIcon, int idcMenu);
BOOL InitInstance        (HINSTANCE hInstance, int iDlg, HWND hParent, int nCmdShow);


/* OwnerDrawnButton "class" ................................................*/
typedef struct {
    UINT    itemID;
    HBITMAP hBitmapUp;
    HBITMAP hBitmapDown;
    HCURSOR hCursor;
    int     isDepressed;
} OwnerDrawnButton;

enum OwnerDrawnButtonAction {
    BTN_NOACTION,
    BTN_PAINTED,
    BTN_DEPRESSED,
    BTN_RELEASED
};

void ODB_Init (OwnerDrawnButton * self,
                            UINT itemID,
                            HBITMAP hBitmapUp, HBITMAP hBitmapDwn,
                            HCURSOR hCursor);

void ODB_Xtor(OwnerDrawnButton * self);

enum OwnerDrawnButtonAction ODB_draw (
                                   OwnerDrawnButton * self,
                                   LPDRAWITEMSTRUCT lpdis);
void ODB_Set         (OwnerDrawnButton * self, int isDepressed);
BOOL ODB_IsDepressed (OwnerDrawnButton const * self);

/* GraphicDisplay "class" for drawing graphic displays
* with up to 24-bit color...
*/
typedef struct {
    HDC     src_hDC;
    int     src_width;
    int     src_height;
    HDC     dst_hDC;
    int     dst_width;
    int     dst_height;
    HWND    hItem;
    HBITMAP hBitmap;
    BYTE   *bits;
    BYTE    bgColor[3];
} GraphicDisplay;

void GraphicDisplay_init  (GraphicDisplay * self,
                           UINT width,  UINT height,
                           UINT itemID, BYTE const bgColor[3]);
void GraphicDisplay_xtor  (GraphicDisplay * self);
void GraphicDisplay_clear (GraphicDisplay * self);
void GraphicDisplay_redraw(GraphicDisplay * self);
#define GraphicDisplay_setPixel(me_, x_, y_, color_) do { \
    BYTE *pixelRGB = &(me_)->bits[3*((x_) \
          + (me_)->src_width * ((me_)->src_height - 1U - (y_)))]; \
    pixelRGB[0] = (color_)[0]; \
    pixelRGB[1] = (color_)[1]; \
    pixelRGB[2] = (color_)[2]; \
} while (0)

#define GraphicDisplay_clearPixel(me_, x_, y_) do { \
    BYTE *pixelRGB = &(me_)->bits[3*((x_) \
          + (me_)->src_width * ((me_)->src_height - 1U - (y_)))]; \
    pixelRGB[0] = (me_)->bgColor[0]; \
    pixelRGB[1] = (me_)->bgColor[1]; \
    pixelRGB[2] = (me_)->bgColor[2]; \
} while (0)

/* SegmentDisplay "class" for drawing segment displays, LEDs, etc...........*/
typedef struct {
    HWND    *hSegment;    /* array of segment controls */
    UINT     segmentNum;  /* number of segments */
    HBITMAP *hBitmap;     /* array of bitmap handles */
    UINT     bitmapNum;   /* number of bitmaps */
} SegmentDisplay;

void SegmentDisplay_init       (SegmentDisplay * self, UINT segNum, UINT bitmapNum);
void SegmentDisplay_xtor       (SegmentDisplay * self);
BOOL SegmentDisplay_initSegment(SegmentDisplay * self, UINT segmentNum, UINT segmentID);
BOOL SegmentDisplay_initBitmap (SegmentDisplay * self, UINT bitmapNum, HBITMAP hBitmap);
BOOL SegmentDisplay_setSegment (SegmentDisplay * self, UINT segmentNum, UINT bitmapNum);

/*..........................................................................*/
typedef struct {

	UINT    itemID;
	HWND    hItem;
	INT     nrItems;
} ComboSelector;
HWND ComboSelector_create (ComboSelector * self, HWND hParent, UINT id);
void ComboSelector_init   (ComboSelector * self,  LPCTSTR const *s, UINT nrItems);


/* useful helper functions .................................................*/
void DrawBitmap(HDC hdc, HBITMAP hBitmap, int xStart, int yStart);

#ifdef __cplusplus
}
#endif

#endif /* qwin_gui_h */
