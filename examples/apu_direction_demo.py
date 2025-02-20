# MaixPy APU Direction Detection Demo
# This demo shows how to use the APU (Audio Processing Unit) to detect sound direction
# using a microphone array.

from Maix import GPIO, APU
from board import board_info
import time

'''
Hardware Setup:
- Connect I2S microphone array to the following pins:
  * I2S_D0 = 23 (Data channel 0)
  * I2S_D1 = 22 (Data channel 1)
  * I2S_D2 = 21 (Data channel 2)
  * I2S_D3 = 20 (Data channel 3)
  * I2S_WS = 19 (Word Select/Left-Right Clock)
  * I2S_SCLK = 18 (Serial Clock)
'''

def init_apu():
    # 1. Initialize system clock for APU (45.1584MHz for 44.1kHz audio)
    APU.init_clock(45158400)
    print("Clock initialized")
    
    # 2. Initialize FPIOA pins for I2S
    APU.init_fpioa(23, 22, 21, 20, 19, 18)
    print("FPIOA initialized")
    
    # 3. Initialize PLIC with priority 4 (range 1-7)
    APU.init_plic(4)
    print("PLIC initialized")
    
    # 4. Initialize I2S with 44.1kHz sample rate
    APU.init_i2s(44100)

    # 5. Initialize APU
    # gain = 96 (adjust based on your needs, range 0-255)
    # channels = 0xFF (all channels enabled)
    APU.init_apu(96, 0xFF)


    #APU.dir_set_interrupt_mask(0)

    print("APU initialized successfully")

def main():
    print("APU Direction Detection Demo")
    print("Initializing...")
    
    # Initialize APU with all components
    init_apu()
    
    # Configure direction detection
    # Parameters:
    # - radius: 0.08 meters (distance between microphones)
    # - mic_num: 4 (number of microphones)
    # - center: (0.0, 0.0) center coordinates
    APU.configure_direction(4.00, 6, True)
    
    print("Starting direction detection...")
    print("Speak or make sound near the microphone array")
    
    try:
        while True:
            # Start direction detection
            APU.start_direction_detection()
            
            # Wait for direction data
            while not APU.dir_is_ready():
                time.sleep_ms(1)
            
            # Get the detected direction (0-15, representing 0-360 degrees in 22.5° steps)
            direction = APU.get_direction()
            
            # Convert direction to degrees (each step is 22.5 degrees)
            degrees = direction * 22.5
            print("Detected sound from: {:.1f}° (sector {})".format(degrees, direction))
            
            # Clear ready flag for next detection
            APU.dir_clear_ready()
            
            # Small delay to prevent too rapid updates
            time.sleep_ms(100)
            
    except KeyboardInterrupt:
        print("\nDemo stopped by user")
        return

if __name__ == "__main__":
    main()
