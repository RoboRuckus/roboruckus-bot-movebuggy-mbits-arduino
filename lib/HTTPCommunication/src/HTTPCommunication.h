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
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Configuration.h>

class HTTPCommunication 
{
    public:  
        // Public methods
        IPAddress getLocalAddress();
        HTTPCommunication(Configuration* Config);
        bool JoinGame(String name);
        bool SignalDone(int id);

    private:
        /// @brief A reference to a confiuration object
        Configuration* config;
};