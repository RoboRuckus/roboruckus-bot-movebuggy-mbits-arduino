#include "HTTPCommunication.h"

/// @brief Set starting parameters for HTTPCommunication object.
/// @param Config A reference to a configuration object.
HTTPCommunication::HTTPCommunication(Configuration* Config)
{
    config = Config;
}

/// @brief Sends bot info to the server.
/// @param name The name of the robot.
/// @param lateralMovement If the robot supports lateral movement
/// @return True on success.
bool HTTPCommunication::JoinGame(String name)
{
    Serial.println("Sending bot info");
    JsonDocument botInfo;
    botInfo["name"] = name;
    botInfo["ip"] = getLocalAddress().toString();
    String info;
    serializeJson(botInfo, info);
    Serial.println(info);
    HTTPClient client;
    client.begin("http://" + config->ServerConfig.ServerIP + ":" + config->ServerConfig.ServerPort + "/bot");
    client.addHeader("Content-Type", "application/json");
    int resultCode = client.PUT(info);
    Serial.println("Result code: " + String(resultCode));
    bool success = false;
    if (resultCode == HTTP_CODE_ACCEPTED)
    {
        success = true;
    }
    else
    {
        Serial.println("FAIL: " + client.errorToString(resultCode) + " " + client.getString());
    }
    client.end();
    return success;
}

/// @brief Sends a signal indicating the robot has finished moving.
/// @param id The ID number of the robot.
/// @return True on success
bool HTTPCommunication::SignalDone(int id) 
{
    long timeout = millis() + 5000;
    bool success = false;
    int retry_count = 0;
    while (millis() < timeout) {
        Serial.println("Sending done moving");
        HTTPClient client;
        client.begin("http://" + config->ServerConfig.ServerIP + ":" + config->ServerConfig.ServerPort + "/bot/Done/");
        client.addHeader("Content-Type", "application/json");
        int resultCode = client.POST("{\"bot\": " + String(id) + "}");
        Serial.println("Result code: " + String(resultCode));
        if (resultCode == HTTP_CODE_ACCEPTED)
        {
            success = true;
        }
        else
        {
            Serial.println("FAIL: " + client.errorToString(resultCode) + " " + client.getString());
        }
        client.end();
        if (success || retry_count >= 3) 
        {
            break;
        }
        retry_count++;
    }
    return success;
}

/// @brief Retrieves the current IP of the sensor hub
/// @return The IP address.
IPAddress HTTPCommunication::getLocalAddress()
{
    return WiFi.localIP();
}