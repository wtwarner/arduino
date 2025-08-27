# AdaFruit Itsy Bitsy M0 Express
# boost converter PWM generator
# uses pin D5 which is high-voltage (5V) to drive MOSFET switch.

import board
import pwmio
import time
from digitalio import DigitalInOut, Direction, Pull

if hasattr(board, "APA102_SCK"):
    import adafruit_dotstar
    rgbled = adafruit_dotstar.DotStar(board.APA102_SCK, board.APA102_MOSI, 1)
else:
    import neopixel
    rgbled = neopixel.NeoPixel(board.NEOPIXEL, 1)
rgbled.brightness = 0.5

# Built in red LED
led = DigitalInOut(board.D13)
led.direction = Direction.OUTPUT


# PWM
pwm = pwmio.PWMOut(board.D5, frequency=42000)
pwm.duty_cycle = int(.5 * 65535)

######################### MAIN LOOP ##############################

led.value = 1
delta = -0.001
while True:
  time.sleep(.025)
  led.value = not led.value
  if (pwm.duty_cycle < .05*65535 and delta < 0) or (pwm.duty_cycle > .60*65535 and delta > 0):
      delta = delta * -1
  pwm.duty_cycle = pwm.duty_cycle + int(delta * 65535)
  rgbled[0] = (int(255 * pwm.duty_cycle/65535.0), 10, int(255 * (65535-pwm.duty_cycle)/65535.0))


