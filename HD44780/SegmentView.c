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



/**
 * Initilaise the GUI Element.
 * ---------------------------------------------------------------------------*/
void segview_CreateSegmentView (SegmentView *self, HWND hLCD, HBITMAP hbmBinarDot)
{
	self->_hLCD = hLCD;
	self->_hbmBinaryDot = hbmBinarDot;

	GetWindowRect(hLCD, &self->_glassRect);

	self->_hdcBinaryDot = CreateCompatibleDC(NULL);
	self->_hdcCharacter = CreateCompatibleDC(NULL);

	GetObject(self->_hbmBinaryDot, sizeof(BITMAP), &self->_bmBinaryDot);
	LONG bmWidth  = self->_bmBinaryDot.bmWidth / 256 * self->ndCharWidth;
	LONG bmHeight = self->_bmBinaryDot.bmHeight * 2; // NB: Only 11 dots of height is actually required.

	HDC hdc = GetDC(hLCD);
	self->_hbmCharacter = CreateCompatibleBitmap(hdc, bmWidth, bmHeight); // The DC used must be a colour DC,
	GetObject(self->_hbmCharacter, sizeof(BITMAP), &self->_bmCharacter);

	SelectObject(self->_hdcBinaryDot, self->_hbmBinaryDot);
	SelectObject(self->_hdcCharacter, self->_hbmCharacter);

//	SendMessage(self->_hLCD, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)self->_hbmCharacter);

	ReleaseDC(hLCD, hdc);
}


/**
 * Destroy the GUI Element.
 * ---------------------------------------------------------------------------*/
void segview_DestroySegmentView (SegmentView *self)
{
	ReleaseDC(self->_hLCD, self->_hdcCharacter);
	ReleaseDC(self->_hLCD, self->_hdcBinaryDot);
	DeleteObject(self->_hbmCharacter);
}


/**
 * @param	charPosn: 0-Based index. (cf Controller Memory Address)
 * @param	font:     8 or 11 bytes corresponding to Number of Character Dot Rows
 * ---------------------------------------------------------------------------*/
void segview_StoreCharacter (SegmentView *self, uint16_t segmentNr, uint8_t logical_line, const uint8_t font[11])
{
	/* Calculate Number of Dot Rows for consideration in the character */
	int nCharRows = (self->controllerDuty == 11) ? 11 : 8;

	/* The target base COMMON bit is going to be 0 or 8 */
	int baseComn = (self->controllerDuty == 16) ? ((logical_line == 0) ? 0 : 8) : 0;

	for (int fontRow = 0; fontRow < nCharRows; fontRow++) {

		WORD comnMask = 1 << (baseComn + fontRow);
		WORD fontMask = 1 << 4;
		for (int segment = 0; segment < 5; fontMask >>= 1, segment++) {

			self->_frameBuffer[segmentNr + segment] &= (WORD)(~comnMask);
			self->_frameBuffer[segmentNr + segment] |= (font[fontRow] & fontMask) ? comnMask : 0;

		}
	}
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



/**
 * @fn		segview_DrawCharacterCell
 * @brief	Draw one 5x7 / 5x10 Pixel Character.
 * @param	SEGnr: 0 - 39
 * @param	nSEGS: 1 - 8 (Usually 5)
 * @param	nCOMS: 1 - 16 (Usually 7)
 * ---------------------------------------------------------------------------*/
static void segview_DrawCharacterCell (SegmentView *self, unsigned SEGnr, unsigned logical_line)
{
	assert(SEGnr < 150);

	SIZE  szTarget; // The Pizel Size of the Glass Representation 
	POINT ptScreen; // The Pizel Location of the Glass Representation 

	szTarget.cx = self->pxCharWidth;
	szTarget.cy = self->pxCharHeight;

	int charPosn     = SEGnr / self->ndCharWidth;
	int COMnr        = (logical_line == 0) ? 0 : 8;
	int COMmask      = (1 << self->ndCharHeight) - 1;
	int nComsPerLine = (self->controllerDuty == 11) ? 11 : 8;

	int binaryDotWidth = (self->_bmBinaryDot.bmWidth / 256);

	for (int dotCol = 0; dotCol < self->ndCharWidth; dotCol++) {

		LONG xPosSource = binaryDotWidth * ((self->_frameBuffer[SEGnr + dotCol] >> COMnr) & 0xff);

		BitBlt(self->_hdcCharacter, (dotCol * binaryDotWidth), 0, binaryDotWidth, self->_bmBinaryDot.bmHeight,
		       self->_hdcBinaryDot, xPosSource, 0, SRCCOPY);
	}

	ptScreen.x = self->pxMargin_LHS + (charPosn % self->nGlasslineChars) * self->pxCharSpacing;
	ptScreen.y = self->pxMargin_TOP + (charPosn / self->nGlasslineChars) * self->pxLineSpacing * 2 + (logical_line * self->pxLineSpacing);

	HDC hdc = GetDC(self->_hLCD);
	SetStretchBltMode (hdc, STRETCH_HALFTONE);
	StretchBlt(hdc, ptScreen.x, ptScreen.y, self->pxCharWidth, self->pxCharHeight,
	           self->_hdcCharacter, 0, 0, self->_bmCharacter.bmWidth, (self->_bmBinaryDot.bmHeight / 8 * self->ndCharHeight), SRCCOPY);

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
		segview_StoreCharacter(self, charPos * 5, 0, FontMap[charPos + 'A' - ' ']);
		segview_StoreCharacter(self, charPos * 5, 1, FontMap[charPos + 'a' - ' ']);
	}
	segview_UpdateSegmentView (self);
}
