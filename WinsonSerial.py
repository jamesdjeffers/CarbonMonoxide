# K96 Modbus Data Trend Acquisition
#
# This program uses the "tkinter" graphical package and built-in Python
# functions to create a multi-threaded GUI that aquires data from an
# external device (K96 sensor from SenseAir) using the Modbus
# serial protocol.
#
# Created by: James D. Jeffers
# Date: 2022/05/02

# The module that contains graphical objects
import tkinter as tk
# Other important Python libraries
import threading
# Modules for numerical data, time stamps, and file export
import numpy as np
import matplotlib.pyplot as plt
import time
import csv
# Modbus functions for serial port operation
import serial

class Application(tk.Frame):
    
    def __init__(self, master=None):         # When the program starts
        tk.Frame.__init__(self, master)      # create window (frame)
        self.pack()                          # Defines window position
        self.commands = [b'\xFF\x01\x86\x00\x00\x00\x00\x00\x79',
                         b'\xFF\x01\x78\x41\x00\x00\x00\x00\x46',
                         b'\xFF\x01\x78\x40\x00\x00\x00\x00\x47',
                         b'\xFF\x01\x78\x41\x00\x00\x00\x00\x46',
                         b'\xFF\x01\x86\x00\x00\x00\x00\x00\x79']
        self.labels = ['LPL Conc','SPL Conc','MPL Conc','Pressure','T0','T1','T2','Humid','T3']
        self.createWidgets()                 # Run module createWidgets
        
        self.ser = serial.Serial(port='COM12',
                            baudrate = 9800,
                            parity=serial.PARITY_NONE,
                            stopbits=serial.STOPBITS_ONE,
                            bytesize=serial.EIGHTBITS,
                            xonxoff= 0,
                            rtscts= 0,
                            timeout=2)
        
        # Trend variables
        self.trendOn = False
        self.startTime = 0
        self.iteration = 0
        self.delayTime = 1
        self.timeArray = np.zeros(10000)
        self.dataArray = np.zeros((9,10000))
        self.threads = list()

    def createWidgets(self):
        
        self.clickHere = tk.Button(self)     # Click button for user action
        self.clickHere["text"] = "Start Trend\n(click here)"
        # What happens when you click the button
        self.clickHere["command"] = self.trend_button
        self.clickHere.pack(side="top")      # Place button in the window
        
        # A widget that displays text and allows you to type
        self.text_here = tk.Entry(self, justify=tk.CENTER, font=('Times','20'))
        # Set postition but also set the width relative to the window
        # tkinter has its own string variable (text) type
        self.contents = tk.StringVar()
        # Initialize the variable with text
        self.contents.set("No trend data")
        # Put the text variable into the widget
        self.text_here["textvariable"]= self.contents

        self.optionFrame = tk.Frame(self)    # Container for password options
        self.optionFrame.pack(side="top", expand=1)
        # A widget that displays text and allows you to type
        self.saveFile = tk.Button(self.optionFrame)
        # Set postition but also set the width relative to the window
        self.saveFile.pack(side="right", expand=1)
        # tkinter has its own string variable (text) type
        self.saveFile["text"] = "Save File"
        self.saveFile["command"] = self.save_file
        
        # Autosave Time Option: length (default = 100)
        self.autoSaveOption = tk.Entry(self.optionFrame, justify=tk.CENTER)
        self.autoSaveOptionLabel=tk.Label(self.optionFrame, text="Autosave Data Interval")
        self.autoSaveOptionLabel.pack(side="left",expand=1)
        self.autoSaveOption.pack(side="left",expand=1)
        self.autoSaveInterval = tk.IntVar()
        self.autoSaveInterval.set(100)
        self.autoSaveOption["textvariable"] = self.autoSaveInterval

        # Include special characters
        self.autoSaveFrame = tk.Frame(self.optionFrame)
        self.autoSaveFrame.pack(after=self.saveFile,side="bottom")
        self.autoSaveOption = tk.Checkbutton(self.autoSaveFrame,
                              text="Autosave data option",
                              command=self.autosave_option)
        self.autoSaveOption.pack(side="left")
        self.autoSaveOptionOn = tk.IntVar()
        self.autoSaveOption["variable"] = self.autoSaveOptionOn
        self.autoSaveFileDisplay = tk.Entry(self.autoSaveFrame)
        self.autoSaveFileDisplay.pack(side="right")
        self.autoSaveFilename = tk.StringVar()
        self.autoSaveFilename.set("trend")
        self.autoSaveFileDisplay["textvariable"] = self.autoSaveFilename

        self.dataFrame = tk.Frame(self)
        self.dataFrame.pack(after=self.optionFrame,side="top",expand=1)
        self.dataViewHolder = list()
        self.dataView = list()
        labelText = "Test"
        #self.dataViewBundle = list()
        for i in range(9):
            #self.dataViewBundle.append(tk.Frame(self))
            #self.dataViewHolder.append(tk.Entry(self.dataViewBundle[i], justify=tk.CENTER, font=('Times','12'), width = 5))
            self.dataViewHolder.append(tk.Entry(self.dataFrame, justify=tk.CENTER, font=('Times','12'), width = 5))
            self.dataView.append(tk.StringVar())
            #self.dataViewLabel=tk.Label(self.dataViewBundle[i], text="Label")
            self.dataViewLabel=tk.Label(self.dataFrame, text=self.labels[i])
            self.dataViewLabel.pack(side="left")
            self.dataView[i].set(i)
            self.dataViewHolder[i]["textvariable"]=self.dataView[i]
            self.dataViewHolder[i].pack(side="left")
        self.text_here.pack(after=self.dataFrame)
        
    def save_file(self):
        with open('trend.csv', 'w', newline='') as f:
            writer = csv.writer(f)
            for i in range(self.iteration):
                dArray = np.zeros(0)
                dArray = np.append(dArray,self.timeArray[i])
                for j in range(9):
                    dArray = np.append(dArray,self.dataArray[j,i])
                writer.writerow(dArray)
            f.close()

    def autosave_option(self):
        arrayMin = np.amin(self.dataArray[:,:self.iteration-1],axis=1)
        arrayMax = np.amax(self.dataArray,axis=1)
        #print(arrayMin)
        #print(arrayMax)
        plt.axis([0,self.timeArray[self.iteration-1],arrayMin[0]-1,arrayMax[0]+1])
        for i in range(self.iteration):
            plt.scatter(self.timeArray[i],self.dataArray[0,i])
        plt.pause(0.1)
     
    def trend_button(self):
        if self.trendOn:
           self.trendOn = False
           self.clickHere["text"] = "Start Trend\n(click here)"
        else:
            self.clickHere["text"] = "Stop Trend\n(click here)"
            self.startTime = time.time()
            print("Trend Started")
            self.trendOn = True
       
            trendThread = threading.Thread(target=self.trend_update, args=())
            self.threads.append(trendThread)
            trendThread.start()

    def trend_update(self):
        while self.trendOn:
            self.timeArray[self.iteration] = time.time()-self.startTime
            
            self.contents.set(self.timeArray[self.iteration])
            for i in range(5):
                self.ser.reset_input_buffer()
                time.sleep(0.01)
                self.ser.write(self.commands[i])
                time.sleep(0.01)

                try:
                    tmp = self.ser.read(9)
                    print("Serial data", tmp)
                    high = tmp[3]
                    low = tmp[4]
                    value = ((high*256) + low)
                    self.dataArray[i,self.iteration] = value
                    self.dataView[i].set(value)

                except:
                    print("Serial read failed", tmp)
                    print(self.ser.in_waiting)
                    print(self.ser.out_waiting)
                    self.ser.reset_input_buffer()
                    self.ser.reset_output_buffer()
                    self.ser.close()
                    self.ser.open()
                
            self.iteration = self.iteration + 1
            time.sleep(self.delayTime)
            if (self.iteration % self.autoSaveInterval.get()) == 0:
                #
                self.save_file
                if self.iteration == 10000:
                    self.iteration = 0
        print ("Trend stopped")

root = tk.Tk()
app = Application(master=root)
# Set the application window properties
app.master.title("K96 Data Trend Collection")
app.master.geometry("375x150")
app.master.maxsize(750,200)
app.mainloop()
