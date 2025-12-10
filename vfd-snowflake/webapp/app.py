#!/usr/bin/env python3
from flask import Flask, render_template, request
import smbus

address = 0x08
bus = smbus.SMBus(1)

app = Flask(__name__)

@app.route('/')
def main():

   templateData = {
     
      }
   # Pass the template data into the template main.html and return it to the user
   return render_template('main.html', **templateData)

# The function below is executed when someone requests a URL with the pin number and action in it:
@app.route("/<changePin>/<action>")
def action(changePin, action):
   if action == "on":
      bus.write_i2c_block_data(address, 0x80, [1])
   if action == "off":
      bus.write_i2c_block_data(address, 0x80, [0])
   templateData = {
      
   }

   return render_template('main.html', **templateData)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
