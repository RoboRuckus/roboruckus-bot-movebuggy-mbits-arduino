/*
 * This file is licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 *
 * Template for RoboRuckus game robot based on the
 * ESP32 based Mbits board via Arduino platform.
 * https://www.elecrow.com/mbits.html
 *
 * This code is intended to work with the :Move Mini Buggy Mk2
 * https://kitronik.co.uk/products/5652-move-mini-mk2-buggy-kit-excl-microbit
 *
 * External libraries needed:
 * ESPAsyncWebServer: https://github.com/esphome/ESPAsyncWebServer
 * ESPAsyncWiFiManager: https://github.com/alanswx/ESPAsyncWiFiManager
 *
 * Contributors: Sam Groveman, Josh Buker
 */

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Configuration.h>
#include <RuckusBot.h>
#include <WiFiConfig.h>
#include <HTTPCommunication.h>
#include <CommandProcessor.h>

// Global definitions

/// @brief Pin with button to reset WiFi settings (hold for ~5 seconds on boot to reset). Button A
const int RESET_PIN = 36;

/// @brief Pin with button to calibrate gyro, push anytime after connecting to game server to calibrate gyro (only after smiling). Button A
const int CALIBRATE_PIN = 36;

//// @brief Pin with button to show IP, push to scroll show last octet on screen (only after smiling). Button B
const int SHOW_IP_PIN = 39;

/// @brief Flag for indicating the Wi-Fi configuration needs to be saved
bool shouldSaveConfig = false;

/// @brief Used to track if the calibration should be for the forward or backward movement
bool forwardCalComplete = false;

/// @brief Configuration object
Configuration config;

/// @brief HTTPCommunication object
HTTPCommunication communicator(&config);

/// @brief RuckusBot object
RuckusBot robot(&config, &communicator);

/// @brief AsyncWebServer object (passed to WfiFi manager and WebServer)
AsyncWebServer server(80);

/// @brief Async command processor
CommandProcessor command(&robot, &config, &communicator);

/// @brief Local web server.
Webserver WebServer(&config, &command, &server);

/* Global functions */

/// @brief Mount or format SPIFFS file system.
void mountSPIFFS()
{
    if (SPIFFS.begin())
    {
        Serial.println("Mounted file system");
    }
    else
    {
        Serial.println("Failed to mount file system");
        // Clean FS
        Serial.println("Formatting SPIFFS, this will take a while, please wait...");
        SPIFFS.format();
        Serial.println("Done, rebooting");
        ESP.restart();
    }
}

/// @brief Run-once setup function
void setup()
{
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(SHOW_IP_PIN, INPUT_PULLUP);

    // Start the serial connection
    Serial.begin(115200);
    Serial.println("Starting");

    // Mount file system
    mountSPIFFS();

    // Initialize robot
    robot.begin();

    // Start command processor loop (8K of stack depth is probably overkill, but it does process large JSON strings and we have the RAM so better safe)
    xTaskCreate(CommandProcessor::CommandProcessorTaskWrapper, "Command Processor Loop", 8192, &command, 1, NULL);

    // Check for reset of WiFi Settings
    if (digitalRead(RESET_PIN) == LOW)
    {
        Serial.println("Resetting WiFi");
        WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
        WiFi.persistent(true);
        WiFi.disconnect(true, true);
        WiFi.persistent(false);
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Check) + ":1");
        delay(2000);
        ESP.restart();
    }

    // Configure WiFi
    DNSServer dns;
    AsyncWiFiManager manager(&server, &dns);
    WiFiConfig configurator(&manager, &command, &config);
    // Load saved network settings and connect to Wi-Fi
    configurator.connectWiFi();
    
    // Assign pins
    pinMode(SHOW_IP_PIN, INPUT_PULLUP);
    pinMode(CALIBRATE_PIN, INPUT_PULLUP);

    // Clear server settings just in case
    WebServer.ServerStop();

    // Start the update server
    WebServer.ServerStart();

    // Join the game and make the robot ready to play
    while (!communicator.JoinGame(config.BotConfig.RobotName) && !WebServer.shouldReboot)
    {
        // Failed to join game, try again after a second.
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::NotReady);
        delay(1000);
    }
    // Success!
    command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::Ready);
}

/// @brief Run-forever loop
void loop()
{
    // Check for firmware update requiring a reboot
    if (WebServer.shouldReboot)
    {
        Serial.println("Firmware updated, rebooting...");
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Check) + ":1");
        // Delay to show image and let server send response
        delay(5000);
        ESP.restart();
    }

    // Check if the calibrate gyro button was pushed
    if (digitalRead(CALIBRATE_PIN) == LOW)
    {
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Duck) + ":0");
        robot.calibrateGyro();
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)robot.currentImage) + ":1");
    }

    // Check if the show IP pin has been pushed
    if (digitalRead(SHOW_IP_PIN) == LOW)
    {
        robot.showIP();
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)robot.currentImage) + ":1");
    }
}