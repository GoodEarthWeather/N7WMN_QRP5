/*
 * menu_data.c
 *
 * Example menu table + implementation for the QRP radio menu system.
 * See menu_data.h for the data structure design rationale.
 */

#include <string.h>
#include <stdio.h>
#include "menu_data.h"
#include "hal_hw.h"
#include "radio_state.h"

/* ------------------------------------------------------------------ */
/* Hardware / state action callbacks -- ONE per menu item, no          */
/* exceptions. Even a "just set a variable" item gets a function here  */
/* so that Menu_OptionMove()/Menu_Init() never need to know what kind  */
/* of thing each item controls -- that knowledge lives ONLY in these   */
/* functions and nowhere else. Signature must match MenuActionFn in    */
/* menu_data.h.                                                        */
/* ------------------------------------------------------------------ */

static void Action_Wpm(const MenuItem_t *item, int16_t value)
{
    (void)item;
    /* Standard PARIS timing: dot length (ms) = 1200 / WPM */
    radioState.keyerDotTimeMs = (uint16_t)(1200 / value);
}

static void Action_Band(const MenuItem_t *item, int16_t value)
{
    (void)item; /* unused here, but available if the callback needs
                 * item->def.list.options[value] etc. */
    radioState.bandIndex = (uint8_t)value;
    HAL_HW_SelectBand((uint8_t)value);
}

/* Add one static Action_* function per item here as you flesh out the
 * remaining 19 slots, e.g.:
 * static void Action_RfPower(const MenuItem_t *item, int16_t value)
 * {
 *     (void)item;
 *     radioState.txPowerWatts = (uint8_t)value;
 *     HAL_HW_SetRfPower(value);
 * }
 */

/* ------------------------------------------------------------------ */
/* String tables for LIST-type items                                   */
/* (const -> FRAM, not RAM)                                            */
/* ------------------------------------------------------------------ */

static const char * const bandOptions[] =
{
    "80M", "40M", "30M", "20M", "17M", "15M", "12M", "10M"
};

static const char * const modeOptions[] =
{
    "CW", "USB", "LSB"
};

static const char * const keyerOptions[] =
{
    "IAMBIC-A", "IAMBIC-B", "STRAIGHT"
};

/* ------------------------------------------------------------------ */
/* The menu table itself.                                              */
/*                                                                       */
/* This is the ONE place you edit to add/reorder/reconfigure menu       */
/* items. ledIndex ties each entry to a physical LED / shift-register   */
/* bit position -- it does NOT have to match the array index, so you    */
/* can reorder entries here without rewiring anything.                  */
/* ------------------------------------------------------------------ */

const MenuItem_t menuTable[NUM_MENU_ITEMS] =
{
    /* index 0 */
    {
        .label = "WPM",
        .ledIndex = 0,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = 5, .maxValue = 40, .step = 1,
                        .defaultValue = 20, .unitSuffix = "" },
        .action = Action_Wpm   /* recomputes keyer dot-time in radioState */
    },
    /* index 1 */
    {
        .label = "BAND",
        .ledIndex = 1,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = bandOptions,
                       .numOptions = sizeof(bandOptions)/sizeof(bandOptions[0]),
                       .defaultIndex = 1 /* "40M" */ },
        .action = Action_Band   /* switches the bandpass filter relays */
    },
    /* index 2 */
    {
        .label = "MODE",
        .ledIndex = 2,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = modeOptions,
                       .numOptions = sizeof(modeOptions)/sizeof(modeOptions[0]),
                       .defaultIndex = 0 },
        .action = NULL
    },
    /* index 3 */
    {
        .label = "KEYER",
        .ledIndex = 3,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = keyerOptions,
                       .numOptions = sizeof(keyerOptions)/sizeof(keyerOptions[0]),
                       .defaultIndex = 0 },
        .action = NULL
    },
    /* index 4 */
    {
        .label = "RF PWR",
        .ledIndex = 4,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = 1, .maxValue = 5, .step = 1,
                        .defaultValue = 5, .unitSuffix = "W" },
        .action = NULL   /* wire up Action_RfPower here once you add it */
    },

    /* ---- fill in the remaining 19 items the same way ---- */
    /* Until they're defined, zero-init is safe: type defaults to      */
    /* MENU_TYPE_RANGE with all-zero fields, which Menu_FormatDisplayString */
    /* below will render harmlessly as "0". Replace as you build them out. */
};

/* ------------------------------------------------------------------ */
/* Mutable runtime state                                               */
/*                                                                       */
/* Placed in a persistent FRAM section so settings survive power        */
/* cycles with no extra save/load code. On the FR2476, ordinary         */
/* globals already live in FRAM, but #pragma PERSISTENT tells the       */
/* linker/loader to NOT re-initialize them from the .cinit table on     */
/* every reset -- i.e. it only takes the initializer the very first     */
/* time the device is programmed, and thereafter your runtime writes    */
/* are what persist across power-off/power-on.                          */
/* ------------------------------------------------------------------ */

#pragma PERSISTENT(menuCurrentValue)
int16_t menuCurrentValue[NUM_MENU_ITEMS] = { 0 };

#pragma PERSISTENT(menuSelectedIndex)
uint8_t menuSelectedIndex = 0;

#pragma PERSISTENT(menuInitialized)
static uint8_t menuInitialized = 0xFF; /* 0xFF => "never initialized" */

/* ------------------------------------------------------------------ */

void Menu_Init(void)
{
    uint8_t i;

    /* Only load factory defaults the very first time the device runs.
     * After that, menuCurrentValue[] already holds the user's saved
     * settings from FRAM and we must NOT stomp them on every boot. */
    if (menuInitialized != 0x01)
    {
        for (i = 0; i < NUM_MENU_ITEMS; i++)
        {
            if (menuTable[i].type == MENU_TYPE_RANGE)
            {
                menuCurrentValue[i] = menuTable[i].def.range.defaultValue;
            }
            else
            {
                menuCurrentValue[i] = (int16_t)menuTable[i].def.list.defaultIndex;
            }
        }
        menuSelectedIndex = 0;
        menuInitialized = 0x01;
    }

    /* Whether values just came from factory defaults above, or were
     * already sitting in FRAM from before the last power-off, the
     * actual hardware/state (relays, keyer timing, etc.) does NOT
     * remember anything -- so every item's action fires once here to
     * bring the radio into agreement with menuCurrentValue[]. NULL is
     * only a placeholder for the 19 slots not yet fleshed out; every
     * real item should have one by the time this ships. */
    for (i = 0; i < NUM_MENU_ITEMS; i++)
    {
        if (menuTable[i].action != NULL)
        {
            menuTable[i].action(&menuTable[i], menuCurrentValue[i]);
        }
    }
}

const MenuItem_t *Menu_GetItem(uint8_t itemIndex)
{
    if (itemIndex >= NUM_MENU_ITEMS)
    {
        itemIndex = 0; /* defensive clamp */
    }
    return &menuTable[itemIndex];
}

uint8_t Menu_GetSelectedLedIndex(void)
{
    return menuTable[menuSelectedIndex].ledIndex;
}

void Menu_SelectMove(int8_t delta)
{
    int16_t newIndex = (int16_t)menuSelectedIndex + delta;

    if (newIndex < 0)
    {
        newIndex += NUM_MENU_ITEMS;
    }
    else if (newIndex >= (int16_t)NUM_MENU_ITEMS)
    {
        newIndex -= NUM_MENU_ITEMS;
    }
    menuSelectedIndex = (uint8_t)newIndex;
}

void Menu_OptionMove(int8_t delta)
{
    const MenuItem_t *item = &menuTable[menuSelectedIndex];
    int16_t value = menuCurrentValue[menuSelectedIndex];

    if (item->type == MENU_TYPE_RANGE)
    {
        value += (int16_t)delta * item->def.range.step;

        /* clamp -- turning past the end just stops (typical for WPM,
         * RF power, etc.) */
        if (value < item->def.range.minValue)
        {
            value = item->def.range.minValue;
        }
        if (value > item->def.range.maxValue)
        {
            value = item->def.range.maxValue;
        }
    }
    else /* MENU_TYPE_LIST */
    {
        int16_t count = (int16_t)item->def.list.numOptions;
        value += delta;

        /* wrap -- typical for BAND/MODE selection */
        if (value < 0)
        {
            value += count;
        }
        else if (value >= count)
        {
            value -= count;
        }
    }

    menuCurrentValue[menuSelectedIndex] = value;

    /* Apply the HW side effect (if any) for the item just changed.
     * Fired unconditionally here even if value ended up unchanged
     * (e.g. clamped at a RANGE limit) -- harmless for idempotent
     * actions like relay selection, and keeps this call site simple. */
    if (item->action != NULL)
    {
        item->action(item, value);
    }
}

void Menu_FormatDisplayString(uint8_t itemIndex, char *buf, uint8_t bufSize)
{
    const MenuItem_t *item = &menuTable[itemIndex];
    int16_t value = menuCurrentValue[itemIndex];

    if (item->type == MENU_TYPE_RANGE)
    {
        /* e.g. "WPM 20" or "RF PWR 5W" */
        snprintf(buf, bufSize, "%s %d%s",
                 item->label, value, item->def.range.unitSuffix);
    }
    else /* MENU_TYPE_LIST */
    {
        uint8_t idx = (uint8_t)value;
        if (idx >= item->def.list.numOptions)
        {
            idx = 0; /* defensive clamp */
        }
        /* e.g. "BAND 17M" */
        snprintf(buf, bufSize, "%s %s",
                 item->label, item->def.list.options[idx]);
    }
}
