#include "Webserver.h"

/// @brief Creates a webserver object.
/// @param config A configuration object reference.
/// @param Command A CommandProcessor object reference.
/// @param webserver An AsyncWebServer object reference.
Webserver::Webserver(Configuration* Config, CommandProcessor* Command, AsyncWebServer* webserver)
{
    server = webserver;
    config = Config;
    command = Command;
}

/// @brief Starts the update server
void Webserver::ServerStart()
{
    Serial.println("Starting update server");
    // Add requests
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(HTTP_CODE_OK, "text/html", this->indexPage_Part1 + config->BotConfig.RobotName + this->indexPage_Part2);
    });

    // Upload a file
    server->on("/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
            shouldReboot = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(HTTP_CODE_OK, "text/plain", this->shouldReboot ? "OK" : "FAIL");
            response->addHeader("Connection", "close");
            request->send(response); 
        }, onUpdate);

    // Receives a move command
    server->on("/move", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if(request->hasParam("move", true) && request->hasParam("magnitude", true))
        {
            int move = request->getParam("move", true)->value().toInt();
            int magnitude = request->getParam("magnitude", true)->value().toInt();
            this->command->AddCommandToQueue(CommandProcessor::CommandTypes::Movement, (CommandProcessor::Movements)move, magnitude);
            request->send(HTTP_CODE_ACCEPTED, "text/plain", "OK");
        }
        else 
        {
            request->send(400, "text/plain", "No or bad move data found.");
        }
    });

    // Receives a player assignment
    server->on("/assignPlayer", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        if(request->hasParam("player", true) && request->hasParam("botNumber", true))
        {
            this->command->AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::AssignPlayer,
                request->getParam("player", true)->value() + ":" + request->getParam("botNumber", true)->value());
            request->send(HTTP_CODE_ACCEPTED, "text/plain", "OK");
        }
        else 
        {
            request->send(400, "text/plain", "No or bad config data found.");
        }
    });

    // Receives damage
    server->on("/takeDamage", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if(request->hasParam("magnitude", true))
        {
            this->command->AddDamageCommandToQueue(request->getParam("magnitude", true)->value().toInt());
            request->send(HTTP_CODE_ACCEPTED, "text/plain", "OK");
        }
        else 
        {
            request->send(400, "text/plain", "No or bad config data found.");
        }
    });

    // Receives reset command
    server->on("/reset", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->command->AddCommandToQueue(CommandProcessor::CommandTypes::Config, CommandProcessor::ConfigCommands::Reset);
        request->send(HTTP_CODE_ACCEPTED, "text/plain", "OK");
    });

    // Receives an instruction in setup mode
    server->on("/setupInstruction", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if(request->hasParam("option", true) && request->hasParam("parameters", true)) 
        {
            int option = request->getParam("option", true)->value().toInt();
            this->command->AddSetupCommandToQueue((CommandProcessor::SetupCommands)option, request->getParam("parameters", true)->value());
            request->send(HTTP_CODE_ACCEPTED, "text/plain", "OK");
        }
         else 
        {
            request->send(400, "text/plain", "No or bad config data found.");
        }
    });

    // Returns the current robot settings
    server->on("/getSettings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(HTTP_CODE_OK, "text/plain", this->config->getSettings());
    });

    server->onNotFound([](AsyncWebServerRequest *request) { 
        request->send(HTTP_CODE_NOT_FOUND); 
    });
    server->begin();
}

/// @brief Stops the update server
void Webserver::ServerStop()
{
    Serial.println("Stopping web server");
    server->reset();
    server->end();
}

/// @brief Handle firmware update
/// @param request
/// @param filename
/// @param index
/// @param data
/// @param len
/// @param final
void Webserver::onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        Serial.printf("Update Start: %s\n", filename.c_str());
        // Ensure firmware will fit into flash space
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
        {
            Update.printError(Serial);
        }
    }
    if (!Update.hasError())
    {
        if (Update.write(data, len) != len)
        {
            Update.printError(Serial);
        }
    }
    if (final)
    {
        if (Update.end(true))
        {
            Serial.printf("Update Success: %uB\n", index + len);
        }
        else
        {
            Update.printError(Serial);
        }
    }
}