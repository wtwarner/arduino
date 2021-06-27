//
// IV-4 VFD
//
#include <Wire.h>
#include <stdint.h>
#include "global.h"

constexpr uint32_t BM(int n) { return 1ul << (n-1); }

struct Vfd {
    Vfd():
        deadline(0)
    {}

    enum seg_to_driver_bit_t : uint32_t {
        SEG_2 = BM(1),
        SEG_3 = BM(2),
        SEG_4 = BM(3),
        SEG_5 = BM(4),
        SEG_6 = BM(5),
        SEG_7 = BM(6),
        SEG_8 = BM(7),
        SEG_9 = BM(8),
        SEG_10 = BM(9),
        SEG_13 = BM(10),
        SEG_14 = BM(11),
        SEG_15 = BM(12),
        SEG_16 = BM(13),
        SEG_17 = BM(14),
        SEG_18 = BM(15),
        SEG_19 = BM(16),
        SEG_20 = BM(17),
        SEG_21 = BM(18),
    };

    static const uint32_t vfd_segs_digits[10];
    static const int NUM_NOTES = 7;
    static const uint32_t vfd_segs_notes[NUM_NOTES];
    
    const uint32_t vfd_segs_sharp = SEG_19 | SEG_17 | SEG_15 | SEG_6;
    const uint32_t vfd_segs_flat = SEG_9 | SEG_13 | SEG_15 | SEG_17 | SEG_14;
    const uint32_t vfd_segs_unknown = SEG_19 | SEG_15;
    const uint32_t vfd_segs_left_dec = SEG_10;
    const uint32_t vfd_segs_right_dec = SEG_2;

    void setup();
    void write_vfd(uint32_t vfd1, uint32_t vfd2);
    void write_note(const char *note_name);
    void loop();

    unsigned long deadline;
};
