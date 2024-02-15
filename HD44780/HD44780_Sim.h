//-----------------------------------------------------------------------------*
// Copyright © 2023,2024 Nigel Winterbottom.
// FILE			: HD44780_Sim.h
// ENCODING		: ISO 8859-1
// DESCRIPTION	: Hitachi HD44780 LCD Controller Simulator.
// CREATED		: 2023-04-10
// AUTHOR		: Nigel Winterbottom
// CPU TYPE		: ANY
// COMPILER		: ANY
// COMMENTS		:
//-----------------------------------------------------------------------------*


#ifndef H_HD44780_SIM_H
#define H_HD44780_SIM_H


/* Dependencies ***************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "CharacterLcd.h"
#include "SegmentView.h"

/* Public Defines *************************************************************/

// commands
#define HD44780_CLEARDISPLAY   0x01
#define HD44780_RETURNHOME     0x02
#define HD44780_ENTRYMODESET   0x04
#define HD44780_DISPLAYCONTROL 0x08
#define HD44780_CURSORSHIFT    0x10
#define HD44780_FUNCTIONSET    0x20
#define HD44780_SETCGRAMADDR   0x40
#define HD44780_SETDDRAMADDR   0x80

// flags for display entry mode
#define HD44780_ENTRYRIGHT 0x02
#define HD44780_ENTRYLEFT  0x00
#define HD44780_ENTRYSHIFTON  0x01
#define HD44780_ENTRYSHIFTOFF 0x00

// flags for display on/off control
#define HD44780_DISPLAYON 0x04
#define HD44780_DISPLAYOFF 0x00
#define HD44780_CURSORON 0x02
#define HD44780_CURSOROFF 0x00
#define HD44780_BLINKON 0x01
#define HD44780_BLINKOFF 0x00

// flags for display/cursor shift
#define HD44780_DISPLAYMOVE 0x08
#define HD44780_CURSORMOVE 0x00
#define HD44780_MOVERIGHT 0x04
#define HD44780_MOVELEFT 0x00

// flags for function set
#define HD44780_8BITMODE 0x10
#define HD44780_4BITMODE 0x00
#define HD44780_2LINE 0x08
#define HD44780_1LINE 0x00
#define HD44780_5x10DOTS 0x04
#define HD44780_5x8DOTS 0x00

/* Public Types ***************************************************************/

typedef struct HD44780_Controller_s
{
	enum BitMode { BitMode_Four, BitMode_Eight } BitMode;

	CharacterLcd_t * lcd;

	uint8_t AddressCounter;
	uint8_t DataRegister;
	uint8_t InstrRegister;
	bool    bCGRAM_Mode;

	int8_t  _CursorMovement;    // -1 or +1
	bool    _bAutoScrollMode;
	bool    _bHandleHighNibble;

} HD44780_Controller_t;

/* Public Variables ***********************************************************/
/* Public Functions ***********************************************************/
void    hd44780_CreateController    (HD44780_Controller_t * controller, CharacterLcd_t * lcd, SegmentView * view);
uint8_t hd44780_ReadControllerCmnd  (HD44780_Controller_t * controller);
uint8_t hd44780_ReadControllerData  (HD44780_Controller_t * controller);
void    hd44780_WriteControllerCmnd (HD44780_Controller_t * controller, uint8_t cmnd);
void    hd44780_WriteControllerData (HD44780_Controller_t * controller, uint8_t data);

#endif /* H_HD44780_SIM_H */
