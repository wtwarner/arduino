#!/usr/bin/env python3
from flask import Flask, render_template, request, redirect, url_for
from smbus2 import SMBus
import time, datetime

address = 0x08
bus = SMBus(1)

app = Flask(__name__)

def populate_template():
   power = bus.read_byte_data(address, 0x80)
   pattern = bus.read_byte_data(address, 0x81)

   templateData = {
	'power': power,
	'pattern': pattern
   }
   return render_template('main.html', **templateData)

@app.route('/', methods=['GET', 'POST'])
def main():
   if request.method == 'POST':
     timezone_aware_dt = datetime.datetime.now(datetime.timezone.utc)
     utime = int(timezone_aware_dt.timestamp() - 8*60*60)
     req = [utime & 0xff, (utime>>8) & 0xff, (utime>>16) & 0xff, (utime>>24) & 0xff]
     bus.write_i2c_block_data(address, 0x82, req)

     req = [int(request.form['onTimeH']), int(request.form['onTimeM'])]
     bus.write_i2c_block_data(address, 0x83, req)

     req = [int(request.form['offTimeH']), int(request.form['offTimeM'])]
     bus.write_i2c_block_data(address, 0x84, req)

   return populate_template()

@app.route("/power/<action>")
def power_action(action):
   print("power")
   if action == "on":
      bus.write_i2c_block_data(address, 0x80, [1])
   if action == "off":
      bus.write_i2c_block_data(address, 0x80, [0])
   return redirect(url_for('main'))

@app.route("/pattern/<action>")
def pattern_action(action):
   print("pattern")
   if action == "all":
      bus.write_i2c_block_data(address, 0x81, [255])
   else:
      bus.write_i2c_block_data(address, 0x81, [int(action)])

   return redirect(url_for('main'))

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
