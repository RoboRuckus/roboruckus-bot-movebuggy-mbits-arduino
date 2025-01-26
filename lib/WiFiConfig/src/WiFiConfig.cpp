#include "WiFiConfig.h"

/// @brief Connects to a saved WiFi network, or configures WiFi.
/// @param WiFiManager The WifManager object to use
/// @param WiFiManager Pointer to a command processor to use
/// @param Config Pointer to a configuration object to use
WiFiConfig::WiFiConfig (AsyncWiFiManager* WiFiManager, CommandProcessor* Command, Configuration* Config) {
    wifiManager = WiFiManager;
    command = Command;
    config = Config;
}

/// @brief Callback notifying us of the need to save config
void WiFiConfig::saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

/// @brief Callback notifying that the access point has started
/// @param myWiFiManager the AsyncWiFiManager making the call
void WiFiConfig::configModeCallback(AsyncWiFiManager *myWiFiManager)
{
    Serial.println("Access point started");
    command->AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::UpdateImage, String((int)RuckusBot::images::Duck) + ":1");
}


/// @brief Callback notifying that new settings were saved and connection successful
/// @param myWiFiManager the AsyncWiFiManager making the call
void WiFiConfig::configModeEndCallback(AsyncWiFiManager *myWiFiManager)
{
    Serial.println("Access point started");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

/// @brief Attempts to connect to Wi-Fi network
void WiFiConfig::connectWiFi() {    
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
            JsonDocument json;
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
    wifiManager->setSaveConfigCallback(std::bind(&WiFiConfig::configModeCallback, this, wifiManager));

    // Set entered AP mode callback
    wifiManager->setAPCallback(std::bind(&WiFiConfig::configModeCallback, this, wifiManager));

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
        // Reset and try again
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
    config->ServerConfig.ServerIP = game_server;
    config->ServerConfig.ServerPort = game_port;

    // Save the custom parameters to file
    if (shouldSaveConfig)
    {
        Serial.println("Saving WiFi config");
        JsonDocument json;
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