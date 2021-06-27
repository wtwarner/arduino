static const int PIN_CLK = 10;
static const int PIN_STROBE = 8;
static const int PIN_DATA = 11;
static const int PIN_BLANK = 9;


enum seg_to_driver_bit_t: uint32_t {
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
  SEG_20|SEG_14 | SEG_21|SEG_3 | SEG_19|SEG_15 | SEG_13|SEG_9, // A
  SEG_20|SEG_14 | SEG_17|SEG_6 | SEG_13|SEG_9 | SEG_15 | SEG_4|SEG_8,   // B
  SEG_20 | SEG_14 | SEG_4 | SEG_8 | SEG_21 | SEG_3  , // C 
  SEG_20|SEG_14 | SEG_17|SEG_6 | SEG_13|SEG_9 | SEG_4|SEG_8,   // D
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15 | SEG_4 | SEG_8, // E
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_19 | SEG_15, // F
  SEG_20 | SEG_14 | SEG_21 | SEG_3 | SEG_15 | SEG_9 | SEG_4 | SEG_8, // G
};

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_STROBE, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_BLANK, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(PIN_CLK, 0);
  digitalWrite(PIN_STROBE, 0);
  digitalWrite(PIN_DATA, 0);
  digitalWrite(PIN_BLANK, 0);
}

void write_vfd(uint32_t data)
{
  for (int i = 0; i < 20; i ++) {
    digitalWrite(PIN_DATA, (data >> (19 - i) & 0x1));
    digitalWrite(PIN_CLK, 1);
    digitalWrite(PIN_CLK, 0);

  }
  digitalWrite(PIN_STROBE, 1);
  digitalWrite(PIN_STROBE, 0);
}

byte led_state = 0;
int data = 0;
void loop() {

  //while (Serial.available() > 0) {

    // look for the next valid integer in the incoming serial stream:
    //int data = Serial.parseInt();

    // look for the newline. That's the end of your sentence:
    //if (Serial.read() == '\n') {
      if (data < 10)
        write_vfd(vfd_digits[data]);
      else
        write_vfd(vfd_notes[data-10]);
      delay(333);

      digitalWrite(LED_BUILTIN, led_state);
      led_state = 1 - led_state;
      if (++data == 17)
        data = 0;
    //}
  }
