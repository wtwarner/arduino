
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

void advance(int i, bool on_blank) {
  digitalWrite(on_blank ? tr_blank_pin[i] : tr_pin[i], 1);

  digitalWrite(LED_BUILTIN, 1);
  delay(MIN_DELAY);
  digitalWrite(on_blank ? tr_blank_pin[i] : tr_pin[i], 0);
  digitalWrite(LED_BUILTIN, 0);
}

int digits[4] = {0};

bool to_blank(int i)
{
  int cnt = 12;
  while (-- cnt) {
    byte on_blank = digitalRead(status_pin[i]);
    Serial.print(i); Serial.print(": Blank? "); Serial.println(on_blank);
    if (on_blank) {
      digits[i] = 11;
      return true;
    }
    advance(i, on_blank);
    delay(MIN_DELAY);
  }
  Serial.println("FAIL to_blank");
  return false;
}
void loop() {
  for (int i = 0; i < 2; i++) {
    to_blank(i);
  }
  delay(1000);
  int val[2] = {6, 9};
  for (int i = 0; i < 2; i++) {
    
    while (digits[i] != val[i]) {
      advance(i, digits[i] == 11);
      digits[i] = (digits[i] + 1) % 11;
      Serial.print(i); Serial.print(": = ");
      if (digits[i] == 11) Serial.println("(blank)");
      else Serial.println((digits[i]) % 10);
      delay(MIN_DELAY);
    }
  }
  delay(1000);

}
