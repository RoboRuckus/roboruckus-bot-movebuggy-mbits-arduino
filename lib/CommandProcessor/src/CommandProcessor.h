/*
 * This file and associated .cpp file are licensed under the MIT Lesser General Public License Copyright (c) 2023 RoboRuckus Group
 *
 * Contributors: Sam Groveman
 */

#pragma once
#include <Arduino.h>
#include <RuckusBot.h>
#include <Configuration.h>
#include <HTTPCommunication.h>

class CommandProcessor 
{
    public:
        /// @brief Allowed types of commands.
        enum CommandTypes { Movement, Config, Damage, Setup };

        /// @brief Allowed types of configuration commands.
        enum ConfigCommands { AssignPlayer, Reset, Ready, NotReady, UpdateImage };

        /// @brief Allowed types of commands for when in setup mode.
        enum SetupCommands { Enter, SpeedTest, NavigationTest, Exit };
        
        /// @brief Allowed types of movement commands.
        enum Movements { Left, Right, Forward, Backward, LeftLateral, RightLateral };

        CommandProcessor(RuckusBot* Bot, Configuration* Config, HTTPCommunication* Communication);
        bool AddCommandToQueue(CommandTypes type, Movements move, int magnitude);
        bool AddCommandToQueue(CommandTypes type, ConfigCommands command, String payload = "");
        bool AddSetupCommandToQueue(SetupCommands command, String payload);
        bool AddDamageCommandToQueue(int magnitude);
        static void CommandProcessorTaskWrapper(void* arg);

    private:
        /// @brief A reference to a robot object.
        RuckusBot* bot;

        /// @brief A reference to a shared configuration object.
        Configuration* config;

        /// @brief A reference to a HTTPCommunication object.
        HTTPCommunication* communication;

        /// @brief Queue to hold commands to be processed.
        QueueHandle_t CommandQueue;
        /// @brief Queue to hold data payloads for config commands.
        QueueHandle_t CommandPayloadQueue;

        bool AddToQueue(int command[4]);
        void ProcessorTask();
        void ExecuteConfigCommand(ConfigCommands command, String payload);
        void ExecuteMoveCommand(Movements move, int magnitude);
        void ExecuteSetupCommand(SetupCommands command, String payload);
};
