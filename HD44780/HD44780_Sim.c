//-----------------------------------------------------------------------------*
// Copyright © 2023 Kane International Ltd.
// Instrument	: KANE Combustion Analyser
// FILE			: HitachiController.c
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Hitachi HD44780 LCD Controller Simulator.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*


/* Implements *****************************************************************/
#include "HD44780_Sim.h"

/* Dependencies ***************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "CharacterLcd.h"
#include "SegmentView.h"


/* Local Defines **************************************************************/
#define MemoryPerRow 40


/* Local Types ****************************************************************/


/* Local Variables ************************************************************/
#define MemoryPerRow 40


/* Local Functions ************************************************************/
static uint8_t ReadNibble   (HD44780_Controller_t * controller, uint8_t byte);
static bool    WriteNibble  (HD44780_Controller_t * controller, uint8_t nibble, uint8_t * byte);

static void SetDDRamAddress (HD44780_Controller_t * self, uint8_t address);
static void SetCGRamAddress (HD44780_Controller_t * self, uint8_t address);
static void FunctionSet     (HD44780_Controller_t * self, uint8_t instr);
static void Shift           (HD44780_Controller_t * self, uint8_t instr);
static void SetDisplayOnOff (HD44780_Controller_t * self, uint8_t instr);
static void SetEntryMode    (HD44780_Controller_t * self, uint8_t instr);
static void CursorHome      (HD44780_Controller_t * self);
static void ClearDisplay    (HD44780_Controller_t * self);

static uint8_t AddressCounterToIndex (HD44780_Controller_t * self);
static void    StepAddressCounter    (HD44780_Controller_t * self);


/* Public Functions ***********************************************************/

void hd44780_CreateController (HD44780_Controller_t * controller, CharacterLcd_t * lcd, SegmentView * view)
{
	controller->lcd = lcd;
	controller->BitMode = BitMode_Eight;
	charlcd_CreateController(lcd, view);
}

uint8_t hd44780_ReadControllerCmnd  (HD44780_Controller_t * self) {
	return ReadNibble(self, self->AddressCounter); // NB: My Status will never be Busy
}

uint8_t hd44780_ReadControllerData  (HD44780_Controller_t * self) {
	return ReadNibble(self, self->DataRegister);
}

void hd44780_WriteControllerCmnd (HD44780_Controller_t * self, uint8_t data)
{
	if (WriteNibble(self, data, &self->InstrRegister) == false)
		return;

	uint8_t instr = self->InstrRegister;

	if (     (instr & 0x80) != 0)
		SetDDRamAddress(self, (instr & 0x7f));
	else if ((instr & 0x40) != 0)
		SetCGRamAddress(self, (instr & 0x3f));
	else if ((instr & 0x20) != 0)
		FunctionSet(self, instr);
	else if ((instr & 0x10) != 0)
		Shift(self, instr);
	else if ((instr & 0x08) != 0)
		SetDisplayOnOff(self, instr);
	else if ((instr & 0x04) != 0)
		SetEntryMode(self, instr);
	else if ((instr & 0x02) != 0)
		CursorHome(self);
	else if ((instr & 0x01) != 0)
		ClearDisplay(self);

}


void hd44780_WriteControllerData (HD44780_Controller_t * self, uint8_t data)
{
	if (WriteNibble(self, data, &self->DataRegister) == false)
		return;

	if (self->bCGRAM_Mode) {
		charlcd_SetCG_RAM(self->lcd, self->AddressCounter, self->DataRegister);
		SetCGRamAddress(self, self->AddressCounter + self->_CursorMovement);
	}
	else {
		charlcd_SetDD_RAM(self->lcd, AddressCounterToIndex(self), self->DataRegister);
		StepAddressCounter(self);
		SetDDRamAddress(self, self->AddressCounter);
	}
}


/* Local Functions ************************************************************/


static void SetDDRamAddress (HD44780_Controller_t * self, uint8_t address)
{
	self->AddressCounter = address;
	self->bCGRAM_Mode    = false;
	self->DataRegister   = charlcd_GetDD_RAM(self->lcd, AddressCounterToIndex(self));
}

static void SetCGRamAddress (HD44780_Controller_t * self, uint8_t address)
{
	self->AddressCounter = address;
	self->bCGRAM_Mode    = true;
}

static void FunctionSet (HD44780_Controller_t * self, uint8_t instr)
{
	/* Bit:   7  6  5  4  3  2  1  0  */
	/* Func:  0  0  1  DL N  F  —  —  */
	self->BitMode = ((instr & 1<<4) != 0) ? BitMode_Eight : BitMode_Four;
	int lines  = ((instr & 1<<3) != 0) ? 2 : 1;

	self->_bHandleHighNibble = true;
	charlcd_SetNrLines(self->lcd, lines);
	/* No Font Switch Implemented */
}

static void Shift (HD44780_Controller_t * self, uint8_t instr)
{
	/* Bit:   7   6   5   4   3    2   1   0  */
	/* Func:  0   0   0   1  S/C  R/L  —   —  */

	int8_t offset = ((instr & 1<<2) != 0) ? +1 : -1; // Read R/nL Bit

	if ((instr & 1<<3) != 0) { // Check Shift/Cursor Bit
		charlcd_ScrollDisplay(self->lcd, offset);
	}
	else {
		self->AddressCounter += offset;
		charlcd_ScrollCursor(self->lcd, offset);
	}
}

static void SetDisplayOnOff (HD44780_Controller_t * self, uint8_t instr)
{
	/* Bit:   7   6   5   4   3    2   1   0  */
	/* Func:  0   0   0   0   1    D   C   B  */

	bool displayOn = (instr & 1<<2) != 0;
	bool cursorOn  = (instr & 1<<1) != 0;
	bool blinkOn   = (instr & 1<<0) != 0;
	charlcd_SetDisplay(self->lcd, displayOn, cursorOn, blinkOn);
}

static void SetEntryMode (HD44780_Controller_t * self, uint8_t instr)
{
	/* Bit:   7   6   5   4   3    2   1   0  */
	/* Func:  0   0   0   0   0    1  I/D  S  */

	bool bCursorInc_Dec = ((instr & 1<<1) != 0);
	bool bAutoScroll    = ((instr & 1<<0) != 0);

	self->_CursorMovement  = bCursorInc_Dec ? 1 : -1;
	self->_bAutoScrollMode = bAutoScroll;
}

static void CursorHome (HD44780_Controller_t * self)
{
	self->AddressCounter = 0;
	self->lcd->ScrollOffset = 0;
	charlcd_CursorHome(self->lcd);
}

static void ClearDisplay (HD44780_Controller_t * self)
{
	self->AddressCounter   = 0;
	self->_CursorMovement  = 1;
	self->_bAutoScrollMode = false;
	charlcd_ClearDisplay(self->lcd);
}

/**
 * @brief	Convert a Virtual AddressCounter into linear DDRAM Index
 * @note    With HD44780 in 2 Line Mode. Virtual addresses are used:
 *          Vaddr 0x00 .. (0x00 + 39) for line one
 *          Vaddr 0x40 .. (0x40 + 39) for line two
 * @return	Index (0 .. 79)
 * ---------------------------------------------------------------------------*/
static uint8_t AddressCounterToIndex (HD44780_Controller_t * self)
{
	uint8_t index;
	if (self->lcd->bTwoLineMode && (self->AddressCounter & 0x40))   index = (self->AddressCounter - 0x40 + 40);
	else                                                            index = self->AddressCounter;

	while (index >= 80) index -= 80;
	return index;
}

static void StepAddressCounter (HD44780_Controller_t * self)
{
	self->AddressCounter += self->_CursorMovement; // This could now be out-of-range & MUST be fixed up.

	// NOTE: With HD44780 in 2 Line Mode. Virtual addresses are used:
	//  Vaddr 0x00 .. (0x00 + 39) for line one
	//  Vaddr 0x40 .. (0x40 + 39) for line two

	if (self->lcd->bTwoLineMode) {

		/* Wrap Around as a single line of 40 chars and merge in the opposite line */
		uint8_t newCounter = self->AddressCounter & ~0x40;
		if (newCounter >= 40) {
			self->AddressCounter = ((self->_CursorMovement == 1) ? 0 : 39) | ((self->AddressCounter ^ 0x40) & 0x40);
		}
	}
	else if (self->AddressCounter > 79) {
		self->AddressCounter = (self->_CursorMovement == 1) ? 0 : 79;
	}
}


/**
 * @brief	Handle 4-Bit and 8-Bit Mode Outgoing Data
 * @return	Byte for Transfer.
 * ---------------------------------------------------------------------------*/
static uint8_t ReadNibble (HD44780_Controller_t * self, uint8_t regData)
{
	uint8_t data;

	if (self->BitMode == BitMode_Eight) {
		return regData;
	}

	/* BitMode_Four If Here - Handle the Nibbles */
	if (self->_bHandleHighNibble) {
		data = (regData & 0xf0);
	}
	else {
		data = (regData & 0x0f) << 4;
	}

	self->_bHandleHighNibble = !(self->_bHandleHighNibble);
	return data;
}

/**
 * @brief	Handle 4-Bit and 8-Bit Mode Incoming Data
 * @return	true: Whole Byte has been received.
 * ---------------------------------------------------------------------------*/
static bool WriteNibble (HD44780_Controller_t * self, uint8_t incoming, uint8_t * byte)
{
	bool bHaveFullByte;

	if (self->BitMode == BitMode_Eight) {
		*byte = incoming;
		return true;
	}

	/* BitMode_Four If Here - Handle the Nibbles */
	if (self->_bHandleHighNibble) {
		*byte = (incoming & 0xf0);
		bHaveFullByte = false;
	}
	else {
		*byte = (*byte & 0xf0) | ((incoming & 0xf0) >> 4);
		bHaveFullByte = true;
	}

	self->_bHandleHighNibble = !(self->_bHandleHighNibble);
	return bHaveFullByte;
}

