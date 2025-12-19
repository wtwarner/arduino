#!/usr/bin/env python3
from flask import Flask, render_template, request, redirect, url_for
from smbus2 import SMBus
import time, datetime
import atexit
from apscheduler.schedulers.background import BackgroundScheduler

address = 0x08  # I2C slave address of Arduino
bus = SMBus(1)

I2C_CMD_POWER = 0x80
I2C_CMD_PATTERN = 0x81
I2C_CMD_CURTIME = 0x82   # Unix time_t LSB first
I2C_CMD_ONTIME = 0x83    # H then M
I2C_CMD_OFFTIME = 0x84   # H then M
I2C_CMD_ENTIMER = 0x85

app = Flask(__name__)

patterns = ["cirOut1", "cirOut0", "test1", "rotate0", "rotate1", "spiral0", "spiral1"]

def populate_template():
   power = bus.read_byte_data(address, I2C_CMD_POWER)
   [patternL, patternH] = bus.read_i2c_block_data(address, I2C_CMD_PATTERN, 2)
   pattern = (patternH << 8) | patternL;
   timerEnable = bus.read_byte_data(address, I2C_CMD_ENTIMER)
   [onTimeH,onTimeM] = bus.read_i2c_block_data(address, I2C_CMD_ONTIME, 2)
   [offTimeH,offTimeM] = bus.read_i2c_block_data(address, I2C_CMD_OFFTIME, 2)

   templateData = {
	'power': power,
	'timerEnable': timerEnable,
	'onTimeH': onTimeH,
	'onTimeM': onTimeM,
	'offTimeH': offTimeH,
	'offTimeM': offTimeM,
   }
   patternShift = 0
   for p in patterns:
      templateData[p] = 1 if (pattern & (1 << patternShift)) else 0
      patternShift = patternShift + 1
   return render_template('main.html', **templateData)

def getFormField(field, default):
   if field in request.form:
      return int(request.form[field])
   else:
      return default

def send_curtime():
    timezone_aware_dt = datetime.datetime.now(datetime.timezone.utc)
    utime = int(timezone_aware_dt.timestamp() - 8*60*60) # correct timezone
    req = [utime & 0xff, (utime>>8) & 0xff, (utime>>16) & 0xff, (utime>>24) & 0xff]
    bus.write_i2c_block_data(address, I2C_CMD_CURTIME, req)
    app.logger.info("sent curtime")

@app.route('/', methods=['GET', 'POST'])
def main():
   if request.method == 'POST':
     app.logger.info("POST")
     send_curtime()
 
     req = [int(request.form['onTimeH']), int(request.form['onTimeM'])]
     bus.write_i2c_block_data(address, I2C_CMD_ONTIME, req)

     req = [int(request.form['offTimeH']), int(request.form['offTimeM'])]
     bus.write_i2c_block_data(address, I2C_CMD_OFFTIME, req)

     power = getFormField('power', 0)
     bus.write_i2c_block_data(address, I2C_CMD_POWER, [power])

     timerEnable = getFormField('timerEnable', 0)
     bus.write_i2c_block_data(address, I2C_CMD_ENTIMER, [timerEnable])
     patternBitMask = 0
     patternShift = 0
     for pattern in patterns:
        en = getFormField(pattern, 0)
        patternBitMask = patternBitMask | (en << patternShift)
        patternShift = patternShift + 1
     bus.write_i2c_block_data(address, I2C_CMD_PATTERN, [patternBitMask & 0xff, (patternBitMask >> 8) & 0xff])
   return populate_template()

if __name__ == '__main__':
    app.logger.setLevel("WARNING")
    sched = BackgroundScheduler()
    sched.add_job(func=send_curtime, trigger='interval', seconds=5)
    sched.start()
    atexit.register(lambda: sched.shutdown())
    app.run(debug=True, host='0.0.0.0', use_reloader=False)
