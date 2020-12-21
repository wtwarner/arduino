#include <ArduinoLog.h>

const int tr_pin[4] = {10, 7};
const int tr_blank_pin[4] = {9, 12};
const int status_pin[4] = {8, 11};
const int MIN_DELAY = 120;

class tr_digit_t {
public:
    tr_digit_t():
        digit(DIG_BLANK),
        deadline_millis(0),
        prev_millis(0),
        name(0)
    {}

     void begin(byte tr_pin_, byte tr_blank_pin_, byte status_pin_, byte nm_) {
        tr_pin = tr_pin_;
        tr_blank_pin = tr_blank_pin_;
        status_pin = status_pin_;
        name = nm_;

        pinMode(tr_pin, OUTPUT);
        pinMode(tr_blank_pin, OUTPUT);
        pinMode(status_pin, INPUT);
        digitalWrite(tr_pin, 0);
        digitalWrite(tr_blank_pin, 0);
    }

    // displayed values, in order of rotation of cylindar
    enum digit_t { DIG_BLANK,
                   DIG_1, DIG_2, DIG_3, DIG_4, DIG_5,
                   DIG_6, DIG_7, DIG_8, DIG_9, DIG_0 };
    // indexed by digit_t; maps to actual value
    const short digit_to_num[11] = {-1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    // indexed by actual value 0..9, maps to digit_t
    static const digit_t num_to_digit[10];

    // returns false if busy
    bool set(digit_t target_) {
        if (act_state == ACT_IDLE) {
            target_digit = target_;
            act_state = ACT_ASSERTED;
            actuate(1);
            Log.trace("%d: start, to ASSERT\n", name);
            return true;
        }
        return false;
    }
    bool is_idle() {
        return act_state == ACT_IDLE;
    }
    // returns false if busy
    bool loop() {
        unsigned long now_millis = millis();
        if (now_millis < prev_millis) { // rollover
            deadline_millis  = now_millis + MIN_DELAY;
        }
        prev_millis = now_millis;

        const bool timeout = (millis() >= deadline_millis);

        switch (act_state) {
            case ACT_IDLE:
                return true;
            case ACT_ASSERTED:
                if (timeout) {
                    act_state = ACT_DEASSERTED;
                    actuate(0);
                    Log.trace("%d: to DEASSERT\n", name);
                }
                break;
            case ACT_DEASSERTED:
                if (timeout) {
                    digit = static_cast<digit_t>((static_cast<int>(digit) + 1) % 11);
                    if (digit == DIG_BLANK) Log.trace("%d: (blank)\n", name);
                    else Log.trace("%d: %d\n", name, digit_to_num[digit]);

                    if (digit == target_digit) {
                        act_state = ACT_IDLE;
                        Log.trace("%d: done, to IDLE\n", name);
                    }
                    else {
                        act_state = ACT_ASSERTED;
                        actuate(1);
                        Log.trace("%d: to ASSERT\n", name);
                    }
                }
                break;
        }

        return false;
    };
    
private:
    digit_t digit;
    byte tr_pin;          // pin to actuate when on non-blank position
    byte tr_blank_pin;    // pin to actuate when on blank position
    byte status_pin;      // pin to query whether on blank position
    enum actuate_state_t {
        ACT_IDLE, ACT_ASSERTED, ACT_DEASSERTED
    };
    actuate_state_t act_state;
    unsigned long deadline_millis;
    unsigned long prev_millis;
    digit_t target_digit;
    byte name;                  // for debug messages

    void actuate(byte on) {
        const byte on_blank = on ? digitalRead(status_pin) : 0; // for off, don't care about blank state
        digitalWrite(on_blank ? tr_blank_pin : tr_pin, on);
        if (on_blank) {
            digit = DIG_BLANK;
        }
        // in 'off', conservatively deassert both pins
        if (!on) {
            digitalWrite(on_blank ? tr_pin : tr_blank_pin, 0);
        }
        Log.verbose("%d: act: %d %s\n", name, on, on_blank ? " (blank)" : "");

        prev_millis = millis();
        if (__builtin_uaddl_overflow(prev_millis, MIN_DELAY, &deadline_millis)) {   // handle rollover of addition
            delay(MIN_DELAY);
            prev_millis = millis();
            deadline_millis = prev_millis + MIN_DELAY;
        }
    }
};

const tr_digit_t::digit_t tr_digit_t::num_to_digit[10] = {DIG_0, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8, DIG_9};

tr_digit_t tr_digits[2];

void setup() {

  Serial.begin(9600);
  int timeout = 200;
  while (!Serial && --timeout) {
    // wait for serial port to connect. Needed for native USB port only.  Wait
    // 2 seconds at most.
    delay(10);
  }

  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.trace("Starting\n");
  for (int i = 0; i < 2; i ++) {
      tr_digits[i].begin(tr_pin[i], tr_blank_pin[i], status_pin[i], i);
  }
  
  pinMode(LED_BUILTIN, OUTPUT);
}

void go_to(tr_digit_t::digit_t vals[2])
{
  for (int i = 0; i < 2; i++) {
      tr_digits[i].set(vals[i]);
  }
  unsigned long start_millis = millis();
  bool idle = false;
  while ((millis() - start_millis) < 5000) {
      idle = true;
      for (int i = 0; i < 2; i++) {
          tr_digits[i].loop();
          idle = idle && tr_digits[i].is_idle();
      }
      if (idle) {
          break;
      }
      delay(10);
  }
  if (!idle) {
      Log.error("FAIL to go_to\n");
  }
}

void loop() {
  Log.trace("To blank\n");
  tr_digit_t::digit_t blanks[2] = {tr_digit_t::DIG_BLANK, tr_digit_t::DIG_BLANK};
  go_to(blanks);
  delay(3000);

  Log.trace("To values\n");
  tr_digit_t::digit_t vals[2] = {tr_digit_t::num_to_digit[6], tr_digit_t::num_to_digit[9]};
  go_to(vals);
  delay(3000);

  Log.trace("To values\n");
  tr_digit_t::digit_t vals2[2] = {tr_digit_t::num_to_digit[3], tr_digit_t::num_to_digit[4]};
  go_to(vals2);
  delay(3000);
}
