//-----------------------------------------------------------------------------*
// Copyright © 2023,2024 Nigel Winterbottom.
// FILE			: SegmentView.h
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Generic 5x8 Character Type LCD DisplaySide Stuff.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*

#ifndef H_SEGMENTVIEW_H
#define H_SEGMENTVIEW_H

/* Dependencies ***************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  /* Win32 API */
#include <stdint.h>


/* Public Defines *************************************************************/
#define NR_GLASSLINES_MAX 6
#define NR_SEGMENTS       400
#define NR_COMMONS        16

/* Public Types ***************************************************************/
typedef struct {

	int      controllerDuty;    // 8, 11 or 16

	/* Basic Glass Layout. There must be some uniformity howeever. */
	int      nGlasslines;       // Number of Glass Panel lines
	int      nGlasslineChars;   // Number of Characters per Glass Panel line
	int      nGlassSegments;    // 1 to 400
	int      ndCharWidth;       // Number of Dots Wide of Character DotMatrix.
	int      ndCharHeight;      // Number of Dots High of Character DotMatrix.

	/* Details for Positioning each Character Cell */
	int      pxMargin_LHS;      // Size of the LHS Margin (Monitor Pixels)
	int      pxMargin_TOP;      // Size of the TOP Margin (Monitor Pixels)
	int      pxCharWidth;       // Width of Character (5 Columns) (Monitor Pixels)
	int      pxCharHeight;      // Height of Character (7, 8, 11 Rows) (Monitor Pixels)
	int      pxCharSpacing;     // Spacing between Character centres (Monitor Pixels)
	int      pxLineSpacing;     // Spacing between line centres (Monitor Pixels)

	/* My Private Stuff Hereafter */
//	uint8_t _row_offsets[NR_GLASSLINES_MAX]; //	setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

	HWND     _hLCD;
	RECT     _glassRect;         // Position & Size of the LCD Window Representing Glass Panel (Monitor Pixels)

	/* Bitmap with 256 DotMatrix Patterns in Column Order */
	HDC      _hdcBinaryDot;
	HBITMAP  _hbmBinaryDot;
	BITMAP   _bmBinaryDot;

	/* Bitmap for Character Composed from BinaryDot Bitmap */
	HDC      _hdcCharacter;
	HBITMAP  _hbmCharacter;
	BITMAP   _bmCharacter;

	//	BYTE *   _bits;

	/* NB: A real system would split this large amount of memory over several driver chips. */
	WORD     _frameBuffer[NR_SEGMENTS]; // Index-0 -> SEG0 : Bit-0 -> COM0.
} SegmentView;



/** Initilaise the GUI Element.*/
void segview_CreateSegmentView   (SegmentView *self, HWND hLCD, HBITMAP hbmBinarDot);
void segview_DestroySegmentView  (SegmentView *self);
void segview_FillGlassBackground (SegmentView *self);
void segview_UpdateSegmentView   (SegmentView *self);
void segview_TestSegmentView     (SegmentView *self);
void segview_StoreSegment        (SegmentView *self, uint16_t segmentNr, uint16_t pixels, uint8_t logical_line);

#endif /* H_SEGMENTVIEW_H */
