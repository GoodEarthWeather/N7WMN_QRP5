/*
 * menu_data.c
 *
 * Example menu table + implementation for the QRP radio menu system.
 * See menu_data.h for the data structure design rationale.
 */

#include <string.h>
#include <stdio.h>
#include "menu_data.h"
#include "radio_state.h"

/* ------------------------------------------------------------------ */
/* String tables for LIST-type items                                   */
/* (const -> FRAM, not RAM)                                            */
/* ------------------------------------------------------------------ */

static const char * const bandOptions[] =
{
    "BAND: 40M", "BAND: 30M", "BAND: 20M", "BAND: 17M", "BAND: 15M"
};

static const char * const modeOptions[] =
{
    "MODE: CW", "MODE: USB", "MODE: LSB"
};

static const char * const keyerOptions[] =
{
    "IAMBIC-A", "IAMBIC-B", "ULTIMATIC"
};

static const char * const filterOptions[] =
{
    "FILT: WIDE", "FILT: NARROW"
};
static const char * const rateOptions[] =
{
    "RATE: 10", "RATE: 100", "RATE: 1K", "RATE: 10K"
};
static const char * const spotOptions[] =
{
    "SPOT: OFF", "SPOT: ON"
};
static const char * const muteOptions[] =
{
    "MUTE: OFF", "MUTE: ON"
};
static const char * const playMemOptions[] =
{
    "PLAY MEM1", "PLAY MEM2", "PLAY MEM3"
};
static const char * const recMemOptions[] =
{
    "RECORD MEM1", "RECORD MEM2", "RECORD MEM3"
};
static const char * const paddleOrientOptions[] =
{
    "PDL: NORMAL", "PDL: REVERSE"
};
static const char * const audioOptions[] =
{
    "MONAURAL", "BINAURAL"
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
        .label = "BAND",
        .ledIndex = 0,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = bandOptions,
                       .numOptions = sizeof(bandOptions)/sizeof(bandOptions[0]),
                       .defaultIndex = 0 /* "40M" */ },
        .action = handleHW_band
    },
    /* index 1 */
    {
        .label = "RATE",
        .ledIndex = 1,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = rateOptions,
                       .numOptions = sizeof(rateOptions)/sizeof(rateOptions[0]),
                       .defaultIndex = 2 /* "1K" */ },
        .action = handleHW_rate
    },

    /* index 2 */
    {
        .label = "FILTER",
        .ledIndex = 2,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = filterOptions,
                       .numOptions = sizeof(filterOptions)/sizeof(filterOptions[0]),
                       .defaultIndex = 0 },
        .action = handleHW_filter
    },
    /* index 3 */
    {
        .label = "SPOT",
        .ledIndex = 3,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = spotOptions,
                       .numOptions = sizeof(spotOptions)/sizeof(spotOptions[0]),
                       .defaultIndex = 0 },
        .action = handleHW_spot
    },
    /* index 4 */
    {
        .label = "MUTE",
        .ledIndex = 4,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = muteOptions,
                       .numOptions = sizeof(muteOptions)/sizeof(muteOptions[0]),
                       .defaultIndex = 0 },
        .action = handleHW_mute
    },
    /* index 5 */
    {
        .label = "MODE",
        .ledIndex = 5,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = modeOptions,
                       .numOptions = sizeof(modeOptions)/sizeof(modeOptions[0]),
                       .defaultIndex = 0 },
        .action = handleHW_mode
    },
    /* index 6 */
    {
        .label = "PLAY MEM",
        .ledIndex = 6,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = playMemOptions,
                       .numOptions = sizeof(playMemOptions)/sizeof(playMemOptions[0]),
                       .defaultIndex = 0 },
        .action = NULL
    },
    /* index 7 */
    {
        .label = "RECORD MEM",
        .ledIndex = 7,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = recMemOptions,
                       .numOptions = sizeof(recMemOptions)/sizeof(recMemOptions[0]),
                       .defaultIndex = 0 },
        .action = NULL
    },
    /* index 8 */
    {
        .label = "WPM",
        .ledIndex = 8,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = 5, .maxValue = 30, .step = 1,
                        .defaultValue = 20, .unitSuffix = " WPM" },
        .action = handleHW_wpm
    },
    /* index 9 */
    {
        .label = "PADDLE ORIENT",
        .ledIndex = 9,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = paddleOrientOptions,
                       .numOptions = sizeof(paddleOrientOptions)/sizeof(paddleOrientOptions[0]),
                       .defaultIndex = 1 },
        .action = NULL
    },
    /* index 10 */
    {
        .label = "KEYER MODE",
        .ledIndex = 10,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = keyerOptions,
                       .numOptions = sizeof(keyerOptions)/sizeof(keyerOptions[0]),
                       .defaultIndex = 2 },
        .action = NULL
    },
    /* index 11 */
    {
        .label = "QSK",
        .ledIndex = 11,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = 5, .maxValue = 800, .step = 1,
                        .defaultValue = 150, .unitSuffix = " ms" },
        .action = NULL
    },
    /* index 12 */
    {
        .label = "AUDIO MODE",
        .ledIndex = 12,
        .type = MENU_TYPE_LIST,
        .def.list = { .options = audioOptions,
                       .numOptions = sizeof(audioOptions)/sizeof(audioOptions[0]),
                       .defaultIndex = 0 },
        .action = handleHW_audioMode
    },
    /* index 13 */
    {
        .label = "VOLTAGE",
        .ledIndex = 13,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = 5, .maxValue = 5, .step = 0,
                        .defaultValue = 5, .unitSuffix = " V" },
        .action = NULL
    },
    /* index 14 */
    {
        .label = "XIT",
        .ledIndex = 14,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = " Hz" },
        .action = NULL
    },
    /* index 15 */
    {
        .label = "RIT",
        .ledIndex = 15,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = " Hz" },
        .action = NULL
    },
    /* index 16 */
    {
        .label = "",
        .ledIndex = 16,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 17 */
    {
        .label = "",
        .ledIndex = 17,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 18 */
    {
        .label = "",
        .ledIndex = 18,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 19 */
    {
        .label = "",
        .ledIndex = 19,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 20 */
    {
        .label = "",
        .ledIndex = 20,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 21 */
    {
        .label = "",
        .ledIndex = 21,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 22 */
    {
        .label = "",
        .ledIndex = 22,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    },
    /* index 23 */
    {
        .label = "",
        .ledIndex = 23,
        .type = MENU_TYPE_RANGE,
        .def.range = { .minValue = -2000, .maxValue = 2000, .step = 10,
                        .defaultValue = 0, .unitSuffix = "" },
        .action = NULL
    }

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

void Option_SelectMove(int8_t delta)
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
