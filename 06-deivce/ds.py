import serial
import time
from PIL import Image

PORT = '/dev/ttyACM0'
BAUDRATE = 115200

DISPLAY_WIDTH = 320
DISPLAY_HEIGHT = 240

def send_command(ser, cmd):
    
    ser.write(cmd.encode('ascii'))
    
    time.sleep(0.005) 
    while ser.in_waiting:
        line = ser.readline().decode('ascii', errors='ignore').strip()
        if line:
            print(f"  <- {line}")

def main():
   
    try:
        image_path = 'get.jpg'
        image = Image.open(image_path)
        print(f"Image opened: {image_path}")
    except FileNotFoundError:
        print(f"Error: File not found at {image_path}")
        print("Please check the path or copy the image to the current directory.")
        
        image_path = '645e666c391e0939493655.jpeg'
        try:
            image = Image.open(image_path)
            print(f"Image opened: {image_path}")
        except FileNotFoundError:
            print(f"Error: File not found at {image_path} either")
            return
    
    # Получаем размеры и конвертируем в RGB
    width, height = image.size
    print(f"Original image size: {width}x{height}")
    
    # Масштабируем изображение под размер дисплея
    if width > DISPLAY_WIDTH or height > DISPLAY_HEIGHT:
        image.thumbnail((DISPLAY_WIDTH, DISPLAY_HEIGHT), Image.Resampling.LANCZOS)
        width, height = image.size
        print(f"Resized to: {width}x{height}")
    
    # Конвертируем в RGB (на всякий случай)
    if image.mode != 'RGB':
        image = image.convert('RGB')
    
    # Открываем Serial порт
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=1)
        print(f"Port {PORT} opened")
    except Exception as e:
        print(f"Failed to open port {PORT}: {e}")
        return
    
    # Небольшая задержка для стабилизации
    time.sleep(2)
    
    # Очищаем входной буфер
    ser.reset_input_buffer()
    
    # Счетчик отправленных пикселей
    total_pixels = 0
    
    try:
        # Пункт 4: отправляем пиксели по одному
        print("Sending pixels...")
        start_time = time.time()
        
        for y in range(height):
            for x in range(width):
                r, g, b = image.getpixel((x, y))
                
                color_hex = (r << 16) | (g << 8) | b
                
                cmd = f"disp_px {x} {y} {color_hex:06X}\n"
                
                ser.write(cmd.encode('ascii'))
                total_pixels += 1
                
                time.sleep(0.0005)
                
                if total_pixels % 1000 == 0:
                    elapsed = time.time() - start_time
                    print(f"  Sent {total_pixels} pixels in {elapsed:.1f}s")
        
        elapsed = time.time() - start_time
        print(f"✅ Sent {total_pixels} pixels in {elapsed:.1f}s")
        
    finally:
        print("Waiting for last pixels to be processed...")
        time.sleep(0.1)
        
        ser.close()
        print("Port closed")

if __name__ == "__main__":
    main()