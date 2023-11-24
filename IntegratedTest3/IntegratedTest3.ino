// Code for search and retrieval of first two blocks

// TODO: 
// - Handle block at node 2
// - Recalibrate block depositing
// - Stopping after collecting second block sometimes
// - When looking for the second grid block, don't search the same nodes twice

/* 
Node numbering for block retrieval (start heading towards node 2):

|           |
5--6--7--8--9
|  |  |  |  |
0--1--2--3--4
|     |     |

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

enum Turn { left90=-1, straight=0, right90=1, turn180=2 };

// Board constants
const int ANTICLOCKWISE_PATH[] = { 2, 3, 4, 9, 8, 7, 6, 5, 0, 1 };
const int FIRST_RETRIEVAL_RETURN_PATH[] = { 5, 2, -1, 2, 3, 6, 7, 2, 3, 4 };
/* The i^th element of the above array is the next node to travel to from node i
on the return journey having just collected a block. This prevents crossing a
node that hasn't already been crossed, which may contain the second block. */
const int FREE_SPACE_SETUP_PATH[5] = { 2, 3, 4, 9, 14 }; 
// Path from start to top right to begin searching free space
const int FREE_SPACE_RETURN_PATH[5] = { 10, 5, 0, 1, 2 };
// Path from top left back to start


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

void updateUltrasoundReading() {
    ultrasound_sensity = analogRead(ultrasound_pin);
    ultrasound_distance = ultrasound_sensity * MAX_RANG / ADC_SOLUTION;
}

void printLineSensorReadings() {
    for (int reading : line_sensor_readings) {
        Serial.print(reading);
    }
    Serial.println();
}

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

void goBackwards(){
    setMotors(-145, -160);
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

void rotate180() {
    setMotors(150, -150);
    delay(2750);
    turnUntilNextLine();
    delay(100);
    stop();
}

void rotate90R() {
    setMotors(150, -150);
    delay(2100);
    stop();
}

void rotate90L() {
    // Could use side sensors rather than timing
    setMotors(-150, 150);
    delay(1950);
    stop();
}

bool detectJunction(){
    /* Are we currently at a junction */
    
    return ((line_sensor_readings[0]==1) || (line_sensor_readings[3]==1));
}

Direction getDesiredDirection(int start_node, int end_node) {
    /* Get the compass direction between two adjacent nodes */

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

Turn getDesiredTurn(Direction start_direction, Direction end_direction) {
    /* Get the turn required to get from one compass direction to another */
    int difference = mod(end_direction - start_direction + 1,  4) - 1; 
    // -1, 0, 1, 2 --> 90L, none, 90R, 180
    return Turn(difference);
}

void lineFollow() {
    /* Line following */

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

void turnUntilNextLine() {
    /* Having already started a turn, continue turning until you hit the perpendicular white line */

    Serial.print("Beginning turn to next line. LSRs: ");
    printLineSensorReadings();

    // while (true) {
    //     // Turn until front sensors are off the line
    //     updateLineSensorReadings();
    //     if ((line_sensor_readings[1] == line_sensor_readings[2]) && (line_sensor_readings[2] == 0)) {
    //         break;
    //     }
    // }

    Serial.print("Continuing turn until front sensors hit line. LSRs: ");
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

void makeTurn(Turn turn) {
    goForwards();
    delay(50);
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
            spinRight();
            break;
    }
    delay(750);
    turnUntilNextLine();
    if (turn == turn180) {
        turnUntilNextLine();
    }
}

void handleJunction() {
    stop();
    delay(500);
    Serial.println("Junction detected");
    Serial.print("LSRs at junction: ");
    printLineSensorReadings();
    
    path.pop(&current_node);
    Direction desired_direction;

    if (path.isEmpty()) {
        Serial.print("Path empty. Handling behaviour for node ");
        Serial.println(current_node);
        if (current_node == -1) {
            Serial.println("Case -1");
            desired_direction = south;
        } else if (current_node == 2) {
            Serial.println("Case 2");
            desired_direction = south;
            int next_node = -1;
            path.push(&next_node);
        } else if (current_node == 14) {
            Serial.println("Case 14");
            desired_direction = west;
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

void handleBlockFound() {
    goForwards();
    delay(300);
    stop();
    delay(500);
    val_magnetic_sensor = digitalRead(magnetic_sensor_pin);
    delay(500);
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
    
    setReturnPath();
    rotate180();
    current_direction = mod(current_direction + 2, 4);
    goForwards();
    delay(500); /* Increasing this delay should help the post-block confusion
    but will cause issues if the 180 turn isn't perfect */
}

void setReturnPath() {
    // TODO: Be smart and choose the shortest path possible when retrieving the 
    // second block. Issue currently is handling the node/junction underneath the block.

    path.clean();
    int next_node = current_node;
    path.push(&next_node);
    while (next_node != 2) {
        next_node = FIRST_RETRIEVAL_RETURN_PATH[next_node];
        path.push(&next_node);
    }
    if (path.isEmpty()) {
        panic();
    }
}

void depositBlock() {
    /* Predefined code depositing a block. Starting from the top of the start box facing
    south, finishing past the top of the start block facing north (on the way to 2) */

    goForwards();
    delay(1350);
    stop();
    delay(500);
    if (current_block_status == magnetic) {
        rotate90R();
    } else {
        rotate90L();
    }
    goForwards();
    delay(7100);
    stop();
    delay(1000);
    goBackwards();
    delay(8200);
    stop();
    delay(500);
    if (current_block_status == magnetic) {
        rotate90R();
    } else {
        rotate90L();
    }
    stop();
    delay(5000);
    goForwards();
    delay(200);
    number_of_blocks_retrieved++;
    current_block_status = no_block;
    current_direction = north;
    current_node = -1;
    if (number_of_blocks_retrieved < 2){
        for (int n : ANTICLOCKWISE_PATH) {
            path.push(&n);
        }
    } else {
        if (number_of_blocks_retrieved == 2) {
            digitalWrite(led_B, HIGH);
            delay(5500);
            digitalWrite(led_B, LOW);
        }
        for (int n : FREE_SPACE_SETUP_PATH) {
            path.push(&n);
        }
    }
    goForwards();
    delay(2000);
}

void freeSearch() {
    /* Starts at junction 14 */

    bool block_detected = false;
    // Drive around free space until a block is detected
    while (!block_detected) {
        block_detected = driveAroundFreeSpaceLookingForBlock();
    }

    getFreeSpaceBlock();
    turnToDesiredDirection(west);
    
    current_node = 11;
    for (int n : FREE_SPACE_RETURN_PATH) {
        path.push(&n);
    }
}

bool driveAroundFreeSpaceLookingForBlock() {
    // TODO: Lots of hard-coding here that I'm not sure I like
    bool block_detected = false;
    if (current_node != 14) {
        panic();
    }
    turnToDesiredDirection(west);

    // Drive 14-->10 checking for block
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        updateUltrasoundReading();

        if (ultrasound_distance < 60) {
            block_detected = true;
            break;
        }

        lineFollow();
    }
    if (block_detected) {
        return true;
    }
    current_node = 10;
    turnToDesiredDirection(south);

    // Drive 10-->5
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollow();
    }
    current_node = 5;
    turnToDesiredDirection(east);

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
        }

        if (ultrasound_distance < 60) {
            block_detected = true;
            break;
        }

        lineFollow();
    }

    if (block_detected) {
        return true;
    }

    turnToDesiredDirection(north);

    // Drive 9-->14
    while (!detectJunction()) {
        updateLineSensorReadings();
        updateFlashingLED();
        lineFollow();
    }
    current_node = 14;
    return false;
}

void getFreeSpaceBlock() {
    /* Having detected a block in the free space, turn left, grab it, and return to the top 
    line (facing north) ready for block retrieval */
    rotate90L();

}

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

    // Initial state of robot
    number_of_blocks_retrieved = 0;
    current_node = -1;
    current_direction = north;
    // int new_path[] = { 14 };
    for (int n : ANTICLOCKWISE_PATH) {
        path.push(&n);
    }
    current_block_status = no_block;

    goForwards();
    delay(3000);
}

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
    tof_distance = tof_sensor.readRangeSingleMillimeters();
    val_magnetic_sensor = digitalRead(magnetic_sensor_pin);

    if ((current_block_status == no_block) && (tof_distance < 30)) {
        handleBlockFound();
    } else if (detectJunction()) {
        handleJunction();
        goForwards();
        delay(300);
        if (current_node == -1) {
            depositBlock();
        } else if (current_node == 14) {
            freeSearch();
        }
    } else {
        lineFollow();
    }
}
