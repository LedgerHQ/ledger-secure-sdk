#ifndef STAX_SYNTH_SE_H
#define STAX_SYNTH_SE_H
#include "stdint.h"
#include "stdbool.h"

#define STAX_MIDI_CHANNEL 2
#define STAX_PWM_CC_NB 30

typedef struct midi_message_s {
	uint8_t cable;
	uint8_t code;
	uint8_t message;
	uint8_t channel;
	uint8_t messageByte1;
	uint8_t messageByte2;
} midi_message_t;

typedef enum stax_note_e{
	STAX_NOTE_C0 = 0,
	STAX_NOTE_Cs0,
	STAX_NOTE_D0,
	STAX_NOTE_Ds0,
	STAX_NOTE_E0,
	STAX_NOTE_F0,
	STAX_NOTE_Fs0,
	STAX_NOTE_G0,
	STAX_NOTE_Gs0,
	STAX_NOTE_A0,
	STAX_NOTE_As0,
	STAX_NOTE_B0,
	STAX_NOTE_C1,
	STAX_NOTE_Cs1,
	STAX_NOTE_D1,
	STAX_NOTE_Ds1,
	STAX_NOTE_E1,
	STAX_NOTE_F1,
	STAX_NOTE_Fs1,
	STAX_NOTE_G1,
	STAX_NOTE_Gs1,
	STAX_NOTE_A1,
	STAX_NOTE_As1,
	STAX_NOTE_B1,
	STAX_NOTE_C2,
	STAX_NOTE_Cs2,
	STAX_NOTE_D2,
	STAX_NOTE_Ds2,
	STAX_NOTE_E2,
	STAX_NOTE_F2,
	STAX_NOTE_Fs2,
	STAX_NOTE_G2,
	STAX_NOTE_Gs2,
	STAX_NOTE_A2,
	STAX_NOTE_As2,
	STAX_NOTE_B2,
	STAX_NOTE_C3,
	STAX_NOTE_Cs3,
	STAX_NOTE_D3,
	STAX_NOTE_Ds3,
	STAX_NOTE_E3,
	STAX_NOTE_F3,
	STAX_NOTE_Fs3,
	STAX_NOTE_G3,
	STAX_NOTE_Gs3,
	STAX_NOTE_A3,
	STAX_NOTE_As3,
	STAX_NOTE_B3,
	STAX_NOTE_C4,
	STAX_NOTE_Cs4,
	STAX_NOTE_D4,
	STAX_NOTE_Ds4,
	STAX_NOTE_E4,
	STAX_NOTE_F4,
	STAX_NOTE_Fs4,
	STAX_NOTE_G4,
	STAX_NOTE_Gs4,
	STAX_NOTE_A4,
	STAX_NOTE_As4,
	STAX_NOTE_B4,
	STAX_NOTE_C5,
	STAX_NOTE_Cs5,
	STAX_NOTE_D5,
	STAX_NOTE_Ds5,
	STAX_NOTE_E5,
	STAX_NOTE_F5,
	STAX_NOTE_Fs5,
	STAX_NOTE_G5,
	STAX_NOTE_Gs5,
	STAX_NOTE_A5,
	STAX_NOTE_As5,
	STAX_NOTE_B5,
	STAX_NOTE_C6,
	STAX_NOTE_Cs6,
	STAX_NOTE_D6,
	STAX_NOTE_Ds6,
	STAX_NOTE_E6,
	STAX_NOTE_F6,
	STAX_NOTE_Fs6,
	STAX_NOTE_G6,
	STAX_NOTE_Gs6,
	STAX_NOTE_A6,
	STAX_NOTE_As6,
	STAX_NOTE_B6,
	STAX_NOTE_C7,
	STAX_NOTE_Cs7,
	STAX_NOTE_D7,
	STAX_NOTE_Ds7,
	STAX_NOTE_E7,
	STAX_NOTE_F7,
	STAX_NOTE_Fs7,
	STAX_NOTE_G7,
	STAX_NOTE_Gs7,
	STAX_NOTE_A7,
	STAX_NOTE_As7,
	STAX_NOTE_B7,
	STAX_NOTE_C8,
	STAX_NOTE_Cs8,
	STAX_NOTE_D8,
	STAX_NOTE_Ds8,
	STAX_NOTE_E8,
	STAX_NOTE_F8,
	STAX_NOTE_Fs8,
	STAX_NOTE_G8,
	STAX_NOTE_Gs8,
	STAX_NOTE_A8,
	STAX_NOTE_As8,
	STAX_NOTE_B8,
	STAX_NOTE_MAX,
} stax_note_t;

typedef enum midi_message_type_e {
	NOTE_OFF = 8,
	NOTE_ON = 9,
	CC = 11
} midi_message_type_t;

typedef enum note_status_e {
	STAX_NOTE_OFF = NOTE_OFF,
	STAX_NOTE_ON = NOTE_ON,
} note_status_t;

// typedef struct stax_synth_s {
// 	note_status_t status;
// 	stax_note_t note;
// 	uint32_t frequency;
// 	piezo_volume_t volume;
// 	uint8_t duty_cycle;
// 	midi_message_t last_message;
// } stax_synth_t;


void STAX_SYNTH_note_on(stax_note_t note, bool highVolume);
void STAX_SYNTH_note_off(void);
void STAX_SYNTH_cc(uint8_t cc_num, uint8_t value);

#endif