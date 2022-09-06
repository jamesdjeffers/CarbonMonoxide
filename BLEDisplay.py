# K96 Modbus Data Trend Acquisition
#
# This program uses the "tkinter" graphical package and built-in Python
# functions to create a multi-threaded GUI that aquires data from an
# external device (K96 sensor from SenseAir) using the Modbus
# serial protocol.
#
# Created by: James D. Jeffers
# Date: 2022/08/29
# The module that contains graphical objects
import tkinter as tk
# Other important Python libraries
import threading
import time
import csv
# Modules for numerical data, time stamps, and file export
import numpy as np
import matplotlib.pyplot as plt

# BLE Libraries
import asyncio
from bleak import BleakScanner
from bleak import BleakClient

address = "75:6F:61:48:18:5F"
MODEL_NBR_UUID = "19B10001-E8F4-537E-4F6C-D104768A1214"

class Application(tk.Frame):
    
    def __init__(self, master=None):         # When the program starts
        tk.Frame.__init__(self, master)      # create window (frame)
        self.pack()                          # Defines window position
        self.createWidgets()                 # Run module createWidgets
        
    def createWidgets(self):
        
        self.clickHere = tk.Button(self)     # Click button for user action
        self.clickHere["text"] = "Start Trend\n(click here)"
        # What happens when you click the button
        self.clickHere["command"] = self.trend_button
        self.clickHere.pack(side="top")      # Place button in the window
     
    async def findBLE(self):
        devices = await BleakScanner.discover()
        for d in devices:
            print(d)
        async with BleakClient(address) as client:
            while(True):
                model_number = await client.read_gatt_char(MODEL_NBR_UUID)
                print(int.from_bytes(model_number, byteorder='little', signed=True))
                #time.sleep(1000)
                
    def trend_button(self):
        asyncio.run(findBLE(self))

root = tk.Tk()
app = Application(master=root)
# Set the application window properties
app.master.title("K96 Data Trend Collection")
app.master.geometry("375x150")
app.master.maxsize(750,200)
app.mainloop()

