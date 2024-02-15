//-----------------------------------------------------------------------------*
// Copyright � 2024 Nigel Winterbottom.
// FILE			: CharacterLcd.c
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Hitachi HD44780 LCD Controller Simulator.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*


/* Implements *****************************************************************/
#include "CharacterLcd.h"

/* Dependencies ***************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "SegmentView.h"


/* Local Defines **************************************************************/
#define MemoryPerRow 40


/* Local Types ****************************************************************/
/* Local Variables ************************************************************/


/* Local Functions ************************************************************/
static void UpdateLcd  (CharacterLcd_t * self);
static void DrawPixels (CharacterLcd_t * self, uint8_t index);



/* Public Functions ***********************************************************/

void charlcd_CreateController (CharacterLcd_t * self, SegmentView * view)
{
	static const uint8_t resetfont0[] = { 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f };
	static const uint8_t resetfont1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	self->view = view;
	charlcd_SetNrLines(self, 1);

	charlcd_ClearDisplay (self);

	for (uint16_t segment = 0; segment < 400; segment+=5) {
		segview_StoreCharacter(self->view, segment, 0, resetfont0);
		segview_StoreCharacter(self->view, segment, 1, resetfont1);
	}
}


void charlcd_SetNrLines (CharacterLcd_t * self, int n)
{
	self->bTwoLineMode = (n == 2);
}

uint8_t charlcd_GetDD_RAM (CharacterLcd_t * self, uint8_t index)
{
	if (index >= 80)
		return 0xff;

	return self->DD_RAM[index];
}

void charlcd_SetDD_RAM (CharacterLcd_t * self, uint8_t index, uint8_t data)
{
	if (index >= 80)
		return;

	self->DD_RAM[index] = data;
	DrawPixels(self, index);
}

uint8_t charlcd_GetCG_RAM (CharacterLcd_t * self, uint8_t index)
{
	return self->CG_RAM[index & 63];
}

void charlcd_SetCG_RAM (CharacterLcd_t * self, uint8_t index, uint8_t data)
{
	self->CG_RAM[index & 63] = data;
	UpdateLcd(self); // Let's simply Update the whole LCD to "forward" any font change onto the view.
}


void charlcd_SetDisplay (CharacterLcd_t * self, bool blinkOn, bool cursorOn, bool displayOn)
{
	self->bBlnkOn    = blinkOn;
	self->bCursonOn  = cursorOn;
	self->bDisplayOn = displayOn;
}

void charlcd_ScrollDisplay (CharacterLcd_t * self, uint8_t offset)
{
	self->ScrollOffset += offset;
	while (self->ScrollOffset < 0)             self->ScrollOffset += MemoryPerRow;
	while (self->ScrollOffset >= MemoryPerRow) self->ScrollOffset -= MemoryPerRow;

	UpdateLcd(self);
}

void charlcd_ScrollCursor (CharacterLcd_t * self, uint8_t offset)
{
	self->CursorX += offset;
	while (self->CursorX < 0)             { self->CursorX += MemoryPerRow; self->CursorY = (self->CursorY + 1) & 1; }
	while (self->CursorX >= MemoryPerRow) { self->CursorX -= MemoryPerRow; self->CursorY = (self->CursorY + 1) & 1; }
}

void charlcd_CursorHome (CharacterLcd_t * self)
{
	self->CursorX = 0;
	self->CursorY = 0;
	self->ScrollOffset = 0;
}

void charlcd_ClearDisplay (CharacterLcd_t * self)
{
	memset(self->DD_RAM, ' ', sizeof self->DD_RAM);
	charlcd_CursorHome(self);
}


/* Local Functions ************************************************************/


void UpdateLcd (CharacterLcd_t * self)
{
	for (uint8_t y = 0; y < (self->bTwoLineMode ? 2 : 1); ++y)
	{
		uint8_t memAddr = self->ScrollOffset;
		for (uint8_t x = 0; x < MemoryPerRow; ++x) {
			DrawPixels (self, memAddr);
		}
	}
}


/**
 * @fn		DrawPixels
 * @brief	Draw one 5x7-Pixel Character.
 * @param	segmentIdx: 0x00 - 0x27.
 */
static void DrawPixels (CharacterLcd_t * self, uint8_t index)
{
	static const uint8_t font[][8] = {
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00 },
		{ 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A, 0x00 },
		{ 0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04, 0x00 },
		{ 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03, 0x00 },
		{ 0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D, 0x00 },
		{ 0x0C, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02, 0x00 },
		{ 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0x00 },
		{ 0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00, 0x00 },
		{ 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x08, 0x00 },
		{ 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 },
		{ 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 },
		{ 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E, 0x00 },
		{ 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00 },
		{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00 },
		{ 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E, 0x00 },
		{ 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02, 0x00 },
		{ 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E, 0x00 },
		{ 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00 },
		{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00 },
		{ 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00 },
		{ 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0x00 },
		{ 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00, 0x00 },
		{ 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08, 0x00 },
		{ 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x00 },
		{ 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00 },
		{ 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00 },
		{ 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0x00 },
		{ 0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E, 0x00 },
		{ 0x0E, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00 },
		{ 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00 },
		{ 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0x00 },
		{ 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E, 0x00 },
		{ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00 },
		{ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00 },
		{ 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F, 0x00 },
		{ 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00 },
		{ 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00 },
		{ 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00 },
		{ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0x00 },
		{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00 },
		{ 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11, 0x00 },
		{ 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00 },
		{ 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00 },
		{ 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00 },
		{ 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00 },
		{ 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00 },
		{ 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E, 0x00 },
		{ 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00 },
		{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00 },
		{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00 },
		{ 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A, 0x00 },
		{ 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11, 0x00 },
		{ 0x11, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x00 },
		{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F, 0x00 },
		{ 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E, 0x00 },
		{ 0x11, 0x0A, 0x1F, 0x04, 0x1F, 0x04, 0x04, 0x00 },
		{ 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x00 },
		{ 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00 },
		{ 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00 },
		{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E, 0x00 },
		{ 0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E, 0x00 },
		{ 0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F, 0x00 },
		{ 0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00 },
		{ 0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08, 0x00 },
		{ 0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E, 0x00 },
		{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00 },
		{ 0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00 },
		{ 0x02, 0x06, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00 },
		{ 0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12, 0x00 },
		{ 0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00 },
		{ 0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11, 0x00 },
		{ 0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00 },
		{ 0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00 },
		{ 0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10, 0x00 },
		{ 0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01, 0x00 },
		{ 0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0x00 },
		{ 0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E, 0x00 },
		{ 0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06, 0x00 },
		{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D, 0x00 },
		{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00 },
		{ 0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A, 0x00 },
		{ 0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00 },
		{ 0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E, 0x00 },
		{ 0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F, 0x00 },
		{ 0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02, 0x00 },
		{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00 },
		{ 0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08, 0x00 },
		{ 0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00 },
		{ 0x00, 0x04, 0x08, 0x1F, 0x08, 0x04, 0x00, 0x00 },
	};

	static const uint8_t fontII[][8] = {

		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0 },
		{ 0x00, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x0a, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x0a, 0 },
		{ 0x04, 0x0f, 0x14, 0x0e, 0x05, 0x1e, 0x04, 0 },
		{ 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03, 0 },
		{ 0x0c, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0d, 0 },
		{ 0x0c, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02, 0 },
		{ 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0 },
		{ 0x00, 0x04, 0x15, 0x0e, 0x15, 0x04, 0x00, 0 },
		{ 0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x08, 0 },
		{ 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0 },
		{ 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0 },
		{ 0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e, 0 },
		{ 0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e, 0 },
		{ 0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f, 0 },
		{ 0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e, 0 },
		{ 0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02, 0 },
		{ 0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e, 0 },
		{ 0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e, 0 },
		{ 0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0 },
		{ 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e, 0 },
		{ 0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c, 0 },
		{ 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00, 0 },
		{ 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x04, 0x08, 0 },
		{ 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0 },
		{ 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0 },
		{ 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0 },
		{ 0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0 },
		{ 0x0e, 0x11, 0x01, 0x0d, 0x15, 0x15, 0x0e, 0 },
		{ 0x0e, 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0 },
		{ 0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e, 0 },
		{ 0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e, 0 },
		{ 0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e, 0 },
		{ 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f, 0 },
		{ 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10, 0 },
		{ 0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f, 0 },
		{ 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11, 0 },
		{ 0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e, 0 },
		{ 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c, 0 },
		{ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0 },
		{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f, 0 },
		{ 0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11, 0 },
		{ 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0 },
		{ 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0 },
		{ 0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10, 0 },
		{ 0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d, 0 },
		{ 0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11, 0 },
		{ 0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e, 0 },
		{ 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0 },
		{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e, 0 },
		{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04, 0 },
		{ 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a, 0 },
		{ 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11, 0 },
		{ 0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0 },
		{ 0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f, 0 },
		{ 0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e, 0 },
		{ 0x11, 0x0a, 0x1f, 0x04, 0x1f, 0x04, 0x04, 0 },
		{ 0x0e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e, 0 },
		{ 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0 },
		{ 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x0e, 0x01, 0x0f, 0x11, 0x0f, 0 },
		{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1e, 0 },
		{ 0x00, 0x00, 0x0e, 0x10, 0x10, 0x11, 0x0e, 0 },
		{ 0x01, 0x01, 0x0d, 0x13, 0x11, 0x11, 0x0f, 0 },
		{ 0x00, 0x00, 0x0e, 0x11, 0x1f, 0x10, 0x0e, 0 },
		{ 0x06, 0x09, 0x08, 0x1c, 0x08, 0x08, 0x08, 0 },
		{ 0x00, 0x00, 0x0f, 0x11, 0x0f, 0x01, 0x0e, 0 },
		{ 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11, 0 },
		{ 0x04, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x0e, 0 },
		{ 0x02, 0x06, 0x02, 0x02, 0x02, 0x12, 0x0c, 0 },
		{ 0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12, 0 },
		{ 0x0c, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e, 0 },
		{ 0x00, 0x00, 0x1a, 0x15, 0x15, 0x11, 0x11, 0 },
		{ 0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0 },
		{ 0x00, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e, 0 },
		{ 0x00, 0x00, 0x1e, 0x11, 0x1e, 0x10, 0x10, 0 },
		{ 0x00, 0x00, 0x0d, 0x13, 0x0f, 0x01, 0x01, 0 },
		{ 0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0 },
		{ 0x00, 0x00, 0x0f, 0x10, 0x0e, 0x01, 0x1e, 0 },
		{ 0x08, 0x08, 0x1c, 0x08, 0x08, 0x09, 0x06, 0 },
		{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0d, 0 },
		{ 0x00, 0x00, 0x11, 0x11, 0x11, 0x0a, 0x04, 0 },
		{ 0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0a, 0 },
		{ 0x00, 0x00, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0 },
		{ 0x00, 0x00, 0x11, 0x11, 0x0f, 0x01, 0x0e, 0 },
		{ 0x00, 0x00, 0x1f, 0x02, 0x04, 0x08, 0x1f, 0 },
		{ 0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02, 0 },
		{ 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0 },
		{ 0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08, 0 },
		{ 0x00, 0x04, 0x02, 0x1f, 0x02, 0x04, 0x00, 0 },
		{ 0x00, 0x04, 0x08, 0x1f, 0x08, 0x04, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 }, // CharCode 0x80
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x1c, 0x14, 0x1c, 0 },
		{ 0x07, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x1c, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0 },
		{ 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0 },
		{ 0x00, 0x1f, 0x01, 0x1f, 0x01, 0x02, 0x04, 0 },
		{ 0x00, 0x00, 0x1f, 0x01, 0x06, 0x04, 0x08, 0 },
		{ 0x00, 0x00, 0x02, 0x04, 0x0c, 0x14, 0x04, 0 },
		{ 0x00, 0x00, 0x04, 0x1f, 0x11, 0x01, 0x06, 0 },
		{ 0x00, 0x00, 0x00, 0x1f, 0x04, 0x04, 0x1f, 0 },
		{ 0x00, 0x00, 0x02, 0x1f, 0x06, 0x0a, 0x12, 0 },
		{ 0x00, 0x00, 0x08, 0x1f, 0x09, 0x0a, 0x08, 0 },
		{ 0x00, 0x00, 0x00, 0x0e, 0x02, 0x02, 0x1f, 0 },
		{ 0x00, 0x00, 0x1e, 0x02, 0x1e, 0x02, 0x1e, 0 },
		{ 0x00, 0x00, 0x00, 0x15, 0x15, 0x01, 0x06, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0 },
		{ 0x1f, 0x01, 0x05, 0x06, 0x04, 0x04, 0x08, 0 },
		{ 0x01, 0x02, 0x04, 0x0c, 0x14, 0x04, 0x04, 0 },
		{ 0x04, 0x1f, 0x11, 0x11, 0x01, 0x02, 0x04, 0 },
		{ 0x00, 0x00, 0x1f, 0x04, 0x04, 0x04, 0x1f, 0 },
		{ 0x02, 0x1f, 0x02, 0x06, 0x0a, 0x12, 0x02, 0 },
		{ 0x08, 0x1f, 0x09, 0x09, 0x09, 0x09, 0x12, 0 },
		{ 0x04, 0x1f, 0x04, 0x1f, 0x04, 0x04, 0x04, 0 },
		{ 0x00, 0x0f, 0x09, 0x11, 0x01, 0x02, 0x0c, 0 },
		{ 0x08, 0x0f, 0x12, 0x02, 0x02, 0x02, 0x04, 0 },
		{ 0x00, 0x1f, 0x01, 0x01, 0x01, 0x01, 0x1f, 0 },
		{ 0x0a, 0x1f, 0x0a, 0x0a, 0x02, 0x04, 0x08, 0 },
		{ 0x00, 0x18, 0x01, 0x19, 0x01, 0x02, 0x1c, 0 },
		{ 0x00, 0x1f, 0x01, 0x02, 0x04, 0x0a, 0x11, 0 },
		{ 0x08, 0x1f, 0x09, 0x0a, 0x08, 0x08, 0x07, 0 },
		{ 0x00, 0x11, 0x11, 0x09, 0x01, 0x02, 0x0c, 0 },
		{ 0x00, 0x0f, 0x09, 0x15, 0x03, 0x02, 0x0c, 0 },
		{ 0x02, 0x1c, 0x04, 0x1f, 0x04, 0x04, 0x08, 0 },
		{ 0x00, 0x15, 0x15, 0x01, 0x01, 0x02, 0x04, 0 },
		{ 0x0e, 0x00, 0x1f, 0x04, 0x04, 0x04, 0x08, 0 },
		{ 0x08, 0x08, 0x08, 0x0c, 0x0a, 0x08, 0x08, 0 },
		{ 0x04, 0x04, 0x1f, 0x04, 0x04, 0x08, 0x10, 0 },
		{ 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x1f, 0 },
		{ 0x00, 0x1f, 0x01, 0x0a, 0x04, 0x0a, 0x10, 0 },
		{ 0x04, 0x1f, 0x02, 0x04, 0x0e, 0x15, 0x04, 0 },
		{ 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x08, 0 },
		{ 0x00, 0x04, 0x02, 0x11, 0x11, 0x11, 0x11, 0 },
		{ 0x10, 0x10, 0x1f, 0x10, 0x10, 0x10, 0x0f, 0 },
		{ 0x00, 0x1f, 0x01, 0x01, 0x01, 0x02, 0x0c, 0 },
		{ 0x00, 0x08, 0x14, 0x02, 0x01, 0x01, 0x00, 0 },
		{ 0x04, 0x1f, 0x04, 0x04, 0x15, 0x15, 0x04, 0 },
		{ 0x00, 0x1f, 0x01, 0x01, 0x0a, 0x04, 0x02, 0 },
		{ 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x01, 0 },
		{ 0x00, 0x04, 0x08, 0x10, 0x11, 0x1f, 0x01, 0 },
		{ 0x00, 0x01, 0x01, 0x0a, 0x04, 0x0a, 0x10, 0 },
		{ 0x00, 0x1f, 0x08, 0x1f, 0x08, 0x08, 0x07, 0 },
		{ 0x08, 0x08, 0x1f, 0x09, 0x0a, 0x08, 0x08, 0 },
		{ 0x00, 0x0e, 0x02, 0x02, 0x02, 0x02, 0x1f, 0 },
		{ 0x00, 0x1f, 0x01, 0x1f, 0x01, 0x01, 0x1f, 0 },
		{ 0x0e, 0x00, 0x1f, 0x01, 0x01, 0x02, 0x04, 0 },
		{ 0x12, 0x12, 0x12, 0x12, 0x02, 0x04, 0x08, 0 },
		{ 0x00, 0x04, 0x14, 0x14, 0x15, 0x15, 0x16, 0 },
		{ 0x00, 0x10, 0x10, 0x11, 0x12, 0x14, 0x18, 0 },
		{ 0x00, 0x1f, 0x11, 0x11, 0x11, 0x11, 0x1f, 0 },
		{ 0x00, 0x1f, 0x11, 0x11, 0x01, 0x02, 0x04, 0 },
		{ 0x00, 0x18, 0x00, 0x01, 0x01, 0x02, 0x1c, 0 },
		{ 0x04, 0x12, 0x08, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x1c, 0x14, 0x1c, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x00, 0x00, 0x09, 0x15, 0x12, 0x12, 0x0d, 0 },
		{ 0x0a, 0x00, 0x0e, 0x01, 0x0f, 0x11, 0x0f, 0 },
		{ 0x00, 0x0e, 0x11, 0x1e, 0x11, 0x1e, 0x10, 0 },
		{ 0x00, 0x00, 0x0e, 0x10, 0x0c, 0x11, 0x0e, 0 },
		{ 0x00, 0x11, 0x11, 0x11, 0x13, 0x1d, 0x10, 0 },
		{ 0x00, 0x00, 0x0f, 0x14, 0x12, 0x11, 0x0e, 0 },
		{ 0x00, 0x06, 0x09, 0x11, 0x11, 0x1e, 0x10, 0 },
		{ 0x00, 0x0f, 0x11, 0x11, 0x11, 0x0f, 0x01, 0 },
		{ 0x00, 0x00, 0x07, 0x04, 0x04, 0x14, 0x08, 0 },
		{ 0x02, 0x1a, 0x02, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x02, 0x00, 0x06, 0x02, 0x02, 0x02, 0x02, 0 },
		{ 0x00, 0x14, 0x08, 0x14, 0x00, 0x00, 0x00, 0 },
		{ 0x04, 0x0e, 0x14, 0x15, 0x0e, 0x04, 0x00, 0 },
		{ 0x08, 0x08, 0x1c, 0x08, 0x1c, 0x08, 0x0f, 0 },
		{ 0x0e, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0 },
		{ 0x0a, 0x00, 0x0e, 0x11, 0x11, 0x11, 0x0e, 0 },
		{ 0x00, 0x16, 0x19, 0x11, 0x11, 0x1e, 0x10, 0 },
		{ 0x00, 0x0d, 0x13, 0x11, 0x11, 0x0f, 0x01, 0 },
		{ 0x0e, 0x11, 0x1f, 0x11, 0x11, 0x0e, 0x00, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x0b, 0x15, 0x1a, 0 },
		{ 0x00, 0x0e, 0x11, 0x11, 0x0a, 0x1b, 0x00, 0 },
		{ 0x0a, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0d, 0 },
		{ 0x1f, 0x10, 0x08, 0x04, 0x08, 0x10, 0x1f, 0 },
		{ 0x00, 0x1f, 0x0a, 0x0a, 0x0a, 0x13, 0x00, 0 },
		{ 0x1f, 0x00, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0 },
		{ 0x00, 0x11, 0x11, 0x11, 0x11, 0x0f, 0x01, 0 },
		{ 0x01, 0x1e, 0x04, 0x1f, 0x04, 0x04, 0x00, 0 },
		{ 0x00, 0x1f, 0x08, 0x0f, 0x09, 0x11, 0x00, 0 },
		{ 0x00, 0x1f, 0x15, 0x1f, 0x11, 0x11, 0x00, 0 },
		{ 0x00, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x04, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0 },
		{ 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0 },
	};


	/* Calculate target SEGMENT Nr (Characters are always 5 segments wide) */
	uint8_t  line;
	uint16_t segment;
	if (self->bTwoLineMode) {
		line = (index < MemoryPerRow) ? 0 : 1;
		segment = 5 * (index % MemoryPerRow);
	}
	else {
		line = 0;
		segment = 5 * index;
	}

	segview_StoreCharacter(self->view, segment, line, font[self->DD_RAM[index] - ' ']);
}
