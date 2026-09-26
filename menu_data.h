/*
 * menu_data.h
 *
 * Data structures for the QRP radio human interface menu system.
 * Target: MSP430FR2476, Code Composer Studio, DriverLib
 *
 * Hardware model:
 *   - 24 LEDs (via 3x 74HCT595 shift registers) -> one lit at a time,
 *     indicates the currently selected menu item.
 *   - 16x2 LCD -> shows "<LABEL> <current option text/value>"
 *   - Menu Encoder      -> moves selection among the 24 items (0..23)
 *   - Menu Option Encoder -> steps the *value* of the selected item
 */

#ifndef MENU_DATA_H_
#define MENU_DATA_H_

#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/* Configuration                                                       */
/* ------------------------------------------------------------------ */

#define NUM_MENU_ITEMS      24U   /* must match number of LEDs         */
#define LCD_LINE_LEN        16U   /* 16x2 LCD, one line used for text  */

/* ------------------------------------------------------------------ */
/* Menu item type                                                      */
/* ------------------------------------------------------------------ */

typedef enum
{
    MENU_TYPE_RANGE = 0,   /* numeric, stepped between min/max          */
    MENU_TYPE_LIST  = 1    /* discrete list of text options             */
} MenuType_t;

/* ---- RANGE variant (e.g. WPM, RF power, sidetone Hz) -------------- */
typedef struct
{
    int16_t     minValue;
    int16_t     maxValue;
    int16_t     step;
    int16_t     defaultValue;
    const char *unitSuffix;   /* appended after the number, may be "" */
                              /* e.g. minValue=5 maxValue=40 step=1   */
                              /* -> "WPM 20"                          */
} MenuRange_t;

/* ---- LIST variant (e.g. BAND, MODE) -------------------------------- */
typedef struct
{
    const char * const *options;   /* array of string pointers          */
    uint8_t              numOptions;
    uint8_t               defaultIndex;
} MenuList_t;
struct MenuItem_s;
typedef void (*MenuActionFn)(const struct MenuItem_s *item, int16_t value);

/* ---- One menu item definition (STATIC / const / lives in FRAM) ---- */
typedef struct MenuItem_s
{
    const char *label;      /* short label, e.g. "WPM", "BAND"          */
                             /* also what you print on the LCD          */
    uint8_t     ledIndex;   /* 0..23, maps to shift-register bit         */
    MenuType_t  type;
    union
    {
        MenuRange_t range;
        MenuList_t  list;
    } def;
    MenuActionFn action;    /* NULL if this item has no HW side effect  */
} MenuItem_t;

/* ------------------------------------------------------------------ */
/* Declarations for the table + mutable state (defined in menu_data.c) */
/* ------------------------------------------------------------------ */

/* Static, read-only menu definitions -- one entry per LED/menu item.  */
extern const MenuItem_t menuTable[NUM_MENU_ITEMS];

/* Mutable "current value" for each item:                              */
/*   - for MENU_TYPE_RANGE: the actual numeric value                   */
/*   - for MENU_TYPE_LIST : the index into options[]                   */
/* Kept separate from menuTable so the definitions can stay const      */
/* while this small array is the only thing that changes at runtime.  */
extern int16_t menuCurrentValue[NUM_MENU_ITEMS];

/* Index (0..23) of the menu item currently selected by the Menu Encoder */
extern uint8_t menuSelectedIndex;

/* ------------------------------------------------------------------ */
/* API                                                                  */
/* ------------------------------------------------------------------ */

/* Call once at startup: loads defaultValue/defaultIndex into
 * menuCurrentValue[] the FIRST time only (see .c file for FRAM
 * first-boot detection pattern). */
void Menu_Init(void);

/* Menu Encoder turned: moves menuSelectedIndex, wraps 0..23.
 * delta is +1 or -1 per detent (adjust if you use quadrature counts). */
void Menu_SelectMove(int8_t delta);

/* Menu Option Encoder turned: changes the value/index of the
 * currently selected item. Clamps for RANGE, wraps for LIST
 * (change to suit taste -- see .c file). */
void Option_SelectMove(int8_t delta);

/* Formats "<LABEL> <value>" into buf for the LCD, e.g. "WPM 20" or
 * "BAND 17M". buf must be at least LCD_LINE_LEN+1 bytes. */
void Menu_FormatDisplayString(uint8_t itemIndex, char *buf, uint8_t bufSize);

/* Returns the ledIndex (0..23) of the currently selected item -- feed
 * this to your shift-register drive routine. */
uint8_t Menu_GetSelectedLedIndex(void);

/* Convenience accessor to a definition, e.g. for validation code. */
const MenuItem_t *Menu_GetItem(uint8_t itemIndex);

#endif /* MENU_DATA_H_ */
