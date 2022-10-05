# Sensor Data Trend Acquisition
#
# This program uses the "tkinter" graphical package and built-in Python
# functions to create a multi-threaded GUI that aquires data from an
# external BLE device. Searches for specific services and characteristics
# then stores data in a text ".csv" file
#
# Created by: James D. Jeffers
# Date: 2022/08/29

# The module that contains graphical objects
import tkinter as tk
# Other important Python libraries
import threading
import time
import csv
import logging
# Modules for numerical data, time stamps, and file export
import numpy as np
import matplotlib.pyplot as plt
# Serial
import serial

# BLE Libraries
import asyncio
from bleak import BleakScanner
from bleak import BleakClient

address0 = "75:6F:61:48:18:5F"
address1 = "8E:7C:C3:04:A4:20"
address2 = "F6:5E:11:F7:A2:AD"

ENV_SENSE_UUID = "19B10001-E8F4-537E-4F6C-D104768A1214"
#ENV_SENSE_UUID = "2BD0"

fileName = "CO_trend"
fileExt = ".csv"

logger = logging.getLogger(__name__)

class Application(tk.Frame):
    
    def __init__(self, master=None):         # When the program starts
        tk.Frame.__init__(self, master)      # create window (frame)
        self.pack()                          # Defines window position
        self.createWidgets()                 # Run module createWidgets

        self.ser = serial.Serial(port='/COM3',
                            baudrate = 9600,
                            timeout=2)
        
        self.timeArray = np.zeros(30000)
        self.dataArray = np.zeros(30000)
        self.iteration = 0
        self.trendOn = False
        self.threads = list()
        
    def createWidgets(self):
        
        self.clickHere = tk.Button(self)     # Click button for user action
        self.clickHere["text"] = "Start Trend\n(click here)"
        # What happens when you click the button
        self.clickHere["command"] = self.trend_button
        self.clickHere.pack(side="top")      # Place button in the window

        self.dataEntry = tk.Entry(self, justify=tk.CENTER, font=('Times','12'), width = 5)
        self.dataValue = tk.StringVar()
        self.dataValue.set("--")
        self.dataEntry.pack(side="top")
        self.dataEntry["textvariable"] = self.dataValue
        self.dataValue.set("**")
     
    async def findBLE(self, devAddress):
        async with BleakClient(devAddress) as client:
            logger.info(f"Connected: {client.is_connected}")    
            startTime = time.time()
            while(self.trendOn):
                model_number = await client.read_gatt_char(ENV_SENSE_UUID)
                self.timeArray[self.iteration] = time.time()- startTime
                self.dataArray[self.iteration] = int.from_bytes(model_number, byteorder='little', signed=True)
                self.dataValue.set(self.dataArray[self.iteration])
                self.iteration = self.iteration +1
                if self.timeArray[self.iteration-1] > 120:
                    self.trendOn = False
                    self.ser.write(b'\x30')
                    await client.disconnect()
                
    def trend_button(self):
        if self.trendOn:
           self.trendOn = False
           self.ser.write(b'\x30')
        else:
            
            self.trendOn = True
            trendThread = threading.Thread(target=self.trend_thread, args=())
            self.threads.append(trendThread)
            trendThread.start()
            #asyncio.run(self.findBLE())

    def trend_thread(self):
        while(True):
            try:
                self.ser.write(b'\x31')
                asyncio.run(self.findBLE(address0))
            except:
                logger.info("Device 1 Complete")
            self.trend_save()
            
            try:
                self.ser.write(b'\x32')
                asyncio.run(self.findBLE(address1))
            except:
                logger.info("Device 2 Complete")
            self.trend_save()
            try:
                self.ser.write(b'\x33')
                asyncio.run(self.findBLE(address2))
            except:
                logger.info("Device 2 Complete")
            self.trend_save()
            time.sleep(120)
            #time.sleep(180)

    def trend_save(self):
        
        with open(fileName + fileExt, 'w', newline='') as f:
            writer = csv.writer(f)
            for i in range(self.iteration):
                dArray = np.zeros(0)
                dArray = np.append(dArray,self.timeArray[i])
                dArray = np.append(dArray,self.dataArray[i])
                writer.writerow(dArray)
            f.close()
        self.trendOn = True

logging.basicConfig(level=logging.INFO)
root = tk.Tk()
app = Application(master=root)
# Set the application window properties
app.master.title("OU NPL CO Data Trend Collection")
app.master.geometry("500x200")
app.master.maxsize(750,200)
app.mainloop()

