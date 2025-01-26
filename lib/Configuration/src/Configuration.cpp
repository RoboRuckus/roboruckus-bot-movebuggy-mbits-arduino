#include "Configuration.h"

/// @brief Called when a robot has new settings.
/// @param settings A JSON object of new parameters.
/// @return True on success.
bool Configuration::updateSettings(String settings) {
    if (settings != "") {
        // Prase settings string
        settings.trim();
        Serial.println("New settings : " + settings);
        JsonDocument new_settings;
        DeserializationError error = deserializeJson(new_settings, settings);
        if (error) 
        {
            Serial.println("Bad setting data received");
            return false;
        }
        new_settings.shrinkToFit();
        BotConfig.RobotName = new_settings["name"].as<String>();
        TunableBotSettings.clear();
        for (JsonPair kv : new_settings["controls"].as<JsonObject>())
        {
            TunableBotSettings[kv.key().c_str()] = BotSetting 
            {
                displayname: kv.value()["displayname"].as<String>(),
                min: kv.value()["min"].as<int>(),
                max: kv.value()["max"].as<int>(),
                increment: kv.value()["increment"].as<float>(),
                value: kv.value()["value"].as<float>()
            };
        }
    }
    return true;
}

/// @brief Saves the current settings to permanent storage like EEPROM or SPIFS.
/// @return True on success.
bool Configuration::saveSettings() {
    // Save values to file
    String settings = getSettings();
    Serial.println("Saving config");
    File configFile = SPIFFS.open("/robot_config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    else
    {
        configFile.print(settings);
        configFile.close();
        return true;
    }
}

/// @brief Retrieves the current robot settings.
/// @return A JSON string of all the modifiable movement parameters.
String Configuration::getSettings() {
    JsonDocument settings_doc;
    settings_doc["name"] = BotConfig.RobotName;
    for (auto const& pair : TunableBotSettings)
    {
        settings_doc["controls"][String(pair.first)]["displayname"] = pair.second.displayname;
        settings_doc["controls"][String(pair.first)]["min"] = pair.second.min;
        settings_doc["controls"][String(pair.first)]["max"] = pair.second.max;
        settings_doc["controls"][String(pair.first)]["increment"] = pair.second.increment;
        settings_doc["controls"][String(pair.first)]["value"] = pair.second.value;
    }
    settings_doc.shrinkToFit();
    String settings_string;
    serializeJson(settings_doc, settings_string);
    return settings_string;
}

/// @brief Loads the robot settings from storage like EEPROM or SPIFFS.
/// @return True on success.
bool Configuration::loadSettings() {
    if (SPIFFS.exists("/robot_config.json")) 
    {
        // File exists, reading and loading
        Serial.println("Opening robot config file");
        File configFile = SPIFFS.open("/robot_config.json", "r");
        if (configFile) 
        {
            Serial.println("Reading robot config file, loading settings...");
            String settings = "";
            // Read file
            while(configFile.available())
            {
                settings += configFile.readString();
            }
            configFile.close();
            Serial.println("Settings loaded");
            // Apply loaded settings
            return updateSettings(settings);            
        } 
        else 
        {
            Serial.println("Robot config file could not be opened");
        }
    }
    else 
    {
        Serial.println("Robot config file not found");
    }
    return false;
}