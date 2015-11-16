import sys
import os
import openpyxl
import serial
import threading
import time

"""
    table_write control_switch [node id] [table index] [id] [straight pin] [turnout pin] [touch id]
    table_write track_turnout [node id] [table index] [id] [straight pin] [turnout pin]
    table_write track_turnout_led_map [node id] [table index] [id] [led num straight 1] [led num turnout 1] [led num straight 2] [led num turnout 2]
    table_write track_sensor [node id] [table index] [id] [pin]
    table_write turnout_map [node id] [table index] [control_switch id] [track_turnout id 1] [track_turnout id 2]
    table_write dcc_command [node id] [table index] [command id] [waveform pin] [power pin] [current sense pin]
"""

def serial_ports():
    ports = ['COM' + str(i + 1) for i in range(256)]

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

def GetSheetData(inSheet):
    resultList = []
    for row in inSheet.rows:
        colList = []
        for cell in row:
            colList.append(cell.value)
        resultList.append(colList)

    return resultList

class SerialPortManager(threading.Thread): 
    
    def __init__(self, inPort):
        threading.Thread.__init__(self) 
        self.port = inPort
        self.cmdResultLock = threading.Lock()
        self.cmdResult = ""
        self.cmdResultEvent = threading.Event()
        self.stop = False
        
    def run(self):
        while not self.stop:
            outputLine = self.port.readline()
            
            if outputLine == "":
                continue
            
            if outputLine.startswith("CC:"):
                with self.cmdResultLock:
                    self.cmdResult = outputLine
                    self.cmdResultEvent.set()
                    
            else:
                print outputLine

    def IssueCmdGetResponse(self, inCmd):    
        
        # clear out any old responses
        with self.cmdResultLock:
            self.cmdResultEvent.clear()
            self.outputQueue = ""
            
        self.port.write(str(inCmd + '\n'))
        
        gotResult = self.cmdResultEvent.wait(5.0)
        
        with self.cmdResultLock:
            if gotResult == False:
                return "TIMEOUT"
        
            return self.cmdResult[3:]    

    def IssueCmd(self, inCmd):    
        
        # clear out any old responses
        with self.cmdResultLock:
            self.cmdResultEvent.clear()
            self.outputQueue = ""
            
        self.port.write(inCmd + '\n')

    def Stop(self):
        self.stop = True
        self.join()

includeNodeSet = set()

if len(sys.argv) > 1:
    for nodeID in sys.argv[1].split(','):
        includeNodeSet.add(int(nodeID))

gSerialPort = serial.Serial(serial_ports()[0], 19200, timeout=3)
gSerialPortManager = SerialPortManager(gSerialPort)
gSerialPortManager.start()

gControlWB = openpyxl.load_workbook("C:\\Users\\brent\\Documents\\TrainSetup.xlsx", read_only=True)

controlSwitchData = GetSheetData(gControlWB['ControlSwitch'])
turnoutData = GetSheetData(gControlWB['Turnout'])
mapData = GetSheetData(gControlWB['Map'])
turnoutLEDMapData = GetSheetData(gControlWB['LED Map'])
configData = GetSheetData(gControlWB['Config'])

nodeIDSet = set()

csList = []
toList = []
mapList = []
ledMapList = []
configList = []

if len(includeNodeSet) > 0:
    print "WARNING: Only including nodes %s" % ', '.join(includeNodeSet)

for row in controlSwitchData[1:]:
    node = row[0]
    idNum = row[1]
    straightPin = row[2]
    turnoutPin = row[3]
    touchID = row[4]
    
    if node == "STOP":
        break
    
    if node == None:
        continue
    
    if len(includeNodeSet) > 0 and node not in includeNodeSet:
        continue
    
    if idNum == None or type(idNum) != type(0):
        idNum = 0xFFFF
        
    if straightPin == None or type(straightPin) != type(0):
        straightPin = 0xFF
        
    if turnoutPin == None or type(turnoutPin) != type(0):
        turnoutPin = 0xFF
    
    if touchID == None or type(touchID) != type(0):
        touchID = 0xFF
        
    nodeIDSet.add(node)
    
    csList.append([node, idNum, straightPin, turnoutPin, touchID])

for row in turnoutData[1:]:
    node = row[0]
    idNum = row[1]
    straightPin = row[2]
    turnoutPin = row[3]

    if node == "STOP":
        break
    
    if node == None:
        continue
    
    if len(includeNodeSet) > 0 and node not in includeNodeSet:
        continue

    if idNum == None or type(idNum) != type(0):
        idNum = 0xFFFF
        
    if straightPin == None or type(straightPin) != type(0):
        straightPin = 0xFF
        
    if turnoutPin == None or type(turnoutPin) != type(0):
        turnoutPin = 0xFF
    
    nodeIDSet.add(node)
    
    toList.append([node, idNum, straightPin, turnoutPin])

for row in mapData[1:]:
    node = row[0]
    csID = row[1]
    ttID1 = row[2]
    ttID2 = row[3]
    
    if node == "STOP":
        break
    
    if node == None:
        continue
    
    if len(includeNodeSet) > 0 and node not in includeNodeSet:
        continue
    
    if csID == None or type(csID) != type(0):
        csID = 0xFFFF

    if ttID1 == None or type(ttID1) != type(0):
        ttID1 = 0xFFFF

    if ttID2 == None or type(ttID2) != type(0):
        ttID2 = 0xFFFF
    
    nodeIDSet.add(node)

    mapList.append([node, csID, ttID1, ttID2])

for row in turnoutLEDMapData[1:]:
    node = row[0]
    turnoutID = row[1]
    straightLED1 = row[2]
    turnoutLED1 = row[3]
    straightLED2 = row[4]
    turnoutLED2 = row[5]
    
    if node == "STOP":
        break
   
    if node == None:
        continue
     
    if len(includeNodeSet) > 0 and node not in includeNodeSet:
        continue
    
    if turnoutID == None or type(turnoutID) != type(0):
        turnoutID = 0xFFFF

    if straightLED1 == None or type(straightLED1) != type(0):
        straightLED1 = 0xFFFF

    if turnoutLED1 == None or type(turnoutLED1) != type(0):
        turnoutLED1 = 0xFFFF

    if straightLED2 == None or type(straightLED2) != type(0):
        straightLED2 = 0xFFFF

    if turnoutLED2 == None or type(turnoutLED2) != type(0):
        turnoutLED2 = 0xFFFF
    
    nodeIDSet.add(node)

    ledMapList.append([node, turnoutID, straightLED1, turnoutLED1, straightLED2, turnoutLED2])

for row in configData[1:]:
    node = row[0]
    varName = row[1]
    varValue = row[2]
    
    if node == "STOP":
        break
    
    if node == None or varName == None or varValue == None:
        break;
    
    if len(includeNodeSet) > 0 and node not in includeNodeSet:
        continue

    nodeIDSet.add(node)

    configList.append([node, varName, varValue])

for nodeID in nodeIDSet:
    result = gSerialPortManager.IssueCmdGetResponse("reset_all_state %d" % nodeID)
    print result

tableIndex = 0
for entry in csList:
    result = gSerialPortManager.IssueCmdGetResponse("table_write control_switch %d %d %d %d %d %d" % (entry[0], tableIndex, entry[1], entry[2], entry[3], entry[4]))
    tableIndex += 1
    print result

tableIndex = 0
for entry in toList:
    result = gSerialPortManager.IssueCmdGetResponse("table_write track_turnout %d %d %d %d %d" % (entry[0], tableIndex, entry[1], entry[2], entry[3]))
    tableIndex += 1
    print result
    
tableIndex = 0
for entry in mapList:
    result = gSerialPortManager.IssueCmdGetResponse("table_write turnout_map %d %d %d %d %d" % (entry[0], tableIndex, entry[1], entry[2], entry[3]))
    tableIndex += 1
    print result
    
tableIndex = 0
for entry in ledMapList:
    result = gSerialPortManager.IssueCmdGetResponse("table_write track_turnout_led_map %d %d %d %d %d %d %d" % (entry[0], tableIndex, entry[1], entry[2], entry[3], entry[4], entry[5]))
    tableIndex += 1
    print result

for entry in configList:
    result = gSerialPortManager.IssueCmdGetResponse("config_var set %d %s %d" % (entry[0], entry[1], entry[2]))
    print result

for nodeID in nodeIDSet:
    result = gSerialPortManager.IssueCmdGetResponse("soft_restart %d" % nodeID)
    print result

gSerialPortManager.Stop()
gSerialPort.close()

print "FINISHED"


