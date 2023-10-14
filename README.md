# :MOVE mini MK2 Buggy Robot for RoboRuckus using the Mbits and Arduino.
## Features
1. Inexpensive
2. Easy to assemble
3. Gyroscope based navigation
4. Web-based firmware updates

## Materials
1. [:MOVE mini MK2 Buggy](https://kitronik.co.uk/products/5652-move-mini-mk2-buggy-kit-excl-microbit) (Also available from [SparkFun Electronics](https://www.sparkfun.com/products/16787) and [RobotShop](https://www.robotshop.com/products/kitronik-move-mini-mk2-buggy-kit-w-o-microbit))
2. [Mbits](https://www.elecrow.com/mbits.html) (Currently broken on their website, but can be purchased from their [official AliExpress](https://www.aliexpress.com/item/1005003540049324.html), [RobotShop](https://www.robotshop.com/products/elecrow-mbits-esp32-dev-board-based-on-letscode-scratch-30-arduino), or an [AliExpress reseller](https://www.aliexpress.com/item/1005005524784099.html))

## Assembly
Assemble the robot according to the instructions provided in the robot kit.

## Programming
1. To program the robot, first download [Visual Studio Code](https://code.visualstudio.com/).
2. Next, follow the instructions to [install PlatformIO](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode).
3. Clone or download this repository to a folder.
4. Open the project folder with PlatformIO in Visual Studio Code.
5. Connect the Mbits to the computer via USB.
6. Upload the code using the [PlatformIO toolbar](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode-toolbar).

## Operation
Using the robot is for the part very simple, just turn it on! However, there is some first-time setup you'll need to do, as well as some advanced tuning options, which are all detailed below.

### The A and B buttons
The A and B buttons on the Mbits have several functions depending on when and how they are pressed:

#### Hold A During Boot
This will reset the currently saved Wi-Fi and game server settings. When this is accomplished, the robot will display a check-mark and reboot, you should release the button when you see the check-mark. See [Connecting to the Game](#connecting-to-the-game).

#### Press A
Pressing the A button any time after the robot has successfully connected to the game server will have it recalibrate the onboard gyroscope. This is automatically done when the robot is powered on, but if the robot is drifting or not turning properly, this can be repeated to help. When the A button is pressed, the robot will display a duck symbol, the robot will display the previous image when the calibration is finished. **The robot should be kept perfectly still during the calibration process.** This can be done anytime, even while playing the game, but shouldn't be done during the movement phase of the game.

#### Press B
Pressing the B button any time after the robot has successfully connected to the game server will have it display the last octet of its IP address on the screen, one number at a time. This can be useful for troubleshooting or for connecting to the robot to [update the firmware](#updating-the-firmware).

### Connecting to the Game
If this is the first time powering on the robot it may take a while as it needs to format and mount the SPIFFS. Once it's finished the initial boot, you'll need to configure it to connect to the Wi-Fi network used by the game server as well as the game server's IP address. When you power on the robot for the first time (or if the expected Wi-Fi network is not available) you'll need to wait a minute or so until the screen displays a duck symbol. This is the symbol used to indicate that the robot is in a setup mode.

Here, there robot has created its own Wi-Fi network named called "Ruckus_XXXXXXX" the X's will be unique to each robot. Connect to that network, ideally with your phone, and visit the IP address `192.168.4.1` to reach the robot's setup interface. Choose "Configure Wi-Fi" to setup the robot, and pick the Wi-Fi address you want to robot to connect to. Enter the Wi-Fi password, IP address of the game server, and the port used by the game server, as shown below.

![photo of assembled robot](/media/RobotWifiGateway.png)

If the Wi-Fi credentials are good, the robot will reboot and attempt to connect to the game server, and you're done! To reset the Wi-Fi or game server settings, see [Hold A During Boot](#hold-a-during-boot).

### Playing With the Robot
Turn the robot on, keep it perfectly still during the boot process to properly calibrate the gyroscope. If the game server and Wi-Fi network are working, the robot will automatically connect and display a happy face. If the robot can't connect to the game server, it will show a sad face and keep trying. Once all the robots you need are connected, you can refer to [this documentation](https://www.roboruckus.com/documentation/running-a-game/) for how to tune their movement and setup the game.

### Robot Tuning Parameters
The following tuning parameters are available for this robot (see [tuning a robot](https://www.roboruckus.com/documentation/running-a-game/#Tuning_the_Robots)):
* Drift Limit: This is the number of degrees the gyroscope will allow a robot to drift off a linear course before correcting.
* Drift Boost: This is how aggressively the robot will move to correct itself when it drifts off course.
* Left Backward Speed: This is the speed of the left wheel when moving backwards. The smaller this number, the faster the movement.
* Left Forward Speed: This is the speed of the left wheel when moving forwards. The larger this number, the faster the movement.
* Left Backward Speed: This is the speed of the right wheel when moving backwards. The larger this number, the faster the movement.
* Left Forward Speed: This is the speed of the left wheel when moving forwards. The smaller this number, the faster the movement.
* Left Zero Point: Sets the zero point of the left wheel. The zero point is ideally "90", but if the left wheel isn't completely still when not moving, adjust this until the movement stops.
* Right Zero Point: Sets the zero point of the right wheel. The zero point is ideally "90", but if the right wheel isn't completely still when not moving, adjust this until the movement stops.
* Linear Move Time: This is the time, in milliseconds, that the robot needs to move a distance of one board square.
* Turn Angle: This is the actual number of degrees the gyroscope needs to measure to have the robot complete a 90-degree turn.
* Robot Color: The color displayed on the robot's LEDs.
* Robot Name: The robot's name.

### Updating the Firmware
You can update the robot's firmware any time after it has connected to the Wi-Fi network (usually after it displays a happy or sad face). Simply connect to the same Wi-Fi network as the robot and enter the robot's IP address in your browser. Once connected, select the appropriate `firmware.bin` file and start the update. Be patient as the robot updates and reboots. All the robot's settings should be preserved.