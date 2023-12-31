// IDP M204 robot control program

// TODO:
// - When looking for the second grid block, don't search the same nodes twice

/*
Node numbering for block retrieval (start heading towards node 2):

    10---------14
    |           |
    |           |
    5--6--7--8--9
    |  |  |  |  |
    0--1--2--3--4
    |     |     |
   ---   ---   ---
   | |   | |   | |
   ---   ---   ---

      N      0
    W-|-E  3-|-1
      S      2
 */

#include <Adafruit_MotorShield.h>
#include "Wire.h"
#include "VL53L0X.h"
#include <cppQueue.h>

// Setup motors
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *left_motor= AFMS.getMotor(3); //left motor
Adafruit_DCMotor *right_motor = AFMS.getMotor(4); // right motor
int left_speed;
int right_speed;
// const float MOTOR_SPEED_FACTOR[2] = { 1, 0.88 };
const float MOTOR_SPEED_FACTOR[2] = { 1, 1 };

// Time of Flight (ToF) sensor
VL53L0X tof_sensor;
uint16_t tof_distance = 0;

// Ultrasonic sensor
#define MAX_RANG  (520)//the max measurement value of the module is 520cm(a little bit longer than effective max range)
#define ADC_SOLUTION  (1023.0)//ADC accuracy of Arduino UNO is 10bit
int ultrasound_pin = A3;
float ultrasound_distance, ultrasound_sensity;

// Line sensors
const int LINE_SENSOR_PINS[4] = { 6, 8, 9, 7 }; // [BL, FL, FR, BR]
int line_sensor_readings[4]; // [BL, FL, FR, BR]

// LEDs
const int led_R = 5; // red LED
const int led_G = 10; // green LED
const int led_B = 4; // blue LED

// Magnet sensor
const int magnetic_sensor_pin = 3; // choose the input pin
int val_magnetic_sensor; // variable for reading the pin status

// Push buttons
const int green_button_pin = 2; // push button pin
int val_green_button;

// Robot attributes
enum Direction { north, east, south, west };
Direction current_direction;
int current_node;
cppQueue path(sizeof(int));
enum BlockStatus { no_block=-1, non_magnetic=0, magnetic=1 };
BlockStatus current_block_status;
int number_of_blocks_retrieved;
int node_of_first_block; // Used to (non-optimally) shorten search route for second block

enum Turn { left90=-1, straight=0, right90=1, turn180=2 };

// Board constants
const int ANTICLOCKWISE_PATH[] = { 2, 3, 4, 9, 8, 7, 6, 5, 0, 1 };

/* The i^th element of FIRST_RETREIVAL_RETURN_PATH is the next node to travel to from node i
on the return journey having just collected a block. This prevents crossing a
node that hasn't already been crossed, which may contain the second block. */
const int FIRST_RETRIEVAL_RETURN_PATH[] = { 5, 2, -1, 2, 3, 6, 7, 2, 3, 4 };

/* Path from start to top right to begin searching free space */
const int FREE_SPACE_SETUP_PATH[5] = { 2, 3, 4, 9, 14 };

/* Path from top left back to start */
const int FREE_SPACE_RETURN_PATH[5] = { 10, 5, 0, 1, 2 };

/**
 * @brief Panic mode. Stops the robot if something has gone critically wrong.
 * 
 */
void panic() {
    stop();
    digitalWrite(led_R, HIGH);
    digitalWrite(led_G, HIGH);
    digitalWrite(led_B, HIGH);
    while (true) {
        delay(1000);
    }
}

int mod(int x, int y) {
    return ((x%y)+y)%y;
}

void waitForButtonPress() {
    Serial.print("Waiting for button press... ");
    do {
        val_green_button = digitalRead(green_button_pin);
    } while (val_green_button == LOW);
    Serial.println("Done");
    delay(500);
}

void updateLineSensorReadings() {
    for (int i = 0; i < 4; i++) 
    {
        line_sensor_readings[i] = digitalRead(LINE_SENSOR_PINS[i]);
    }
}

void updateTOFReading() {
    tof_distance = tof_sensor.readRangeSingleMillimeters();
}

void updateUltrasoundReading() {
    ultrasound_sensity = analogRead(ultrasound_pin);
    ultrasound_distance = ultrasound_sensity * MAX_RANG / ADC_SOLUTION;
}

void updateMagneticSensorReading() {
    val_magnetic_sensor = digitalRead(magnetic_sensor_pin);
}

void printLineSensorReadings() {
    for (int reading : line_sensor_readings) {
        Serial.print(reading);
    }
    Serial.println();
}

/**
 * @brief Set the speeds of the two motors. Because of the orientation of the motors,
 * they must be set to run backwards when we wish to go forwards and vice versa.
 * 
 * @param new_left_speed Desired speed of the left motor between -255 and 255
 * @param new_right_speed Desired speed of the right motor between -255 and 255
 */
void setMotors(int new_left_speed, int new_right_speed) {
    left_speed = new_left_speed * MOTOR_SPEED_FACTOR[0];
    right_speed = new_right_speed * MOTOR_SPEED_FACTOR[1];
    left_motor -> setSpeed(abs(left_speed));
    right_motor -> setSpeed(abs(right_speed));
    left_motor -> run((left_speed == 0) ? RELEASE : (left_speed > 0) ? BACKWARD : FORWARD);
    right_motor -> run((right_speed == 0) ? RELEASE : (right_speed > 0) ? BACKWARD : FORWARD);
}

void goForwards(){
    setMotors(175, 160);
}

void goForwardsFast(){
    setMotors(200, 181);
}

void goBackwards(){
    setMotors(-146, -157);
}
void spinRight(){
    setMotors(155, -150);
}

void spinLeft(){
    setMotors(-155, 150);
}

void turnRight(){
    setMotors(140, -75);
}

void turnLeft(){
    setMotors(-75, 140);
}

void stop(){
    setMotors(0, 0);
}

/**
 * @brief Semi-smart 180 turning. Turns blind for a time and then continues turning until the
 * front sensors are on the line.
 * 
 */
void rotate180() {
    setMotors(150, -150);
    delay(2750);
    turnUntilNextLine();
    delay(100);
    stop();
}

/**
 * @brief Turn 180 degrees solely using a calibrated time delay. Should only be used when the
 * robot is not line-following.
 * 
 */
void rotate180Dumb() {
    setMotors(150, -150);
    delay(3200);
    stop();
}

/**
 * @brief Turns 90 degrees right using a time delay. To turn using the line sensors instead, use 
 * `spinRight()` followed by `turnUntilNextLine()`.
 * 
 */
void rotate90R() {
    setMotors(150, -150);
    delay(1850);
    stop();
}

/**
 * @brief Turns 90 degrees left using a time delay. To turn using the line sensors instead, use 
 * `spinLeft()` followed by `turnUntilNextLine()`.
 * 
 */
void rotate90L() {
    setMotors(-150, 150);
    delay(1450);
    stop();
}

/**
 * @brief Use the back line sensors to detect if we are currently at a junction.
 * 
 * @return Whether the robot is at a junction or not
 */
bool detectJunction(){
    return ((line_sensor_readings[0]==1) || (line_sensor_readings[3]==1));
}

/**
 * @brief Get the compass direction between two adjacent nodes
 * 
 * @param start_node int
 * @param end_node int
 * @return Direction enum
 */
Direction getDesiredDirection(int start_node, int end_node) {
    if ((start_node/5) == (end_node/5)) {
        // nodes are on the same line
        int difference = end_node - start_node;
        if (abs(difference) != 1){
            Serial.println("Non-adjacent nodes (1)");
            panic();
        } else {
            return difference > 0 ? east : west;
        }
    } else if (mod(start_node, 5) == mod(end_node, 5)) {
        int difference = (end_node / 5) - (start_node / 5);
        return difference > 0 ? north : south;
    } else {
        Serial.println("Non-adjacent nodes (2)");
        panic();
    }
}

/**
 * @brief Get the turn required to go from one compass direction to another
 * 
 * @param start_direction Direction enum
 * @param end_direction Direction enum
 * @return Turn enum
 */
Turn getDesiredTurn(Direction start_direction, Direction end_direction) {
    /* Get the turn required to get from one compass direction to another */
    int difference = mod(end_direction - start_direction + 1,  4) - 1;
    // -1, 0, 1, 2 --> 90L, none, 90R, 180
    return Turn(difference);
}

/**
 * @brief Use the front two sensors to follow a line, correcting itself if it deviates
 * from the line. For use in loops.
 * 
 */
void lineFollow() {
    if ((line_sensor_readings[1] == 1) && (line_sensor_readings[2] == 0)) {
        // Deviating right
        turnLeft();
    } else if ((line_sensor_readings[1] == 0) && (line_sensor_readings[2] == 1)) {
        // Deviating left
        turnRight();
    } else {
        goForwardsFast();
    }
}

/**
 * @brief Variation of `lineFollow` where extra precision is required.
 * 
 */
void lineFollowSlow() {
    // TODO: Merge with `lineFollow` into a single function.

    if ((line_sensor_readings[1] == 1) && (line_sensor_readings[2] == 0)) {
        // Deviating right
        turnLeft();
    } else if ((line_sensor_readings[1] == 0) && (line_sensor_readings[2] == 1)) {
        // Deviating left
        turnRight();
    } else {
        goForwards();
    }
}

/**
 * @brief Wrapper around `lineFollowSlow` that runs in a loop for a given amount of time.
 * 
 * @param time_ms Time to line follow for in milliseconds
 */
void lineFollowForTime(int time_ms) {
    unsigned long start_time = millis();
    unsigned long end_time = start_time;
    while ((end_time - start_time) < time_ms) {
        updateLineSensorReadings();
        lineFollowSlow();
        end_time = millis();
    }
}

/**
 * @brief Having already started a turn, continue turning until you hit the perpendicular white line
 * 
 */
void turnUntilNextLine() {
    Serial.print("Beginning turn to next line. LSRs: ");
    printLineSensorReadings();

    while (true) {
        // Continue turning until the front sensors are back on a white line
        updateLineSensorReadings();
        if ((line_sensor_readings[1] == line_sensor_readings[2]) && (line_sensor_readings[2] == 1)) {
            break;
        }
    }

    Serial.print("Next line hit. LSRs: ");
    printLineSensorReadings();
    delay(100);
}

/**
 * @brief Make a given turn
 * 
 * @param turn enum
 */
void makeTurn(Turn turn) {
    stop();

    switch (turn) {
        case left90:
            spinLeft();
            break;
        case straight:
            return;
        case right90:
            spinRight();
            break;
        case turn180:
            rotate180();
            return;
    }
    delay(750);
    turnUntilNextLine();
}

/**
 * @brief Main function for handling a junction while path-following. Makes any required turns
 * according to the current `path`.
 * 
 */
void handleJunction() {
    stop();
    delay(300);
    Serial.println("Junction detected");
    Serial.print("LSRs at junction: ");
    printLineSensorReadings();
    
    path.pop(&current_node);
    Direction desired_direction;

    if (path.isEmpty()) {
        Serial.print("Path empty. Handling behaviour for node ");
        Serial.println(current_node);
        // TODO: Find neater implementation for block retrieval 
        // than catching the -1 and 2 nodes here.
        if (current_node == 2) {
            // Currently carrying a block and is at node 2, ready to begin block depositing.
            desired_direction = south;
            int n = -1;
            path.push(&n);
        } else if (current_node == -1) {
            // Mid block depositing.
            return;
        } else if (current_node == 14) {
            desired_direction = west;
        } else if (current_node == 1) {
            /* Completed anticlockwise grid search path without finding block
            (Block probably at node 1 and detected past junction) 
            Set anticlockwise path again */
            for (int n : ANTICLOCKWISE_PATH) {
                path.push(&n);
            }
            desired_direction = east;
        } else {
            Serial.println("No path set and not retrieving at node 2");
            panic();
        }
    } else {
        int next_node;
        path.peek(&next_node);

        Serial.print("Next segment: ");
        Serial.print(current_node);
        Serial.print("-->");
        Serial.print(next_node);
        Serial.println();

        desired_direction = getDesiredDirection(current_node, next_node);
    }

    turnToDesiredDirection(desired_direction);
    lineFollowForTime(500);
}

void turnToDesiredDirection(Direction desired_direction) {
    Turn desired_turn = getDesiredTurn(current_direction, desired_direction);

    Serial.print("Direction change: ");
    Serial.print(current_direction);
    Serial.print("-->");
    Serial.println(desired_direction);
    Serial.print("Turn required: ");
    Serial.println(desired_turn);

    makeTurn(desired_turn);
    current_direction = desired_direction;
}

/**
 * @brief Detect block type and set LEDs and variables accordingly. For use with both grid
 * blocks and free-space blocks.
 * 
 */
void handleBlockFound() {
    goForwards();
    delay(300);
    stop();
    delay(500);
    val_magnetic_sensor = digitalRead(magnetic_sensor_pin);
    if (val_magnetic_sensor == HIGH) {
        // Detected magnetic block
        current_block_status = magnetic;
        digitalWrite(led_R, HIGH);
        delay(5500);
        digitalWrite(led_R, LOW);
        delay(1000);
    } else {
        // Block is non-magnetic
        current_block_status = non_magnetic;
        digitalWrite(led_G, HIGH);
        delay(5500);
        digitalWrite(led_G, LOW);
        delay(1000);
    }
}

/**
 * @brief Function called when we detect a grid block. Calls `handleBlockFound` and `setReturnPath`.
 * 
 */
void handleGridBlockFound() {
    if (number_of_blocks_retrieved == 0) {
        node_of_first_block = current_node;
    }
    handleBlockFound();
    if (current_node == 1) { // This is some not nice hardcoding of an edge case
        path.clean();
        int next_node = 2;
        path.push(&next_node);
    } else {
        setReturnPath();
        rotate180();
        current_direction = mod(current_direction + 2, 4);
        goForwards();
        delay(500); /* Increasing this delay should help the post-block confusion
        but will cause issues if the 180 turn isn't perfect */
    }
}

/**
 * @brief After finding a grid block, set the path for the return home. Needs to avoid 
 * going over unvisited nodes in case there is a block there.
 * 
 */
void setReturnPath() {
    // TODO: Be smart and choose the shortest path possible when retrieving the 
    // second block. Issue currently is handling the node/junction underneath the block.

    path.clean();
    int next_node = current_node;
    path.push(&next_node);
    if (next_node == -1) {
        // Block is at node 2 (picked up before junction) (bodge)
        return;
    }
    while (next_node != 2) {
        next_node = FIRST_RETRIEVAL_RETURN_PATH[next_node];
        path.push(&next_node);
    }
    if (path.isEmpty()) {
        panic();
    }
}

/**
 * @brief Function for depositing a block. Starting from the top of the start box facing 
 * south, finishing past the top of the start block facing north (on the way to 2).
 * 
 */
void depositBlock() {
    if (current_block_status == magnetic) {
        turnToDesiredDirection(west);
    } else {
        turnToDesiredDirection(east);
    }
    lineFollowForTime(750);
    while(!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollowSlow();
    }
    turnToDesiredDirection(south);
    lineFollowForTime(1350);
    if (current_block_status == magnetic) {
        rotate90R();
        current_direction = west;
    } else {
        rotate90L();
        current_direction = east;
    }
    goForwards();
    delay(5850);
    stop();
    delay(1000);
    goBackwards();
    delay(2000);
    do {
        updateLineSensorReadings();
    } while(!detectJunction());
    turnToDesiredDirection(north);

    while(!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollowSlow();
    }
    if (current_block_status == magnetic) {
        turnToDesiredDirection(east);
    } else {
        turnToDesiredDirection(west);
    }
    lineFollowForTime(500);
    while(!detectJunction()) {
      updateLineSensorReadings();
      updateFlashingLED();
      lineFollowSlow();
    }
    turnToDesiredDirection(north);
    stop();

    number_of_blocks_retrieved++;
    current_block_status = no_block;
    current_direction = north;
    current_node = -1;

    if (number_of_blocks_retrieved == 1) {
        lineFollowForTime(1000);
        goBackwards();
        delay(2300);
        stop();
        digitalWrite(led_B, HIGH);
        delay(5500);
        digitalWrite(led_B, LOW);
        goForwards();
        delay(1500);
        
        if ((node_of_first_block == 0) || ((node_of_first_block > 4) && (node_of_first_block < 9))) {
            int new_path[] = { 2, 7, 6, 5, 0, 1 };
            for (int n : new_path) {
                path.push(&n);
            }
        } else {
            for (int n : ANTICLOCKWISE_PATH) {
                path.push(&n);
            }
        }
    } else {
        for (int n : FREE_SPACE_SETUP_PATH) {
            path.push(&n);
        }
    }
    lineFollowForTime(1000);
}

/**
 * @brief Algorithm for searching for a block in the free space. Starts at junction 14 and 
 * begins a loop around the free space. While on a long edge, scans with the side-mounted ultrasound
 * for a block. Attempts to get the block. If it misses, it return to node 14 and begins the loop 
 * again. If it gets the block, it begins the retrieval process.
 * 
 */
void freeSearch() {
    bool block_detected = false;
    // Drive around free space until a block is detected
    while (!block_detected) {
        block_detected = driveAroundFreeSpaceLookingForBlock();
    }
    getFreeSpaceBlock();

    current_direction = north;
    
    if (current_block_status == no_block) {
        turnToDesiredDirection(east);
        current_node = 13;
        int new_path[] = { 14 };
        for (int n : new_path) {
            path.push(&n);
        }
    } else {
        turnToDesiredDirection(west);
        current_node = 11;
        for (int n : FREE_SPACE_RETURN_PATH) {
            path.push(&n);
        }
    }

}

/**
 * @brief Drives a single loop around the free space. Returns if it detects a block while on a 
 * long edge.
 * 
 * @return bool: True if block detected while on long edge of free-space. 
 * False if no block detected during loop - finished loop at node 14 facing North.
 */
bool driveAroundFreeSpaceLookingForBlock() {
    // TODO: Lots of hard-coding here that could be improved
    bool block_detected = false;
    if (current_node != 14) {
        panic();
    }
    if (current_direction != west) {
        turnToDesiredDirection(west);
    }
    lineFollowForTime(1000);

    // Drive 14-->10 checking for block
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        updateUltrasoundReading();

        if (ultrasound_distance < 45) {
            block_detected = true;
            break;
        }

        lineFollowForTime(200);
    }
    if (block_detected) {
        return true;
    }
    current_node = 10;
    turnToDesiredDirection(south);
    lineFollowForTime(2000);

    // Drive 10-->5
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollow();
    }
    current_node = 5;
    turnToDesiredDirection(east);
    lineFollowForTime(2000);

    int junction_count = 0; // Rather than path following, hard-code to go past three junctions
    // to get to 9 from 5

    // Drive 5-->9 checking for block
    while (junction_count < 4) {
        updateLineSensorReadings();
        updateFlashingLED();
        updateUltrasoundReading();

        if (detectJunction()) {
            current_node++;
            junction_count++;
            lineFollowForTime(500);
        }

        if (ultrasound_distance < 45) {
            block_detected = true;
            break;
        }

        lineFollowSlow();
    }

    if (block_detected) {
        return true;
    }

    turnToDesiredDirection(north);
    lineFollowForTime(2000);

    // Drive 9-->14
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollow();
    }
    current_node = 14;
    return false;
}

/**
 * @brief Get the amount of time to drive forward for in order to be properly line up with a 
 * free-space block. A function of the distance to the block. (Negative to go backwards).
 * 
 * @return int Time in ms to drive forwards for (negative for backwards) before turning to retrieve
 * a free-space block.
 */
int getFreeSpaceTurnDelay() {
    float actual_distance = ultrasound_distance + 10;
    float required_delay = (actual_distance - 45) * 40;
    int required_delay_rounded = (int) required_delay;
    return required_delay_rounded;

}

/**
 * @brief Having detected a block in the free space, turn and attempt to get it.
 * 
 */
void getFreeSpaceBlock() {
    stop();
    delay(500);

    // Because of the detection cone of the ultrasound, we may need to drive a bit forwards
    // backwards (depending on distance to block) before turning to make sure we are lined
    // up properly.
    int forwards_time = getFreeSpaceTurnDelay();
    Serial.print("Ultrasound distance: ");
    Serial.println(ultrasound_distance);
    Serial.print("Turn delay: ");
    Serial.println(forwards_time);
    if (forwards_time < 0) {
        goBackwards();
    } else if (forwards_time > 0) {
        goForwards();
    }
    delay(abs(forwards_time));
    stop();
    rotate90L();
    goForwards();
    delay(500);
    bool block_found = false;
    while (true) {
        updateTOFReading();
        updateLineSensorReadings();

        if (tof_distance < 30) {
            block_found = true;
            break;
        }

        if (detectJunction()) {
            break;
        }
    }

    if (block_found) {
        handleBlockFound();
        if (current_node == 14) {
            rotate90L();
            delay(100);
            rotate90L();
            delay(100);
        }
        goForwards();
        while (!detectJunction()) {
            updateLineSensorReadings();
            updateFlashingLED();
        } // TODO: Similar code to below.
    } else {
        // We missed the block. Go back to the top line so we can path follow to 14 and start
        // the loop again.
        if (current_node == 14) {
            // We started from the top line. We're now at the bottom of the free space and 
            // need to turn around and go back to the top.
            // rotate180Dumb();
            rotate90L();
            delay(100);
            rotate90L();
            delay(100);
            goForwards();
            delay(1000);
            do {
                updateLineSensorReadings();
                updateFlashingLED();
            } while (!detectJunction());
        }

    }
}

/**
 * @brief Make the blue LED flash. Should be included in any loop in which the robot is moving.
 * 
 */
void updateFlashingLED() {
    digitalWrite(led_B, mod((millis() / 500), 2));
}

void setup() {

    Serial.begin(9600);           // set up Serial library at 9600 bps
    Serial.println("IDP team 204 - grid block collection");

    if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
        Serial.println("Could not find Motor Shield. Check wiring.");
        while (1);
    }
    Serial.println("Motor Shield found.");

    Wire.begin();
    tof_sensor.setTimeout(500);
    if (!tof_sensor.init())
    {
      Serial.println("Failed to detect and initialize sensor!");
      while (1) {}
    }

    // Defining pin modes
    for (int pin : LINE_SENSOR_PINS) {
        pinMode(pin, INPUT);
    }

    pinMode(led_B, OUTPUT);
    pinMode(led_G, OUTPUT);
    pinMode(led_R, OUTPUT);

    pinMode(magnetic_sensor_pin, INPUT);

    pinMode(green_button_pin, INPUT);
    waitForButtonPress();

    // Initial state of robot. Can be altered to easily test the robot in different scenarios.
    number_of_blocks_retrieved = 0;
    current_node = -1;
    current_direction = north;
    // int new_path[] = { 14 };
    for (int n : ANTICLOCKWISE_PATH) {
        path.push(&n);
    }
    current_block_status = no_block;

    goForwards();
    delay(2000);
}

/**
 * @brief Main loop for robot activity. Handles path -following and block detection.
 * 
 */
void loop(){
    // Pause if button pressed
    val_green_button = digitalRead(green_button_pin);
    if (val_green_button == HIGH) {
        stop();
        delay(1000);
        waitForButtonPress();
        delay(1000);
        goForwards();
    }

    updateFlashingLED();

    // Update sensors
    updateLineSensorReadings();
    updateTOFReading();

    if ((current_block_status == no_block) && (tof_distance < 30)) {
        handleGridBlockFound();
    } else if (detectJunction()) {
        handleJunction();
        if (path.isEmpty()) {
            if (current_node == -1) {
                depositBlock();
            } else if (current_node == 14) {
                freeSearch();
            }
        }
    } else {
        lineFollow();
    }
}
