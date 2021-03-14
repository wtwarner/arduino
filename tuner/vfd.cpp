#include "vfd.h"

const uint32_t Vfd::vfd_segs_digits[10] = {
    SEG_20 | SEG_14 | SEG_4 | SEG_8 | SEG_21 | SEG_3 | SEG_13 | SEG_9,
    SEG_17 | SEG_6,
    SEG_20 | SEG_14 | SEG_13 | SEG_19 | SEG_15 | SEG_3 | SEG_4 | SEG_8,
    SEG_20 | SEG_14 | SEG_13 | SEG_9 | SEG_19 | SEG_15 | SEG_4 | SEG_8,
    SEG_21 | SEG_19 | SEG_15 | SEG_13 | SEG_9,
    SEG_20 | SEG_14 | SEG_21 | SEG_19 | SEG_15 | SEG_9 | SEG_4 | SEG_8,
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_9 | SEG_4 | SEG_8,
    SEG_20 | SEG_14 | SEG_13 | SEG_9,
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_13 | SEG_9 | SEG_4 | SEG_8,
    SEG_20 | SEG_14 | SEG_21 | SEG_19 | SEG_15 | SEG_13 | SEG_9 | SEG_4 | SEG_8,
};

const uint32_t Vfd::vfd_segs_notes[NUM_NOTES] = {
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_13 | SEG_9, // A
    SEG_20 | SEG_14 | SEG_17 | SEG_6 | SEG_13 | SEG_9 | SEG_15 | SEG_4 | SEG_8, // B
    SEG_20 | SEG_14 | SEG_4 | SEG_8 | SEG_21 | SEG_3  , // C
    SEG_20 | SEG_14 | SEG_17 | SEG_6 | SEG_13 | SEG_9 | SEG_4 | SEG_8, // D
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_4 | SEG_8, // E
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15, // F
    SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_15 | SEG_9 | SEG_4 | SEG_8, // G
};

void Vfd::setup() {
    // clk and data are shared with dac; not init here.
    pinMode(VFD_PIN_STROBE, OUTPUT);
    pinMode(VFD_PIN_BLANK, OUTPUT);
    
    digitalWrite(VFD_PIN_STROBE, 0);
    analogWrite(VFD_PIN_BLANK, 0);
    
    write_vfd(vfd_segs_unknown, vfd_segs_unknown);
}


void Vfd::write_vfd(uint32_t vfd1, uint32_t vfd2)
{
    // vfd shares pins with in-9 so must exclude in-9 ISR
    noInterrupts();

    for (int i = 0; i < 20; i ++) {
        digitalWrite(VFD_PIN_DATA, (vfd2 >> (19 - i) & 0x1));
        digitalWrite(VFD_PIN_CLK, 1);
        digitalWrite(VFD_PIN_CLK, 0);
    }
    for (int i = 0; i < 20; i ++) {
        digitalWrite(VFD_PIN_DATA, (vfd1 >> (19 - i) & 0x1));
        digitalWrite(VFD_PIN_CLK, 1);
        digitalWrite(VFD_PIN_CLK, 0);
    }
    digitalWrite(VFD_PIN_STROBE, 1);
    digitalWrite(VFD_PIN_STROBE, 0);

    interrupts();
}

void Vfd::write_note(const char *note_name)
{
    const int vfd_note = note_name[0] - 'A';
    uint32_t segment_left = vfd_segs_unknown;
    uint32_t segment_right = vfd_segs_unknown;
    if (vfd_note < NUM_NOTES) {
        segment_left = vfd_segs_notes[vfd_note];
        segment_right = 0;
        if (note_name[1] == '#') {
            segment_left |= vfd_segs_right_dec;
            segment_right = vfd_segs_sharp;
        }
        else if (note_name[1] == 'b') {
            segment_left |= vfd_segs_left_dec;
            segment_right = vfd_segs_flat;
        }
        deadline = millis() + 1000;
    }
    
    write_vfd(segment_left, segment_right);
    //write_vfd(segment_right, segment_left); 
}


void Vfd::loop() {
    if (deadline < millis()) {
        write_vfd(vfd_segs_unknown, vfd_segs_unknown);
        deadline = millis() + 1000;
    }
}
