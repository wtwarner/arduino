'''

    Send command to Arduino + HC-08 BLE module

    - Service UUID 'FFE1'
    - Characteristics UUID 'FFE0'

'''

# Core Bluetooth module
import cb

# iOS UI module
import ui

# Time
import time


# Delegate handles all BLE events
class MyCentralManagerDelegate(object):
    def __init__(self):
        self.peripheral = None
        self.characteristic = None
        self.button = None

        # UI view
        self.view = ui.View()
        self.view.name = 'BLE Toggle'
        self.view.background_color = '#dddddd'
        print(self)
        print(self.view)

        # Set Time button
        self.button = ui.Button(title='**** LOADING ****')
        self.button.name = "set_time_button"
        self.button.border_width = 1
        self.button.bg_color = '#DDDDDD'
        self.button.font_color = 'black'
        self.button.enabled = False
        self.button.center = (self.view.width * 0.5, self.view.height * 0.3)
        self.button.flex = 'LRTB'
        self.button.action = self.button_click
        self.view.add_subview(self.button)

        # display brightness Slider
        self.brightness_label = ui.Label(frame=(0,0,self.view.width * .4, self.view.height * .1), flex='LRTB', text='Brightness')
        self.brightness_label.center = (self.view.width * 0.1, self.view.height * 0.4)
        self.view.add_subview(self.brightness_label)
        
        self.brightness_slider = ui.Slider()
        self.brightness_slider.name = 'brightness'
        self.brightness_slider.enabled = False
        self.brightness_slider.center = (self.view.width * 0.5, self.view.height * 0.4)
        self.brightness_slider.continuous = True
        self.brightness_slider.flex = 'LRTB'
        self.brightness_slider.action = self.brightness_slider_click
        self.view.add_subview(self.brightness_slider)

        # LED Brightness Slider
        self.led_brightness_label = ui.Label(frame=(0,0,self.view.width * .4, self.view.height * .1), flex='LRTB', text='LED Brightness')
        self.led_brightness_label.center = (self.view.width * 0.3, self.view.height * 0.5)
        self.view.add_subview(self.led_brightness_label)
        
        self.led_brightness_slider = ui.Slider()
        self.led_brightness_slider.name = 'led_brightness'
        self.led_brightness_slider.enabled = False
        self.led_brightness_slider.center = (self.view.width * 0.5, self.view.height * 0.5)
        self.led_brightness_slider.continuous = True
        self.led_brightness_slider.flex = 'LRTB'
        self.led_brightness_slider.action = self.led_brightness_slider_click
        self.view.add_subview(self.led_brightness_slider)

        # Testmode Slider
        self.testmode_label = ui.Label(frame=(0,0,self.view.width * .4, self.view.height * .1), flex='LRTB', text='Test Mode')
        self.testmode_label.center = (self.view.width * 0.3, self.view.height * 0.7)
        self.view.add_subview(self.testmode_label)
        
        self.testmode_slider = ui.Slider()
        self.testmode_slider.name = 'test_mode'
        self.testmode_slider.enabled = False
        self.testmode_slider.center = (self.view.width * 0.5, self.view.height * 0.7)
        self.testmode_slider.continuous = True
        self.testmode_slider.flex = 'LRTB'
        self.testmode_slider.action = self.testmode_slider_click
        self.view.add_subview(self.testmode_slider)

        # Get-time button (debug)
        self.gt_button = ui.Button(title='Dbg: Get time')
        self.gt_button.name = "gt_button"
        self.gt_button.border_width = 1
        self.gt_button.bg_color = '#DDDDDD'
        self.gt_button.font_color = 'black'
        self.gt_button.enabled = False
        self.gt_button.center = (self.view.width * 0.5, self.view.height * 0.8)
        self.gt_button.flex = 'LRTB'
        self.gt_button.action = self.gt_button_click
        self.view.add_subview(self.gt_button)

        # Show the view
        # On the iPad could be Sheet, Popover or Fullscreen
        # iPhone only has fullscreen so ignores the parameter
        self.view.present('sheet')

    def button_click(self, sender):
        self.send_string(time.strftime("st %Y-%m-%dT%H:%M:%S"))

    def brightness_slider_click(self, sender):
        self.send_string("b {:d}".format(int(sender.value * 255)))

    def led_brightness_slider_click(self, sender):
        self.send_string("nb {:d}".format(int(sender.value * 255)))

    def testmode_slider_click(self, sender):
        self.send_string("tm {:d}".format(int(sender.value * 2)))

    def gt_button_click(self, sender):
        self.send_string("gt")

    # Device discovered
    def did_discover_peripheral(self, p):
        if p.name == 'Clock' and not self.peripheral:
            print('Discovered ' + p.name)
            self.peripheral = p
            cb.connect_peripheral(self.peripheral)

    # Connected
    def did_connect_peripheral(self, p):
        print('Connected Peripheral ' + p.name)
        print('Looking for Service FFE0')
        p.discover_services()

    # Services discovered
    def did_discover_services(self, p, error):
        for s in p.services:
            if s.uuid == 'FFE0':
                print('Found Service ' + s.uuid)
                print('Looking for Characteristic FFE1')
                p.discover_characteristics(s)

    # Characteristics discovered
    def did_discover_characteristics(self, s, error):
        for c in s.characteristics:
            if c.uuid == 'FFE1':
                print('Found Characteristic ' + c.uuid)
                self.characteristic = c
                print(self.button)
                self.button.enabled = True
                self.button.title = 'Set Time'
                self.brightness_slider.enabled = True
                self.led_brightness_slider.enabled = True
                self.testmode_slider.enabled = True
                self.gt_button.enabled = True

    # Send strings
    def send_string(self, string_to_send):
        self.peripheral.write_characteristic_value(self.characteristic, "[" + string_to_send + "]", False)


# Initialize
cb.set_central_delegate(MyCentralManagerDelegate())
cb.scan_for_peripherals()
