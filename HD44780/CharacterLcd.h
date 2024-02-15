//-----------------------------------------------------------------------------*
// Copyright © 2023,2024 Nigel Winterbottom.
// FILE			: CharacterLcd.h
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Hitachi HD44780 LCD Controller Simulator.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*

#ifndef H_CHARACTERLCD_H
#define H_CHARACTERLCD_H

/* Dependencies ***************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "SegmentView.h"


/* Local Defines **************************************************************/
#define MemoryPerRow 40


/* Public Types ***************************************************************/
typedef struct CharacterLcd_s
{
	SegmentView * view;
	uint8_t DD_RAM[80];
	uint8_t CG_RAM[64];
	int     CursorX;
	int     CursorY;
	uint8_t ScrollOffset;

	bool bTwoLineMode;
	bool bBlnkOn;
	bool bDisplayOn;
	bool bCursonOn;
} CharacterLcd_t;


/* Public Functions ***********************************************************/


void charlcd_CreateController (CharacterLcd_t * self, SegmentView * view);
void charlcd_SetNrLines       (CharacterLcd_t * self, int n);
void charlcd_SetDisplay       (CharacterLcd_t * self, bool displayOn, bool cursorOn, bool blinkOn);
void charlcd_ScrollDisplay    (CharacterLcd_t * self, uint8_t n);
void charlcd_ScrollCursor     (CharacterLcd_t * self, uint8_t n);
void charlcd_CursorHome       (CharacterLcd_t * self);
void charlcd_ClearDisplay     (CharacterLcd_t * self);

uint8_t charlcd_GetDD_RAM     (CharacterLcd_t * self, uint8_t index);
uint8_t charlcd_GetCG_RAM     (CharacterLcd_t * self, uint8_t index);
void    charlcd_SetDD_RAM     (CharacterLcd_t * self, uint8_t index, uint8_t data);
void    charlcd_SetCG_RAM     (CharacterLcd_t * self, uint8_t index, uint8_t data);

#endif /* H_CHARACTERLCD_H */
