# Winsen Data Trend Acquisition
#
# This program uses the "Bleak" Bluetooth Low Enengry Python library
# with asychronous data transfer to aquires data from an
# external device (CO sensor from Winsen) using the Modbus
# serial protocol.
#
# Created by: James D. Jeffers
# Date: 2022/08/29

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
#address = "8E:7C:C3:04:A4:20"

MODEL_NBR_UUID = "19B10001-E8F4-537E-4F6C-D104768A1214"
CO_SERIAL_UUID = "19B10001-E8F5-537E-4F6C-D104768A1214"

timeArray = np.zeros(10000)
dataArray = np.zeros((2,10000))
    
async def findBLE(self):
    devices = await BleakScanner.discover()
    for d in devices:
        print(d)
    startTime = time.time()
    iteration = 0
    async with BleakClient(address) as client:
        while(True):
            model_number = await client.read_gatt_char(MODEL_NBR_UUID)
            data_num = int.from_bytes(model_number, byteorder='little', signed=True)
            print(data_num)
            timeArray[iteration] = time.time()- startTime
            dataArray[0,iteration] = data_num
            #model_number = await client.read_gatt_char(CO_SERIAL_UUID)
            #data_num = int.from_bytes(model_number, byteorder='little', signed=True)
            #print(data_num)
            #dataArray[1,iteration] = data_num
    
            with open('CO_trend.csv', 'w', newline='') as f:
                writer = csv.writer(f)
                for i in range(iteration):
                    dArray = np.zeros(0)
                    dArray = np.append(dArray,timeArray[i])
                    dArray = np.append(dArray,dataArray[0,i])
                    #dArray = np.append(dArray,dataArray[1,i])
                    writer.writerow(dArray)
                f.close()
                
            iteration = iteration + 1
            #time.sleep(1000)


asyncio.run(findBLE(address))

