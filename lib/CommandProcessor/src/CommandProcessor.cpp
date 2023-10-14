#include "CommandProcessor.h"

/// @brief Processes and dispatches commands received by the robot.
/// @param bot A reference to a RuckusBot object.
CommandProcessor::CommandProcessor(RuckusBot* Bot, Configuration* Config, HTTPCommunication* Communication)
{
    bot = Bot;
    config = Config;
    communication = Communication;
    // Uses size 4 int arrays, only three are used, fourth is for future use.
    CommandQueue = xQueueCreate(5, sizeof(int[4]));
    CommandPayloadQueue = xQueueCreate(5, sizeof(String*));
}

/// @brief Adds a command to the queue.
/// @param type The command type.
/// @param move The type of move.
/// @param magnitude The magnitude of the move.
/// @param OutOfTurn If the move is out of turn.
/// @return True on success.
bool CommandProcessor::AddCommandToQueue(CommandTypes type, Movements move, int magnitude)
{
    return AddToQueue(new int[4] { type, move, magnitude, 0 });
}

/// @brief Adds a command to the queue.
/// @param type The command type.
/// @param command The type configuration command.
/// @param payload Any data payload associated with the command.
/// @return True on success.
bool CommandProcessor::AddCommandToQueue(CommandTypes type, ConfigCommands command, String payload)
{
    if (AddToQueue(new int[4] {type, command, 0, 0}))
    {
        String *payload_ptr = new String(payload);
        if (xQueueSend(CommandPayloadQueue, &payload_ptr, 10) != pdTRUE)
        {
            return true;
        }
    }
    return false;
}

/// @brief Adds a command to the queue for the robot to take damage.
/// @param magnitude The amount of damage to take.
/// @return True on success.
bool CommandProcessor::AddDamageCommandToQueue(int magnitude) 
{
    return AddToQueue(new int[4] {CommandTypes::Damage, magnitude, 0, 0});
}

/// @brief Adds a command to the queue when the robot is in setup mode. 
/// @param command The command to execute.
/// @param payload Any data associated with the command.
/// @return True on success.
bool CommandProcessor::AddSetupCommandToQueue(SetupCommands command, String payload)
{
    if (AddToQueue(new int[4] {CommandTypes::Setup, command, 0, 0}))
    {
        String *payload_ptr = new String(payload);
        if (xQueueSend(CommandPayloadQueue, &payload_ptr, 10) != pdTRUE)
        {
            return true;
        }
    }
    return false;
}

/// @brief Wraps the command processor task for static access.
/// @param arg The CommandProcessor object.
void CommandProcessor::CommandProcessorTaskWrapper(void* arg){
    static_cast<CommandProcessor*>(arg)->ProcessorTask();
}

/// @brief Runs in an infinite loop to process commands in the command queue.
void CommandProcessor::ProcessorTask()
{
    int command[4];
    String *payload = NULL;
    while(true) 
    {
        if (xQueueReceive(CommandQueue, &command, 10) == pdTRUE)
        {
            Serial.println("Processing command");
            switch (command[0])
            {
                case CommandTypes::Movement:
                    ExecuteMoveCommand((Movements)command[1], command[2]);
                    break;
                case CommandTypes::Damage:
                    bot->takeDamage(command[1]);
                    break;
                case CommandTypes::Config:
                    // Receive payload.
                    if (xQueueReceive(CommandPayloadQueue, &payload, 500 / portTICK_PERIOD_MS) == pdTRUE)
                    {
                        Serial.print("Payload: ");
                        Serial.println(*payload);
                        ExecuteConfigCommand((ConfigCommands)command[1], *payload);
                        delete payload;
                    }
                    else
                    {
                        Serial.println("Missing or bad config data payload.");
                    }
                    break;
                case CommandTypes::Setup:
                    // Receive payload.
                    if (xQueueReceive(CommandPayloadQueue, &payload, 500 / portTICK_PERIOD_MS) == pdTRUE)
                    {
                        Serial.print("Payload: ");
                        Serial.println(*payload);
                        ExecuteSetupCommand((SetupCommands)command[1], *payload);                        
                        delete payload;
                    }
                    else
                    {
                        Serial.println("Missing or bad setup data payload.");
                    }
                    break;
                
                default:
                    Serial.print("Bad command: ");
                    Serial.println(command[0]);
                    break;
            }
        }

        // Wait before checking again
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

/// @brief Executes a movement command.
/// @param move The movement command to execute.
/// @param magnitude The magnitude of the movement.
/// @param OutOfTurn If the movement is out of turn (WIP).
void CommandProcessor::ExecuteMoveCommand(Movements move, int magnitude)
{
    if (magnitude > 0)
    {
        Serial.println("Moving");
        switch (move)
        {
            case Movements::Left:
                bot->turn(RuckusBot::turnType::Left, magnitude);
                break;
            case Movements::Right:
                bot->turn(RuckusBot::turnType::Right, magnitude);
                break;
            case Movements::Forward:
                bot->driveForward(magnitude);
                break;
            case Movements::Backward:
                bot->driveBackward(magnitude);
                break;
            case Movements::LeftLateral:
                bot->slide(RuckusBot::turnType::Left, magnitude);
                break;
            case Movements::RightLateral:
                bot->slide(RuckusBot::turnType::Right, magnitude);
                break;
        }
    }
    else
    {
        // Robot trying to move, but is blocked
        bot->blockedMove();
    }
    communication->SignalDone(config->BotConfig.RobotNumber);
}

/// @brief Executes a configuration command.
/// @param command The command to execute.
/// @param payload Data payload accompanying command.
void CommandProcessor::ExecuteConfigCommand(ConfigCommands command, String payload)
{
    switch (command)
    {
        case ConfigCommands::AssignPlayer:   
        {         
            int player = 0;
            int botNumber = 0;
            if (player != 0)
            {
                bot->playerAssigned(player);
                config->BotConfig.RobotNumber = botNumber;
                config->BotConfig.PlayerNumber = player;
            }
            break;
        }
        case ConfigCommands::Reset:
            bot->reset();
            config->BotConfig.PlayerNumber = 0;
            break;
        case ConfigCommands::Ready:
            bot->ready();
            break;
        case ConfigCommands::NotReady:
            bot->notReady();
            break;
        case ConfigCommands::UpdateImage:
        {
            int image = payload.substring(0, payload.indexOf(":")).toInt();
            int shouldCache = payload.substring(payload.indexOf(":") + 1).toInt();
            bot->showImage((RuckusBot::images)image, (RuckusBot::colors)config->TunableBotSettings["robotColor"].value, shouldCache == 1 ? true : false);
            break;
        }
    }
}

/// @brief Executes a command in setup mode.
/// @param command The command to execute.
/// @param payload Data payload accompanying command.
void CommandProcessor::ExecuteSetupCommand(SetupCommands command, String payload) 
{
    switch (command)
    {
        case SetupCommands::Enter:
            bot->setup(true);
            break;
        case SetupCommands::SpeedTest:
            if(bot->inSetupMode && config->updateSettings(payload))
            {
                bot->showImage(RuckusBot::images::Duck, (RuckusBot::colors)config->TunableBotSettings["robotColor"].value);
                bot->speedTest();
            }
            break;
        case SetupCommands::NavigationTest:
            if(bot->inSetupMode && config->updateSettings(payload))
            {
                bot->showImage(RuckusBot::images::Duck, (RuckusBot::colors)config->TunableBotSettings["robotColor"].value);
                bot->navigationTest();
            }
            break;
        case SetupCommands::Exit:
            if(bot->inSetupMode && config->updateSettings(payload))
                config->saveSettings();
            // Exit setup mode
            bot->setup(false);       
            break;
        default:
            break;
    }
}

/// @brief Adds a command to the command queue.
/// @param command The command.
/// @return True on success.
bool CommandProcessor::AddToQueue(int command[4]) 
{
    Serial.println("Adding command to queue");
    if (xQueueSend(CommandQueue, (void*)command, 10) != pdTRUE)
    {
        Serial.println("Queue full");
        return false;
    }
    return true;
}
