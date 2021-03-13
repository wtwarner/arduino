

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

static const uint32_t vfd_digits[10] = {
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

static const uint32_t vfd_notes[7] = {
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_13 | SEG_9, // A
  SEG_20 | SEG_14 | SEG_17 | SEG_6 | SEG_13 | SEG_9 | SEG_15 | SEG_4 | SEG_8, // B
  SEG_20 | SEG_14 | SEG_4 | SEG_8 | SEG_21 | SEG_3  , // C
  SEG_20 | SEG_14 | SEG_17 | SEG_6 | SEG_13 | SEG_9 | SEG_4 | SEG_8, // D
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_4 | SEG_8, // E
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15, // F
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_15 | SEG_9 | SEG_4 | SEG_8, // G
};

const uint32_t SEG_LEFT_DEC = SEG_2;
const uint32_t SEG_RIGHT_DEC = SEG_10;
const uint32_t SEG_UNKNOWN = SEG_19 | SEG_15;

void vfd_setup() {


  //pinMode(VFD_PIN_CLK, OUTPUT);
  pinMode(VFD_PIN_STROBE, OUTPUT);
  //pinMode(VFD_PIN_DATA, OUTPUT);
  pinMode(VFD_PIN_BLANK, OUTPUT);


  //digitalWrite(VFD_PIN_CLK, 0);
  digitalWrite(VFD_PIN_STROBE, 0);
  //digitalWrite(VFD_PIN_DATA, 0);
  analogWrite(VFD_PIN_BLANK, 0);

  write_vfd(SEG_UNKNOWN);
}

void write_vfd(uint32_t data)
{
  for (int i = 0; i < 20; i ++) {
    digitalWrite(VFD_PIN_DATA, (data >> (19 - i) & 0x1));
    digitalWrite(VFD_PIN_CLK, 1);
    digitalWrite(VFD_PIN_CLK, 0);

  }
  digitalWrite(VFD_PIN_STROBE, 1);
  digitalWrite(VFD_PIN_STROBE, 0);
}

void vfd_write_note(const char *note_name)
{
  uint32_t segments = SEG_UNKNOWN;
  unsigned char vfd_note = note_name[0] - 'A';
  if (vfd_note <= 7)
    segments = vfd_notes[vfd_note];
  if (note_name[1] == '#')
    segments |= SEG_RIGHT_DEC;
  else if (note_name[1] == 'b')
    segments |= SEG_LEFT_DEC;
  write_vfd(segments); 
}
