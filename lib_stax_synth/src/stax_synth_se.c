#include "stax_synth_se.h"
#include "os_io_seproxyhal.h"
#include "seproxyhal_protocol.h"
#include "os.h"
#include "string.h"


void STAX_SYNTH_note_on(stax_note_t note, bool highVolume) {
	PRINTF("[SE] note on %d\n", note);
	uint8_t buffer[9];
	midi_message_t msg;
	msg.cable = 0; // Dummy value, not used
	msg.code = NOTE_ON;
	msg.message = NOTE_ON;
	msg.channel = STAX_MIDI_CHANNEL;
	msg.messageByte1 = note;
	msg.messageByte2 = highVolume?127:1;

	buffer[0] = SEPROXYHAL_TAG_MIDI;
	buffer[1] = 0;
	buffer[2] = 6;
	memcpy(&buffer[3], &msg, sizeof(midi_message_t));
	io_seproxyhal_spi_send(buffer, 9);
}

void STAX_SYNTH_note_off(void) {
	PRINTF("[SE] note off\n");
	uint8_t buffer[9];
	midi_message_t msg;
	msg.cable = 0; // Dummy value, not used
	msg.code = NOTE_OFF;
	msg.message = NOTE_OFF;
	msg.channel = STAX_MIDI_CHANNEL;
	msg.messageByte1 = 0; // Dummy value, not used
	msg.messageByte2 = 0; // Dummy value, not used

	buffer[0] = SEPROXYHAL_TAG_MIDI;
	buffer[1] = 0;
	buffer[2] = 6;
	memcpy(&buffer[3], &msg, sizeof(midi_message_t));
	io_seproxyhal_spi_send(buffer, 9);
}

void STAX_SYNTH_cc(uint8_t cc_num, uint8_t value) {
	PRINTF("[SE] cc\n");
	uint8_t buffer[9];
	midi_message_t msg;
	msg.cable = 0; // Dummy value, not used
	msg.code = 0; // Dummy value, not used
	msg.message = CC;
	msg.channel = STAX_MIDI_CHANNEL;
	msg.messageByte1 = cc_num;
	msg.messageByte2 = value;

	buffer[0] = SEPROXYHAL_TAG_MIDI;
	buffer[1] = 0;
	buffer[2] = 6;
	memcpy(&buffer[3], &msg, sizeof(midi_message_t));
	io_seproxyhal_spi_send(buffer, 9);
}