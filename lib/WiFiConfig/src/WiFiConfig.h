/*
 * This file and associated .cpp file are licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 * 
 * External libraries needed:
 * ESPAsyncWiFiManager: https://github.com/alanswx/ESPAsyncWiFiManager
 * ArduinoJSON: https://arduinojson.org/
 * 
 * Contributors: Sam Groveman, Josh Buker
 */

#pragma once
#include <ESPAsyncWiFiManager.h>
#include <ArduinoJson.h>
#include <CommandProcessor.h>
#include <Configuration.h>
#include <RuckusBot.h>

class WiFiConfig {
    public:
        WiFiConfig(AsyncWiFiManager* WiFiManager, CommandProcessor* Command, Configuration* Config);
        void connectWiFi();

    private:
        bool shouldSaveConfig;
        AsyncWiFiManager* wifiManager;
        CommandProcessor* command;
        Configuration* config;
        void configModeCallback(AsyncWiFiManager *myWiFiManager);
        void configModeEndCallback(AsyncWiFiManager *myWiFiManager);
        void saveConfigCallback();
};