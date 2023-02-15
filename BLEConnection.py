# Bluetooth Data Trend Acquisition
#
# This program uses the "Bleak" Bluetooth Low Enengry Python library
# with asychronous data transfer to aquires data from an
# external device (CO sensor from Winsen) using the Modbus
# serial protocol.
#
# Created by: James D. Jeffers
# Created: 2022/08/29, Updated: 2022/11/14

import time
import csv
# Modules for numerical data, time stamps, and file export
import struct
import numpy as np
import matplotlib.pyplot as plt

# BLE Libraries
import asyncio
from bleak import BleakScanner
from bleak import BleakClient

address   = "F6:5E:11:F7:A2:AD"
address00 = "75:6F:61:48:18:5F"
address01 = "73:EF:88:BD:4F:2F"
address02 = "B9:80:BC:09:FC:0B"
address03 = "95:D6:59:B0:AD:9F"
address04 = "35:B2:AC:4F:B0:1B"
address05 = "62:52:CF:3F:A8:A9"
address06 = "8E:7C:C3:04:A4:20"

CO_CONCENTRATION_UUID = "00002BD0-0000-1000-8000-00805F9B34FB"
CO_TEST_UUID = "00002AF4-0000-1000-8000-00805F9B34FB"
CO_STATUS_UUID = "00002BBB-0000-1000-8000-00805F9B34FB"

timeArray = np.zeros(10000)
dataArray = np.zeros((2,10000))

fileName = 'CO_trend_'
fileExtension = '.csv'
    
async def findBLE(self):
    devices = await BleakScanner.discover()
    for d in devices:
        print(d)
    startTime = time.time()
    iteration = 0
    fileOutput = fileName + fileExtension
    async with BleakClient(address00) as client:
        #async with BleakClient(address01) as client2:
            while(True):
                co_concentration = await client.read_gatt_char(CO_CONCENTRATION_UUID)
                data_num = int.from_bytes(co_concentration, byteorder='little', signed=True)
                print(data_num)
                timeArray[iteration] = time.time()- startTime
                dataArray[0,iteration] = data_num
                #model_number = await client2.read_gatt_char(CO_CONCENTRATION_UUID)
                #data_num = int.from_bytes(model_number, byteorder='little', signed=True)
                #print(data_num)
                #dataArray[1,iteration] = data_num
                co_concentration = await client.read_gatt_char(CO_TEST_UUID)
                data_num = int.from_bytes(co_concentration, byteorder='little', signed=True)
                #print(data_num)
                dataArray[1,iteration] = data_num
        
                with open(fileOutput, 'w', newline='') as f:
                    writer = csv.writer(f)
                    for i in range(iteration):
                        dArray = np.zeros(0)
                        dArray = np.append(dArray,timeArray[i])
                        dArray = np.append(dArray,dataArray[0,i])
                        dArray = np.append(dArray,dataArray[1,i])
                        writer.writerow(dArray)
                    f.close()
                    
                iteration = iteration + 1
                #time.sleep(1000)


asyncio.run(findBLE(address))

