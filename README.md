# Wheely

Wheely is a my personal robotics project built around the ESP32 using ESP-IDF and C++.  I had lots of fun building this project.


Wheely autonomously navigates its surroundings by moving forward until it detects an obstacle nearby. 
It then performs a 360° scan using an ultrasonic sonar sensor, identifies the direction with the most available space, and continues moving in that direction.
The MPU6050 is used to track the robot's rotation during scanning.

Wheely is held together using a cardboard and some tape.

https://github.com/user-attachments/assets/fc375967-e5e9-4252-b9e4-e576b63a4b25

## Hardware Components

### Main Controller
- ESP32 DevKit V1 (DOIT, 30-pin)

### Sensors and Display
- HC-SR04 Ultrasonic Distance Sensor
- MPU6050 (GY-521) IMU
- 0.96" SSD1306 OLED Display

### Motor System
- TB6612FNG Dual DC Motor Driver
- 2× 6V 12mm DC Geared Motors (200 RPM)
- 2× Wheels
- Metal Ball Caster Wheel

### Power Management
- 2× 18650 Batteries
- 2S 5A Battery Management System (BMS)
- LM2596 Mini Adjustable 3A DC-DC Buck Converter

### Miscellaneous
- 2× Mini Breadboards
- Resistors
- Jumper Wires

## Software
- C++
- ESP-IDF 6.1
- FreeRTOS
  


