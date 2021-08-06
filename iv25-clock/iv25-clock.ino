static const int PIN_CLK = 12;
static const int PIN_SDI = 11;
static const int PIN_STROBE = 10;
static const int PIN_OE_ = 9;

void setup() {
  Serial.begin(9600);
  delay(1500);
  Serial.println("Start");
 pinMode(LED_BUILTIN, OUTPUT);

 pinMode(PIN_CLK, OUTPUT);
 pinMode(PIN_SDI, OUTPUT);
 pinMode(PIN_STROBE, OUTPUT);
 pinMode(PIN_OE_, OUTPUT);
 digitalWrite(PIN_CLK, 0);
 digitalWrite(PIN_SDI, 0);
 digitalWrite(PIN_STROBE, 0);
 digitalWrite(PIN_OE_, 1);
 
}

const uint8_t map_seg_to_bit[7] = {
  5, 6, 1, 0, 4, 3, 2
};

uint16_t cnt = 0;
void loop() {
   uint16_t seg_bit0 = ((cnt/7) == 0) ? (1 << map_seg_to_bit[cnt % 7]) : 0;
   uint16_t seg_bit1 = ((cnt/7) == 1) ? (1 << map_seg_to_bit[cnt % 7]) : 0;
   uint16_t sdi_d = seg_bit0 | (seg_bit1 << 8);
   //uint16_t sdi_d = (1 << map_seg_to_bit[cnt % 7]) << 8;
   Serial.print(cnt); Serial.print(" ");
   Serial.print(sdi_d, HEX); Serial.print("\n");
   for (int i = 0; i < 16; i ++) {
      digitalWrite(PIN_SDI, (sdi_d >> (15 - i)) & 0x1);
      digitalWrite(PIN_CLK, 1);
      digitalWrite(PIN_CLK, 0);
   }
   digitalWrite(PIN_STROBE, 1);
   digitalWrite(PIN_STROBE, 0);
   digitalWrite(PIN_OE_, 0);

   
   delay(250);
   cnt=(cnt+1)%14;
}
