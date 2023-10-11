#include "RuckusBot.h"
/// @brief Creates a new robot object.
/// @param Config A reference to the shared configuration object.
/// @param communication A reference to the shared HTTPCommunication object.
RuckusBot::RuckusBot(Configuration *Config, HTTPCommunication *Communication)
{
    config = Config;
    communication = Communication;
}

/// @brief Actually initialize robot with call to begin method
void RuckusBot::begin()
{
    Serial.println("Initializing robot");
    Wire.begin(22, 21);

    // Start LEDs
    FastLED.addLeds<WS2812B, 13, GRB>(leds, 25);
    FastLED.addLeds<WS2812B, FRONT_LEDS_PIN, GRB>(frontLED, NUM_FRONT_LEDS);

    FastLED.setBrightness(10);
    FastLED.clear();
    delay(50);
    FastLED.show();

    // Start IMU
    mpu6050.begin();
    // begin() is called a second time to avoid a bug where the gyro counts twice the angle expected
    delay(5);
    mpu6050.begin();
    // Initial calibration of the gyro
     calibrateGyro();    

    // Start servos
    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    // Attach servos
    left.setPeriodHertz(50); // Standard 50hz servo
    right.setPeriodHertz(50);
    left.attach(LEFT_SERVO_PIN, 500, 2500); // pin, min pulse, max pulse
    right.attach(RIGHT_SERVO_PIN, 500, 2500);

    // Attempt to load saved settings
    if (!config->loadSettings())
    {
        Serial.println("Applying default settings");
        config->BotConfig.RobotName = "Test Bot";
        
        config->TunableBotSettings["leftForwardSpeed"] = Configuration::BotSetting {
            displayname : "Left Forward Speed",
            min : 90,
            max : 180,
            increment : 1,
            value : 165
        };
        config->TunableBotSettings["rightForwardSpeed"] = Configuration::BotSetting {
            displayname : "Right Forward Speed",
            min : 0,
            max : 90,
            increment : 1,
            value : 15
        };
        config->TunableBotSettings["leftBackwardSpeed"] = Configuration::BotSetting {
            displayname : "Left Backward Speed",
            min : 0,
            max : 90,
            increment : 1,
            value : 15
        };
        config->TunableBotSettings["rightBackwardSpeed"] = Configuration::BotSetting {
            displayname : "Right Backward Speed",
            min : 90,
            max : 180,
            increment : 1,
            value : 165
        };
         config->TunableBotSettings["leftZero"] = Configuration::BotSetting {
            displayname : "Left Zero Point",
            min : 30,
            max : 150,
            increment : 1,
            value : 90
        };
        config->TunableBotSettings["rightZero"] = Configuration::BotSetting {
            displayname : "Right Zero Point",
            min : 30,
            max : 150,
            increment : 1,
            value : 90
        };
        config->TunableBotSettings["linearTime"] = Configuration::BotSetting {
            displayname : "Linear Movement Time",
            min : 500,
            max : 2000,
            increment : 10,
            value : 1200
        };
        config->TunableBotSettings["drift"] = Configuration::BotSetting {
            displayname : "Drift Limit",
            min : 0,
            max : 15,
            increment : 1,
            value : 5
        };
        config->TunableBotSettings["driftBoost"] = Configuration::BotSetting {
            displayname : "Drift Boost",
            min : 0,
            max : 20,
            increment : 1,
            value : 10
        };
        config->TunableBotSettings["turnAngle"] = Configuration::BotSetting {
            displayname : "Turn Angle",
            min : 60,
            max : 120,
            increment : 0.5,
            value : 90
        };
        config->TunableBotSettings["robotColor"] = Configuration::BotSetting {
            displayname : "Robot Color",
            min : 0,
            max : 7,
            increment : 1,
            value : 0
        };
        
        // Save default settings
        config->saveSettings();
    }
}

/// @brief Called when a player is assigned to the robot
/// @param player The player number to assign
void RuckusBot::playerAssigned(int player)
{
    showImage((images)player, (colors)config->TunableBotSettings["robotColor"].value);
}

/// @brief Display an image in a color on the screen
/// @param image The image to display
/// @param color The color to use
/// @param cache Save the image as the current image
void RuckusBot::showImage(images image, colors color, bool cache)
{
    // Save current image
    if (cache && currentImage != image)
    {
        currentImage = image;
    }
    FastLED.clear();
    delay(10);
    Display(image_maps[(int)image], color_map[(int)color]);
    showColor(color_map[(int)color]);
}

/// @brief Called when the robot needs to turn.
/// @param direction Direction of turn.
/// @param magnitude How many multiples of 90-degrees to turn.
void RuckusBot::turn(turnType direction, int magnitude)
{
    Serial.println("Turning");
    // Calculate total turn degrees
    int target = (config->TunableBotSettings["turnAngle"].value * magnitude) - 10; // The -10 seems to be a hack for the sensor to get accurate turns
    // Create a smart pointer to a new GyroHelper object. Smart pointer aids in deallocation
    std::unique_ptr<GyroHelper> helper(new GyroHelper(mpu6050));
    // Check direction of turn and activate motors appropriately.
    if (direction == RuckusBot::turnType::Right)
    {
        left.write(config->TunableBotSettings["leftForwardSpeed"].value);
        right.write(config->TunableBotSettings["rightBackwardSpeed"].value);
    }
    else if (direction == RuckusBot::turnType::Left)
    {
        left.write(config->TunableBotSettings["leftBackwardSpeed"].value);
        right.write(config->TunableBotSettings["rightForwardSpeed"].value);
    }
    else
    {
        // Bad command, exit.
        Serial.println("Bad turn command");
        return;
    }
    // Keep turning until target angle is met
    while (abs(helper->getAngle()) < target)
    {
        //Serial.println(abs(helper->getAngle()));
        delay(20);
    }
    // Stop motors
    left.write(config->TunableBotSettings["leftZero"].value);
    right.write(config->TunableBotSettings["rightZero"].value);
}

/// @brief Has a bot perform a lateral (slide) motion.
/// @param direction The direction to slide.
/// @param magnitude The number of spaces to slide.
void RuckusBot::slide(turnType direction, int magnitude)
{
    Serial.println("Sliding");
    switch (direction)
    {
    case turnType::Left:
        turn(turnType::Left, 1);
        delay(100);
        driveForward(magnitude);
        delay(100);
        turn(turnType::Right, 1);
        break;
    case turnType::Right:
        turn(turnType::Right, 1);
        delay(100);
        driveForward(magnitude);
        delay(100);
        turn(turnType::Left, 1);
        break;
    }
}

/// @brief Called when the robot needs to drive forward
/// @param magnitude How many spaces to cover
void RuckusBot::driveForward(int magnitude)
{
    Serial.println("Moving forward");
    // Calculate total time needed for the move
    int total = config->TunableBotSettings["linearTime"].value * magnitude;
    float gyroX;
    // Create a smart pointer to a new GyroHelper object. Smart pointer aids in deallocation
    std::unique_ptr<GyroHelper> helper(new GyroHelper(mpu6050));
    int leftSpeed;
    int rightSpeed;
    long start = millis();
    // Keep driving until time limit is reached
    while (millis() - start < total)
    {
        gyroX = helper->getAngle();
        /*
         * Check if the robot has drifted of course using the gyro.
         * If it has, then increase the speed of one wheel until the
         * robot is back on course.
         */
        if (gyroX > config->TunableBotSettings["drift"].value)
        {
            rightSpeed = config->TunableBotSettings["rightForwardSpeed"].value - config->TunableBotSettings["driftBoost"].value;
            leftSpeed = config->TunableBotSettings["leftForwardSpeed"].value;
        }
        else if (gyroX < -config->TunableBotSettings["drift"].value)
        {
            rightSpeed = config->TunableBotSettings["rightForwardSpeed"].value;
            leftSpeed = config->TunableBotSettings["leftForwardSpeed"].value + config->TunableBotSettings["driftBoost"].value;
        }
        else
        {
            rightSpeed = config->TunableBotSettings["rightForwardSpeed"].value;
            leftSpeed = config->TunableBotSettings["leftForwardSpeed"].value;
        }
        // Set the motors to the appropriate speed
        left.write(leftSpeed);
        right.write(rightSpeed);
        delay(50);
    }
    // Stop motors
    left.write(config->TunableBotSettings["leftZero"].value);
    right.write(config->TunableBotSettings["rightZero"].value);
}

/// @brief Called when the robot needs to drive backward
/// @param magnitude How many spaces to cover
void RuckusBot::driveBackward(int magnitude)
{
    Serial.println("Moving backward");
    // Calculate total time needed for the move
    int total = config->TunableBotSettings["linearTime"].value * magnitude;
    float gyroX;
    // Create a smart pointer to a new GyroHelper object. Smart pointer aids in deallocation
    std::unique_ptr<GyroHelper> helper(new GyroHelper(mpu6050));
    int leftSpeed;
    int rightSpeed;
    long start = millis();
    // Keep driving until time limit is reached
    while (millis() - start < total)
    {
        gyroX = helper->getAngle();
        /*
         * Check if the robot has drifted of course using the gyro.
         * If it has, then increase the speed of one wheel until the
         * robot is back on course.
         */
        if (gyroX > config->TunableBotSettings["drift"].value)
        {
            rightSpeed = config->TunableBotSettings["rightBackwardSpeed"].value;
            leftSpeed = config->TunableBotSettings["leftBackwardSpeed"].value - config->TunableBotSettings["driftBoost"].value;
        }
        else if (gyroX < (0 - config->TunableBotSettings["drift"].value))
        {
            rightSpeed = config->TunableBotSettings["rightBackwardSpeed"].value + config->TunableBotSettings["driftBoost"].value;
            leftSpeed = config->TunableBotSettings["leftBackwardSpeed"].value;
        }
        else
        {
            rightSpeed = config->TunableBotSettings["rightBackwardSpeed"].value;
            leftSpeed = config->TunableBotSettings["leftBackwardSpeed"].value;
        }
        // Set the motors to the appropriate speed
        left.write(leftSpeed);
        right.write(rightSpeed);
        delay(50);
    }
    // Stop the motors
    left.write(config->TunableBotSettings["leftZero"].value);
    right.write(config->TunableBotSettings["rightZero"].value);
}

/// @brief Called when a robot is told to move, but is blocked
void RuckusBot::blockedMove()
{
    showImage(images::Surprised, (colors)config->TunableBotSettings["robotColor"].value);
    delay(1000);
    showImage((images)config->BotConfig.PlayerNumber, (colors)config->TunableBotSettings["robotColor"].value);
}

/// @brief  Called when the robot takes damage.
/// @param amount Total damage taken so far.
void RuckusBot::takeDamage(int amount)
{
    Serial.print(amount);
    showImage(images::Surprised, (colors)config->TunableBotSettings["robotColor"].value);
    delay(1000);
    showImage((images)config->BotConfig.PlayerNumber, (colors)config->TunableBotSettings["robotColor"].value);
}

/// @brief Should be called when the robot is initialized, connected to the game server, and ready to play.
void RuckusBot::ready()
{
    showImage(RuckusBot::images::Happy, (RuckusBot::colors)config->TunableBotSettings["robotColor"].value);
    Serial.println("Ready!");
}

/// @brief Should be called if there was a problem getting the robot connected to the game server.
void RuckusBot::notReady() 
{
    showImage(RuckusBot::images::Sad, (RuckusBot::colors)config->TunableBotSettings["robotColor"].value);
}

/// @brief Runs a speed test to see if the robot drives straight
void RuckusBot::speedTest()
{
    driveForward(3);
    delay(1000);
    driveBackward(3);
}

/// @brief Runs a navigation test to see how the robot performs
void RuckusBot::navigationTest()
{
    driveForward(2);
    delay(1000);
    driveBackward(1);
    delay(1000);
    turn(turnType::Right, 1);
    delay(1000);
    turn(turnType::Left, 1);
    delay(1000);
    turn(turnType::Right, 2);
}

/// @brief Called when the game is reset
void RuckusBot::reset()
{
    showImage(images::Happy, (colors)config->TunableBotSettings["robotColor"].value);
    return;
}

void RuckusBot::setup(bool enable)
{
    if (enable)
    {
        inSetupMode = true;
        showImage(images::Duck, (colors)config->TunableBotSettings["robotColor"].value);
    }
    else
    {
        inSetupMode = false;
        showImage(images::Happy, (colors)config->TunableBotSettings["robotColor"].value);
    }
}

/// @brief Shows the last octet of the robot's IP on the display
void RuckusBot::showIP()
{
    String botIP = communication->getLocalAddress().toString();
    Serial.println("Displaying IP...");
    Serial.println("Bot IP: " + botIP);
    String last_octet = botIP.substring(botIP.lastIndexOf('.') + 1);
    Serial.println("Last octet: " + last_octet);
    showImage(images::Clear, (colors)config->TunableBotSettings["robotColor"].value, false);
    delay(1000);
    for (int i = 0; i < last_octet.length(); i++)
    {
        // Substring is used instead of [] operator since the [] operator seems to access the wrong part of memory
        showImage((images)last_octet.substring(i, i + 1).toInt(), (colors)config->TunableBotSettings["robotColor"].value, false);
        delay(1500);
        showImage(images::Clear, (colors)config->TunableBotSettings["robotColor"].value, false);
        delay(1000);
    }
}

/// @brief Calibrates the gyroscope offsets
void RuckusBot::calibrateGyro()
{
    mpu6050.calcGyroOffsets(true, 2000, 1000);
    Serial.println("");
}

/// @brief Displays an image on the LED screen. Adapted from https://www.elecrow.com/wiki/index.php?title=Mbits#Use_with_Mbits-RGB_Matrix
/// @param dat The binary encoding of LED statuses per row
/// @param myRGBcolor The color to use
void RuckusBot::Display(uint8_t dat[], CRGB myRGBcolor)
{
    for (int c = 0; c < 5; c++)
    {
        for (int r = 0; r < 5; r++)
        {
            if (bitRead(dat[c], r))
            {
                // Set the LED color at the given column and row
                leds[c * 5 + 4 - r] = myRGBcolor;
            }
        }
    }
    FastLED.show();
}

/// @brief Show color on front LEDs
/// @param myRGBcolor The color to use
void RuckusBot::showColor(CRGB myRGBcolor)
{
    for (int i = 0; i < frontLED.len; i++)
    {
        frontLED[i] = myRGBcolor;
    }
    FastLED.show();
}