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

/// @brief Callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

/// @brief Callback notifying that the access point has started
/// @param myWiFiManager the AsyncWiFiManager making the call
void configModeCallback(AsyncWiFiManager *myWiFiManager)
{
    Serial.println("Access point started");
    command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Duck) + ":1");
}

/// @brief Attempts to connect to Wi-Fi network
/// @param wifiManager The AsyncWiFiManager to connect with
void connectWiFi(AsyncWiFiManager *wifiManager)
{
    char game_server[40] = "192.168.3.1";
    char game_port[6] = "8082";
    // SPIFFS should already be mounted
    // Load settings and check for WiFi config
    if (SPIFFS.exists("/wifi_config.json"))
    {
        // File exists, reading and loading
        Serial.println("Reading WiFi config file");
        File configFile = SPIFFS.open("/wifi_config.json", "r");
        if (configFile)
        {
            Serial.println("Opened WiFi config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            char buf[size];
            // Create JSON object to hold data
            StaticJsonDocument<256> json;
            configFile.readBytes(buf, size);
            configFile.close();
            DeserializationError result = deserializeJson(json, buf);
            // Print to serial port what was loaded
            serializeJson(json, Serial);
            if (result.code() == DeserializationError::Ok)
            {
                Serial.println("\nParsed json");
                strcpy(game_server, json["game_server"]);
                strcpy(game_port, json["game_port"]);
            }
            else
            {
                Serial.println("Failed to load WiFi config");
            }
        }
    }

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    AsyncWiFiManagerParameter custom_game_server("server", "Game Server", game_server, 40);
    AsyncWiFiManagerParameter custom_game_port("port", "Game Port", game_port, 6);

    // Set config save notify callback
    wifiManager->setSaveConfigCallback(saveConfigCallback);

    // Set entered AP mode callback
    wifiManager->setAPCallback(configModeCallback);

    // Add all the parameters here
    wifiManager->addParameter(&custom_game_server);
    wifiManager->addParameter(&custom_game_port);

    // Set SSID to ESP32 chip ID
    String AP_ssid_string = "Ruckus_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    char AP_ssid[AP_ssid_string.length()];
    AP_ssid_string.toCharArray(AP_ssid, AP_ssid_string.length());

    // Fetches ssid and password and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager->autoConnect(AP_ssid, "RuckusBot"))
    {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        // Reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }

    // Read updated parameters
    strcpy(game_server, custom_game_server.getValue());
    strcpy(game_port, custom_game_port.getValue());

    /* This seems unnecessary, can just keep everything as Strings, but provides no validation of inputs. 
    // Parse game server IP address
    Serial.println("Parsing saved IP");
    byte ip[4];
    if (sscanf(game_server, "%hhu.%hhu.%hhu.%hhu", ip, ip + 1, ip + 2, ip + 3) != 4)
    {
        Serial.print("Invalid IP: ");
        Serial.println(game_server);
        ip[0] = 192;
        ip[1] = 168;
        ip[2] = 3;
        ip[3] = 1;
    }
    IPAddress IP(ip[0], ip[1], ip[2], ip[3]);
    */
    
    // Update configuration
    config.ServerConfig.ServerIP = game_server;
    config.ServerConfig.ServerPort = game_port;

    // Save the custom parameters to file
    if (shouldSaveConfig)
    {
        Serial.println("Saving WiFi config");
        StaticJsonDocument<256> json;
        json["game_server"] = game_server;
        json["game_port"] = game_port;

        File configFile = SPIFFS.open("/wifi_config.json", "w");
        if (!configFile)
        {
            Serial.println("Failed to open WiFi config file for writing");
        }
        else
        {
            serializeJson(json, configFile);
            configFile.close();
        }
    }
}

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

    // Local initialization. Once its business is done, there is no need to keep it around
    DNSServer dns;
    AsyncWiFiManager wifiManager(&server, &dns);

    // Mount file system
    mountSPIFFS(); 

    // Initialize robot
    robot.begin();    

    // Start command processor loop (8K of stack depth is probably overkill, but it does process large JSON strings and we have the RAM so better safe)
    xTaskCreate(CommandProcessor::CommandProcessorTaskWrapper, "Command Processor Loop", 8192, &command, 1, NULL);

    // Check for reset
    if (digitalRead(RESET_PIN) == LOW)
    {
        Serial.println("Resetting WiFi");
        wifiManager.resetSettings();
        command.AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Check) + ":1");
        delay(2000);
        ESP.restart();
    }

    // Load saved network settings and connect to Wi-Fi
    connectWiFi(&wifiManager);
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