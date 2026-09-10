import serial
from time import sleep
# Configure port and baud rate to match your target
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
#end commands formatted as byte strings with \r termination
delay = 0.2
while 1:    
    ser.write(b"R,100\r")
    sleep(delay)
    ser.write(b"Y,100\r")
    sleep(delay)
    ser.write(b"G,100\r")
    sleep(delay)
# Send quick sequence
ser.write(b"RYGYRYG\r")

ser.close()
