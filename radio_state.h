/*
 * radio_state.h
 *
 * Live operating parameters for the radio, derived from menu settings.
 * This is the decoupling point between the menu system and everything
 * else: the keyer, TX chain, etc. read from here and don't need to
 * know anything about menus, encoders, or LEDs. Menu action callbacks
 * are the ONLY code that writes to this struct.
 */

#ifndef RADIO_STATE_H_
#define RADIO_STATE_H_

#include <stdint.h>
#include "menu_data.h"


void handleHW_wpm(const  MenuItem_t *, int16_t);
void handleHW_band(const  MenuItem_t *, int16_t);
void handleHW_rate(const MenuItem_t *, int16_t);
void handleHW_filter(const MenuItem_t *, int16_t);
void handleHW_spot(const MenuItem_t *, int16_t);
void handleHW_mode(const MenuItem_t *, int16_t);

#define BAND_40M 0   //=> this is defined in radio_state.h because it is used by lcdLib.c
#define BAND_30M 1
#define BAND_20M 2
#define BAND_17M 3
#define BAND_15M 4


typedef struct
{
    uint8_t txMode; // flag; if true, in transmit mode; if not true, in receive mode
    uint8_t selectedSideband;
    uint8_t selectedFilter;
    uint8_t audioState;
    uint8_t wpm;  // current cw speed
    uint32_t frequency; // current vco freq; not including rit or xit
    uint32_t rxFrequency;  // current vco freq. with rit
    uint32_t txFrequency;  // current vco freq. with xit
    int16_t ritOffset;
    uint16_t xitOffset;
    uint16_t freqMultiplier;
    uint8_t bandIndex;
    uint32_t maxBandFreq;
    uint32_t minBandFreq;
    uint8_t bandRelayCode;
    uint8_t modeIndex;
    uint8_t filterIndex;
    uint8_t keyerIndex;
    uint8_t ledIndex;
} RadioState_t;

extern RadioState_t radioState;

#endif /* RADIO_STATE_H_ */
