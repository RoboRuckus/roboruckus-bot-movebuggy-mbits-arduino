/*
 * This file and associated .cpp file are licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 *
 * External libraries needed:
 * ArduinoJson: https://github.com/bblanchon/ArduinoJson
 *
 * Contributors: Sam Groveman
 */

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <map>

class Configuration 
{
    private:
        /// @brief A description of the robot configuration
        struct botConfig 
        {
            /// @brief Contains the robot's name
            String RobotName;
            /// @brief The player number assigned to the bot
            int PlayerNumber;
            // The robot number
            int RobotNumber;
        };

        /// @brief A description of the game server configuration
        struct serverConfig
        {
            /// @brief The IP address of the game server
            String ServerIP;
            /// @brief The port used by the game server
            String ServerPort;
        };
        
    public:
        /// @brief The current robot configuration.
        botConfig BotConfig;

        /// @brief The current game server configuration.
        serverConfig ServerConfig;

        /// @brief A description of a tunable setting for a robot.
        struct BotSetting 
        {
            String displayname;
            int min;
            int max;
            float increment;
            float value; 
        };

        /// @brief A collection of tunable robot settings.
        std::map<String, BotSetting> TunableBotSettings;

        // Public methods
        bool updateSettings(String settings);
        String getSettings();
        bool loadSettings();
        bool saveSettings();
};
