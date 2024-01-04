/*
 * This file and associated .cpp file are licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 *
 * Template for RoboRuckus game robot based on the
 * ESP32 based Mbits board via Arduino platform.
 * https://www.elecrow.com/mbits.html
 *
 * This code is intended to work with the :Move Mini Buggy Mk2
 * https://kitronik.co.uk/products/5652-move-mini-mk2-buggy-kit-excl-microbit
 * 
 * External libraries needed:
 * FastLED https://github.com/FastLED/FastLED
 * Temperature_LM75_Derived: https://github.com/jeremycole/Temperature_LM75_Derived <-- Not currently used
 * Tone32: https://github.com/lbernstone/Tone32 <-- Not currently used
 * MPU6050_tockn: https://github.com/Tockn/MPU6050_tockn
 * ESPAsyncWebServer: https://github.com/esphome/ESPAsyncWebServer
 * ESPAsyncWiFiManager: https://github.com/alanswx/ESPAsyncWiFiManager
 * ArduinoJson: https://github.com/bblanchon/ArduinoJson
 * ESP32Servo https://github.com/madhephaestus/ESP32Servo
 *
 * Contributors: Sam Groveman
 */


/* 
 * !!!!!
 * The methods in this file should never be called directly under normal operation.
 * All commands to the robot should go through the CommandProcessor queue.
 */

#pragma once
#include <memory>
#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <MPU6050_tockn.h>
#include <ESP32Servo.h>
#include <Configuration.h>
#include <HTTPCommunication.h>

class RuckusBot 
{
    private:
        // Robot pins
        #define FRONT_LEDS_PIN  26
        #define NUM_FRONT_LEDS  5
        #define RIGHT_SERVO_PIN 32
        #define LEFT_SERVO_PIN  25

        // Robot variables

        /// @brief A reference to the shared configuration object.
        Configuration* config;

         /// @brief A reference to the shared configuration object.
        HTTPCommunication* communication;

        /// @brief Helper class for getting angle robot has turned.
        /// Used because the MPU6050 library gyroAngle can't
        /// be reset without calling begin() method again.    
        class GyroHelper {
            public:
            /// @brief  Initialize the helper using a the specific sensor
            /// @param Gyro The senor to use
            GyroHelper(MPU6050 &Gyro) : gyro(Gyro) {
                previousTime = millis();
                gyro.update();        
            }

            /// @brief Get the angle turned since last called
            /// @return Float of the degrees turned since last call
            float getAngle() {
                gyro.update();
                // Get rotation in deg/s
                float gyroX = gyro.getGyroX();
                // Calculate time since last call in seconds
                interval = (millis() - previousTime) * 0.001;
                previousTime = millis();
                // Calculate total degrees turned so far
                totalAngle += gyroX * interval;
                // Return total angle turned
                return totalAngle;
            }

            private:
            MPU6050 &gyro;
            long previousTime;
            float interval = 0;
            float totalAngle = 0;
        };

    public:    
        /// @brief Enum of possible LED colors
        enum colors { Red, Green, Blue, Yellow, Purple, Orange, Cyan, White };

        /// @brief Enum of image maps for the screen
        enum images {Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Happy, Sad, Surprised, Duck, Check, Clear };

        /// @brief True if the robot is in setup mode
        bool inSetupMode = false;

        /// @brief Stores the last cached image that was displayed on the screen
        images currentImage = images::Clear;
        
        /// @brief Turn direction
        enum turnType { Left, Right };

        // Public methods
        RuckusBot(Configuration* Config, HTTPCommunication* Communication);
        void begin();
        void playerAssigned(int player);
        void showImage(images image, colors color, bool cache = true);
        void turn(turnType direction, int magnitude);
        void slide(turnType direction, int magnitude);
        void driveForward(int magnitude);
        void driveBackward(int magnitude);
        void blockedMove();
        void takeDamage(int amount);
        void speedTest();
        void navigationTest();
        void reset();
        void setup(bool enable);
        void showIP();
        void calibrateGyro();
        void ready();
        void notReady();

    private:
        CRGBArray<25> leds;
        CRGBArray<NUM_FRONT_LEDS> frontLED;
        // Temperature sensor not currently used
        // Generic_LM75 Tmp75Sensor;
        MPU6050 mpu6050 = MPU6050(Wire);
        // Buzzer not currently used
        // #define BUZZER_PIN     33
        // #define BUZZER_CHANNEL 0
        Servo left, right;

        /// @brief Image maps for display. Binary maps for each row, 1 on, 0 off.
        uint8_t image_maps[16][5] = {
            {B01100,B10010,B10010,B10010,B01100}, // 0
            {B00100,B01100,B00100,B00100,B01110}, // 1
            {B11100,B00010,B01100,B10000,B11110}, // 2
            {B11110,B00010,B00100,B10010,B01100}, // 3
            {B00110,B01010,B10010,B11111,B00010}, // 4
            {B11111,B10000,B11110,B00001,B11110}, // 5
            {B00010,B00100,B01110,B10001,B01110}, // 6
            {B11111,B00010,B00100,B01000,B10000}, // 7
            {B01110,B10001,B01110,B10001,B01110}, // 8
            {B01110,B10001,B01110,B00100,B01000}, // 9
            {B01010,B01010,B00000,B10001,B01110}, // Happy
            {B01010,B01010,B00000,B01110,B10001}, // Sad
            {B01010,B00000,B00100,B01010,B00100}, // Surprised
            {B01100,B11100,B01111,B01110,B00000}, // Duck
            {B00000,B00001,B00010,B10100,B01000}, // Check
            {B00000,B00000,B00000,B00000,B00000}  // Clear
        };

        /// @brief Color maps for display
        CRGB color_map[8] = {
            CRGB(255, 0, 0),    // Red
            CRGB(0, 255, 0),    // Green
            CRGB(0, 0, 255),    // Blue
            CRGB(255, 128, 0),  // Yellow
            CRGB(255, 0, 196),  // Purple
            CRGB(255, 96, 0),   // Orange
            CRGB(0, 196, 255),  // Cyan
            CRGB(144, 144, 128) // White
        };

        String getValue(String data, char separator, int index);
        void Display(uint8_t dat[], CRGB myRGBcolor);
        void showColor(CRGB myRGBcolor);      
};