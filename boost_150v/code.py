# AdaFruit Itsy Bitsy M0 Express
# boost converter PWM generator
# uses pin D5 which is high-voltage (5V) to drive MOSFET switch.

import board
import pwmio
import time
import usb_cdc
import analogio
import digitalio

pin_hv = analogio.AnalogIn(board.A5)
# 34v: 7329
# 62v: 12995
# 80v: 16548
# 105v: 20757
# 140v: 27814

if hasattr(board, "APA102_SCK"):
    import adafruit_dotstar
    rgbled = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)
else:
    import neopixel
    rgbled = neopixel.NeoPixel(board.NEOPIXEL, 1)
rgbled.brightness = 0.25

# Built in red LED
led = digitalio.DigitalInOut(board.D13)
led.direction = digitalio.Direction.OUTPUT


# PWM
freq = 30000
duty = .5
pwm = pwmio.PWMOut(board.D5, frequency=freq, variable_frequency=True)
pwm.frequency = int(freq)
pwm.duty_cycle = int(duty * 65535)

######################### MAIN LOOP ##############################

led.value = 1
delta = -0.002
print("start")
line=""
ramp=True
while True:
  time.sleep(.025)
  led.value = not led.value
  if (pwm.duty_cycle < .05*65535 and delta < 0) or (pwm.duty_cycle > .50*65535 and delta > 0):
      delta = delta * -1
  if ramp:
      duty = duty + delta

  hv_sum = 0
  for _ in range(6):
      hv_sum += pin_hv.value
      time.sleep(.001)
  # emergency stop
  v=int(.005 * hv_sum/6.0)
  #print((v,))
  if v > 190:
      duty = .1
      print("STOP!")

  pwm.duty_cycle = int(duty * 65535)

  rgbled[0] = (int(255 * pwm.duty_cycle/65535.0), 10, int(255 * (65535-pwm.duty_cycle)/65535.0))

  if usb_cdc.console.in_waiting:
    c = usb_cdc.console.read(1).decode('utf-8')
    line = line + c
    if c == '\n':
        if line[0] == 'f':
            freq = int(line[1:])
            print("freq ", freq)
            pwm.frequency = freq
        elif line[0] == 'd':
            duty = float(line[1:])/100.0
            print("duty ", duty)
        elif line[0] == 'r':
            ramp = not ramp
        elif line[0] == 'v':
            v=pin_hv.value
            print("v:", v, int(.005*v),"V")
        line=""
    elif c:
        print("["+line+"]..")

