
const int tr_pin[4] = {10, 7};
const int tr_blank_pin[4] = {9, 12};
const int status_pin[4] = {8, 11};
const int MIN_DELAY = 180;

void setup() {

  Serial.begin(9600);
  int timeout = 200;
  while (!Serial && --timeout) {
    // wait for serial port to connect. Needed for native USB port only.  Wait
    // 2 seconds at most.
    delay(10);
  }

  Serial.println("Starting");

  // put your setup code here, to run once:
  for (int i = 0; i < 2; i++) {
    pinMode(tr_pin[i], OUTPUT);
    pinMode(tr_blank_pin[i], OUTPUT);
    pinMode(status_pin[i], INPUT);
    digitalWrite(tr_pin[i], 0);
    digitalWrite(tr_blank_pin[i], 0);
  }
  pinMode(LED_BUILTIN, OUTPUT);

}

enum digit_t { DIG_BLANK, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5,
  DIG_6, DIG_7, DIG_8, DIG_9, DIG_0 };
digit_t digits[4] = {DIG_BLANK};
byte dig_to_num[11] = {-1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
byte num_to_dig[10] = {DIG_0, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8, DIG_9};

void advance(int i) {
  const byte on_blank = digitalRead(status_pin[i]);
  digitalWrite(on_blank ? tr_blank_pin[i] : tr_pin[i], 1);

  digitalWrite(LED_BUILTIN, 1);
  delay(MIN_DELAY);
  digitalWrite(on_blank ? tr_blank_pin[i] : tr_pin[i], 0);
  digitalWrite(LED_BUILTIN, 0);
  digits[i] = (digits[i] + 1) % 11;
  delay(MIN_DELAY);
}

bool to_blank(int i)
{
  byte cnt = 12;
  while (-- cnt) {
    byte on_blank = digitalRead(status_pin[i]);
    Serial.print(i); Serial.print(": Blank? "); Serial.println(on_blank);
    if (on_blank) {
      digits[i] = DIG_BLANK;
      return true;
    }
    advance(i);
  }
  Serial.println("FAIL to_blank");
  return false;
}
void loop() {
  for (int i = 0; i < 2; i++) {
    to_blank(i);
  }
  delay(1000);
  byte val[2] = {2, 0};
  for (int i = 0; i < 2; i++) {
    
    while (dig_to_num[digits[i]] != val[i]) {
      advance(i);
      
      Serial.print(i); Serial.print(": = ");
      if (digits[i] == DIG_BLANK) Serial.println("(blank)");
      else Serial.println(dig_to_num[digits[i]]);

    }
  }
  delay(1000);

}
