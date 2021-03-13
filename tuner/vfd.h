//
// IV-4 VFD
//
#include <Wire.h>
#include <stdint.h>
#include "global.h"

struct Vfd {
    Vfd():
        deadline(0)
    {}
           
    enum seg_to_driver_bit_t : uint32_t {
        SEG_2 = 1ul << 19,
        SEG_3 = 1ul << 18,
        SEG_4 = 1ul << 17,
        SEG_5 = 1ul << 16,
        SEG_6 = 1ul << 15,
        SEG_7 = 1ul << 14,
        SEG_8 = 1ul << 13,
        SEG_9 = 1ul << 12,
        SEG_10 = 1ul << 11,
        SEG_13 = 1u << 8,
        SEG_14 = 1u << 7,
        SEG_15 = 1u << 6,
        SEG_16 = 1u << 5,
        SEG_17 = 1u << 4,
        SEG_18 = 1u << 3,
        SEG_19 = 1u << 2,
        SEG_20 = 1u << 1,
        SEG_21 = 1u << 0,
    };

    static const uint32_t vfd_segs_digits[10];
    static const int NUM_NOTES = 7;
    static const uint32_t vfd_segs_notes[NUM_NOTES];
    
    const uint32_t vfd_segs_sharp = SEG_19 | SEG_17 | SEG_15 | SEG_6;
    const uint32_t vfd_segs_flat = SEG_21 | SEG_3 | SEG_19 | SEG_6 | SEG_4;
    const uint32_t vfd_segs_unknown = SEG_19 | SEG_15;
    const uint32_t vfd_segs_left_dec = SEG_2;
    const uint32_t vfd_segs_right_dec = SEG_10;

    void setup();
    void write_vfd(uint32_t vfd1, uint32_t vfd2);
    void write_note(const char *note_name);
    void loop();

    unsigned long deadline;
};
