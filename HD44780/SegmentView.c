//-----------------------------------------------------------------------------*
// Copyright © 2023,2024 Nigel Winterbottom.
// FILE			: SegmentView.c
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Generic 5x8 Character Type LCD DisplaySide Stuff.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*


/* Implements *****************************************************************/
#include "SegmentView.h"

/* Dependencies ***************************************************************/
#include <assert.h>
#include <stdint.h>
#include <CommCtrl.h>

#include "Res/HD44780Res.h"



/* Local Types ****************************************************************/
/* Local Variables ************************************************************/
SegmentView   view;


/* (R,G,B) colors for the LCD */
static BYTE const c_onColor [3] = { 255U, 255U,   0U }; /* yellow */
static BYTE const c_offColor[3] = {  15U,  15U,  15U }; /* very dark gray */

/* Local functions ---------------------------------------------------------*/
static void segview_DrawCharacterCell (SegmentView *self, unsigned SEGnr, unsigned logical_line);


#if 0
static void Show (HWND hwnd, HDC hdc, int xText, int yText, int iMapMode, TCHAR * szMapMode)
{
	TCHAR szBuffer[60];
	RECT rect;
	SaveDC (hdc);
	SetMapMode (hdc, iMapMode);
	GetClientRect (hwnd, &rect);
	DPtoLP (hdc, (PPOINT)&rect, 2);
	RestoreDC (hdc, -1);
	TextOut (hdc, xText, yText, szBuffer,
	         wsprintf (szBuffer, TEXT ("%-20s %7d %7d %7d %7d"), szMapMode,
                       rect.left, rect.right, rect.top, rect.bottom));
}
#endif



/**----------------------------------------------------------------------------/
 * Initilaise the GUI Element.
 * ---------------------------------------------------------------------------*/
void segview_CreateSegmentView (SegmentView *self,  HINSTANCE hInstance, HWND hLCD)
{
	self->_hLCD = hLCD;
	self->_hbmBinaryDot = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_LCD_PIXELS));

	GetWindowRect(hLCD, &self->_glassRect);
	self->_hdcBinaryDot = CreateCompatibleDC(NULL);
	self->_hdcCharacter = CreateCompatibleDC(NULL);

	GetObject(self->_hbmBinaryDot, sizeof(BITMAP), &self->_bmBinaryDot);

	HDC hdc       = GetDC(hLCD);
	LONG bmWidth  = self->_bmBinaryDot.bmWidth / 256 * self->ndCharWidth;
	LONG bmHeight = self->_bmBinaryDot.bmHeight * 2; // NB: Only 11 dots of height is actually required.
	self->_hbmCharacter = CreateCompatibleBitmap(hdc, bmWidth, bmHeight); // The DC used must be a colour DC,
	GetObject(self->_hbmCharacter, sizeof(BITMAP), &self->_bmCharacter);

	SelectObject(self->_hdcBinaryDot, self->_hbmBinaryDot);
	SelectObject(self->_hdcCharacter, self->_hbmCharacter);

//	SendMessage(self->_hLCD, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)self->_hbmCharacter);

	ReleaseDC(hLCD, hdc);
}


/**----------------------------------------------------------------------------/
 * Destroy the GUI Element.
 * ---------------------------------------------------------------------------*/
void segview_DestroySegmentView (SegmentView *self)
{
	ReleaseDC(self->_hLCD, self->_hdcCharacter);
	ReleaseDC(self->_hLCD, self->_hdcBinaryDot);
	DeleteObject(self->_hbmCharacter);
	DeleteObject(self->_hbmBinaryDot);
}


/**----------------------------------------------------------------------------/
 * @note	Segment Memory is organised as one 16-bit integer per senment. Bit-0 representing the top pixel.
 * @param	segmentNr:0-Based index. (cf Controller Memory Address)
 * @param	pixels:   8 or 11 bits of vertical pixel data.
 * @param	logical_line: 0 or 1.
 * ---------------------------------------------------------------------------*/
void segview_StoreSegment (SegmentView *self, WORD segmentNr, WORD pixels, BYTE logical_line)
{
	WORD comnMask = self->controllerDuty == 11 ? 0x7ff : 0xff;
	if (logical_line == 1) {
		comnMask = 0xff00;
		pixels <<= 8;
	}

	/* Merge bits from two values according to a mask. https://graphics.stanford.edu/~seander/bithacks.html */
	self->_frameBuffer[segmentNr] = self->_frameBuffer[segmentNr] ^ ((pixels ^ self->_frameBuffer[segmentNr]) & comnMask);
}



/**
 * ---------------------------------------------------------------------------*/
void segview_FillGlassBackground (SegmentView *self)
{
	HDC hdc = GetDC(self->_hLCD);
	StretchBlt(hdc, 0, 0, (self->_glassRect.right - self->_glassRect.left), (self->_glassRect.bottom - self->_glassRect.top),
		self->_hdcBinaryDot, 0, 0, 1, 1, SRCCOPY);
	ReleaseDC(self->_hLCD, hdc);
}


/**
 * ---------------------------------------------------------------------------*/
void segview_UpdateSegmentView (SegmentView *self)
{
	for (int segmentNr = 0; segmentNr < self->nGlassSegments; segmentNr += self->ndCharWidth) {
		segview_DrawCharacterCell(self, segmentNr, 0);

		if (self->controllerDuty == 16)
			segview_DrawCharacterCell(self, segmentNr, 1);
	}
}



/**----------------------------------------------------------------------------/
 * @fn		segview_DrawCharacterCell
 * @brief	Draw one 5x7 / 5x10 Pixel Character.
 * @param	SEGnr: 0 - 39
 * @param	nSEGS: 1 - 8 (Usually 5)
 * @param	nCOMS: 1 - 16 (Usually 7)
 * ---------------------------------------------------------------------------*/
static void segview_DrawCharacterCell (SegmentView *self, unsigned SEGnr, unsigned logical_line)
{
	/// Build an intermediate BitMap by copying 5 columns (selected by FrameBuffer contents) from BinaryDotMatrix.
	/// Once complete, I StretchBlt this intermediate BitMap onto the Screen.

	assert(SEGnr < 150);

	SIZE  szBinMtx; // The Pixel Size of the Source BinaryDotMatrix Bitmap.
	POINT ptBinMtx; // The Pixel Location of the Source BinaryDotMatrix Bitmap Column.
	SIZE  szScreen; // The Pixel Size of the Glass Representation
	POINT ptScreen; // The Pixel Location of the Glass Representation

	szBinMtx.cx = self->_bmBinaryDot.bmWidth / 256; // The Width of one Pixel Column
	szBinMtx.cy = self->_bmBinaryDot.bmHeight;
	szScreen.cx = self->pxCharWidth;
	szScreen.cy = self->pxCharHeight;

	int charPosn     = SEGnr / self->ndCharWidth;
	int COMnr        = (logical_line == 0) ? 0 : 8;

	for (int dotCol = 0; dotCol < self->ndCharWidth; dotCol++) {

		ptBinMtx.x = szBinMtx.cx * ((self->_frameBuffer[SEGnr + dotCol] >> COMnr) & 0xff);
		ptBinMtx.y = 0;

		BitBlt(self->_hdcCharacter, (dotCol * szBinMtx.cx), ptBinMtx.y, szBinMtx.cx, szBinMtx.cy,
		       self->_hdcBinaryDot, ptBinMtx.x, ptBinMtx.y, SRCCOPY);
	}

	ptScreen.x = self->pxMargin_LHS + (charPosn % self->nGlasslineChars) * self->pxCharSpacing;
	ptScreen.y = self->pxMargin_TOP + (charPosn / self->nGlasslineChars) * self->pxLineSpacing * 2 + (logical_line * self->pxLineSpacing);

	HDC hdc = GetDC(self->_hLCD);
	SetStretchBltMode (hdc, STRETCH_HALFTONE);
	StretchBlt(hdc, ptScreen.x, ptScreen.y, szScreen.cx, szScreen.cy,
	           self->_hdcCharacter, 0, 0, self->_bmCharacter.bmWidth, (self->_bmBinaryDot.bmHeight * self->ndCharHeight / 8), SRCCOPY);

	ReleaseDC(self->_hLCD, hdc);
}


void segview_TestSegmentView (SegmentView *self)
{
	static const uint8_t FontMap[][8] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 4, 4, 4, 4, 4, 0, 4, 0 },
		{ 10, 10, 10, 0, 0, 0, 0, 0 },
		{ 10, 10, 31, 10, 31, 10, 10, 0 },
		{ 4, 15, 20, 14, 5, 30, 4, 0 },
		{ 24, 25, 2, 4, 8, 19, 3, 0 },
		{ 12, 18, 20, 8, 21, 18, 13, 0 },
		{ 12, 4, 8, 0, 0, 0, 0, 0 },
		{ 2, 4, 8, 8, 8, 4, 2, 0 },
		{ 8, 4, 2, 2, 2, 4, 8, 0 },
		{ 0, 4, 21, 14, 21, 4, 0, 0 },
		{ 0, 4, 4, 31, 4, 4, 0, 0 },
		{ 0, 0, 0, 0, 12, 4, 8, 0 },
		{ 0, 0, 0, 31, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 12, 12, 0 },
		{ 0, 1, 2, 4, 8, 16, 0, 0 },
		{ 14, 17, 19, 21, 25, 17, 14, 0 },
		{ 4, 12, 4, 4, 4, 4, 14, 0 },
		{ 14, 17, 1, 2, 4, 8, 31, 0 },
		{ 31, 2, 4, 2, 1, 17, 14, 0 },
		{ 2, 6, 10, 18, 31, 2, 2, 0 },
		{ 31, 16, 30, 1, 1, 17, 14, 0 },
		{ 6, 8, 16, 30, 17, 17, 14, 0 },
		{ 31, 1, 2, 4, 8, 8, 8, 0 },
		{ 14, 17, 17, 14, 17, 17, 14, 0 },
		{ 14, 17, 17, 15, 1, 2, 12, 0 },
		{ 0, 12, 12, 0, 12, 12, 0, 0 },
		{ 0, 12, 12, 0, 12, 4, 8, 0 },
		{ 2, 4, 8, 16, 8, 4, 2, 0 },
		{ 0, 0, 31, 0, 31, 0, 0, 0 },
		{ 16, 8, 4, 2, 4, 8, 16, 0 },
		{ 14, 17, 1, 2, 4, 0, 4, 0 },
		{ 14, 17, 1, 13, 21, 21, 14, 0 },
		{ 14, 17, 17, 17, 31, 17, 17, 0 },
		{ 30, 17, 17, 30, 17, 17, 30, 0 },
		{ 14, 17, 16, 16, 16, 17, 14, 0 },
		{ 30, 17, 17, 17, 17, 17, 30, 0 },
		{ 31, 16, 16, 30, 16, 16, 31, 0 },
		{ 31, 16, 16, 30, 16, 16, 16, 0 },
		{ 14, 17, 16, 23, 17, 17, 15, 0 },
		{ 17, 17, 17, 31, 17, 17, 17, 0 },
		{ 14, 4, 4, 4, 4, 4, 14, 0 },
		{ 7, 2, 2, 2, 2, 18, 12, 0 },
		{ 17, 18, 20, 24, 20, 18, 17, 0 },
		{ 16, 16, 16, 16, 16, 16, 31, 0 },
		{ 17, 27, 21, 21, 17, 17, 17, 0 },
		{ 17, 17, 25, 21, 19, 17, 17, 0 },
		{ 14, 17, 17, 17, 17, 17, 14, 0 },
		{ 30, 17, 17, 30, 16, 16, 16, 0 },
		{ 14, 17, 17, 17, 21, 18, 13, 0 },
		{ 30, 17, 17, 30, 20, 18, 17, 0 },
		{ 15, 16, 16, 14, 1, 1, 30, 0 },
		{ 31, 4, 4, 4, 4, 4, 4, 0 },
		{ 17, 17, 17, 17, 17, 17, 14, 0 },
		{ 17, 17, 17, 17, 17, 10, 4, 0 },
		{ 17, 17, 17, 21, 21, 21, 10, 0 },
		{ 17, 17, 10, 4, 10, 17, 17, 0 },
		{ 17, 17, 17, 10, 4, 4, 4, 0 },
		{ 31, 1, 2, 4, 8, 16, 31, 0 },
		{ 14, 8, 8, 8, 8, 8, 14, 0 },
		{ 17, 10, 31, 4, 31, 4, 4, 0 },
		{ 14, 2, 2, 2, 2, 2, 14, 0 },
		{ 4, 10, 17, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 31, 0 },
		{ 8, 4, 2, 0, 0, 0, 0, 0 },
		{ 0, 0, 14, 1, 15, 17, 15, 0 },
		{ 16, 16, 22, 25, 17, 17, 30, 0 },
		{ 0, 0, 14, 16, 16, 17, 14, 0 },
		{ 1, 1, 13, 19, 17, 17, 15, 0 },
		{ 0, 0, 14, 17, 31, 16, 14, 0 },
		{ 6, 9, 8, 28, 8, 8, 8, 0 },
		{ 0, 0, 15, 17, 15, 1, 14, 0 },
		{ 16, 16, 22, 25, 17, 17, 17, 0 },
		{ 4, 0, 12, 4, 4, 4, 14, 0 },
		{ 2, 6, 2, 2, 2, 18, 12, 0 },
		{ 16, 16, 18, 20, 24, 20, 18, 0 },
		{ 12, 4, 4, 4, 4, 4, 14, 0 },
		{ 0, 0, 26, 21, 21, 17, 17, 0 },
		{ 0, 0, 22, 25, 17, 17, 17, 0 },
		{ 0, 0, 14, 17, 17, 17, 14, 0 },
		{ 0, 0, 30, 17, 30, 16, 16, 0 },
		{ 0, 0, 13, 19, 15, 1, 1, 0 },
		{ 0, 0, 22, 25, 16, 16, 16, 0 },
		{ 0, 0, 15, 16, 14, 1, 30, 0 },
		{ 8, 8, 28, 8, 8, 9, 6, 0 },
		{ 0, 0, 17, 17, 17, 19, 13, 0 },
		{ 0, 0, 17, 17, 17, 10, 4, 0 },
		{ 0, 0, 17, 17, 21, 21, 10, 0 },
		{ 0, 0, 17, 10, 4, 10, 17, 0 },
		{ 0, 0, 17, 17, 15, 1, 14, 0 },
		{ 0, 0, 31, 2, 4, 8, 31, 0 },
		{ 2, 4, 4, 8, 4, 4, 2, 0 },
		{ 4, 4, 4, 4, 4, 4, 4, 0 },
		{ 8, 4, 4, 2, 4, 4, 8, 0 },
		{ 0, 4, 2, 31, 2, 4, 0, 0 },
		{ 0, 4, 8, 31, 8, 4, 0, 0 },
	};


	/* Just as a Test - lets fill the framebuffer with some character patterns */
	for (char charPos = 0; charPos < 80; charPos++) {

		WORD segmentNr = charPos * 5;
		uint8_t displayData1 = charPos + 'A';
		uint8_t displayData2 = charPos + 'a';

		for (WORD font_bm = 1 << 4; font_bm; font_bm >>= 1) {

			WORD pixels1 = 0, pixels2 = 0;
			WORD pixel_bm = 1; // Pixel BitMask

			for (int fontRow = 0; fontRow < 8; fontRow++, pixel_bm <<= 1) {
				pixels1 |= (FontMap[displayData1 - ' '][fontRow] & font_bm) ? pixel_bm : 0;
				pixels2 |= (FontMap[displayData2 - ' '][fontRow] & font_bm) ? pixel_bm : 0;
			}
			segview_StoreSegment(self, segmentNr, pixels1, 0);
			segview_StoreSegment(self, segmentNr, pixels2, 1);
			segmentNr++;
		}
	}
	segview_UpdateSegmentView (self);
}
