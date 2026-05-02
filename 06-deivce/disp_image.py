import time
import serial
from PIL import Image

def read_value(ser):
	while True:
		try:
			line = ser.readline().decode('ascii')
			value = float(line)
			return value
		except ValueError:
			continue

def main():
    ser = serial.Serial(port='/dev/ttyACM0', baudrate=62500000, timeout=0.0)
    
    if ser.is_open:
	    print(f"Port {ser.name} opened")
    else:
        print(f"Port {ser.name} closed")


    try:
        ser.write("disp_screen\n".encode('ascii'))  
        image = Image.open('get.jpg')
        width, height = image.size
        for i in range(width):
            for j in range(height):
                r, g, b = image.getpixel((i, j))
                hex_color = f"{r:02x}{g:02x}{b:02x}"
                text = "disp_px " + str(i) +" "+ str(j) +" "+ hex_color + "\n"
                ser.write(text.encode('ascii'))  


    finally:
        time.sleep(0.1)
        ser.close()
        print("Port closed")

if __name__ == "__main__":
	main()