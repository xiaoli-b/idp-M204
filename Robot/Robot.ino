//Code for search and retrieval of first two blocks

#include <Adafruit_MotorShield.h>
#include "Wire.h"
#include "VL53L0X.h"
#include <cppQueue.h>

//time of flight sensor
VL53L0X sensor; 
uint16_t distance = 0;


Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *left_motor= AFMS.getMotor(3); //left motor
Adafruit_DCMotor *right_motor = AFMS.getMotor(4); // right motor
int left_speed;
int right_speed;
const float MOTOR_SPEED_FACTOR[2] = { 1, 0.88 };

#define MAX_RANG  (520)//the max measurement value of the module is 520cm(a little bit longer than effective max range)
#define ADC_SOLUTION  (1023.0)//ADC accuracy of Arduino UNO is 10bit 

//line sensors
const int LINE_SENSOR_PINS[4] = { 6, 8, 9, 7 }; // [BL, FL, FR, BR]
int line_sensor_readings[4]; // [BL, FL, FR, BR]

//ultrasonic sensor
int sensityPin = A3;
float dist, sensity;

//led 
const int led_R = 5; // red LED
const int led_G = 7; // green LED
const int led_B = 4; // blue LED

//hall sensor
const int inputMagneticPin = 3; // choose the input pin
int valMagnetic; // variable for reading the pin status

//push buttons
const int green_button_pin = 2; // push button pin
int val_green_button;


// Robot attributes
enum Direction { north, east, south, west };
Direction current_direction;
int current_node;
cppQueue path(sizeof(int));
enum Status { line_following, turning };
Status current_status;

enum Turn { left90=-1, straight=0, right90=1, turn180=2 };


// class Queue{
//     public:
//         int max_length;
//         int start;
//         int end;
//         int* queue;
//         bool push(int x) {
//             if e
//             end = (end+1)%max_length;
//             queue[end] = x;
//         }
// }

void waitForButtonPress() {
    do {
        val_green_button = digitalRead(green_button_pin);
    } while (val_green_button == LOW);
}

int updateLineSensorReadings() {
    for (int i = 0; i < 4; i++) 
    {
        line_sensor_readings[i] = digitalRead(LINE_SENSOR_PINS[i]);
    }
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
    setMotors(200, 200);
}
void spinRight(){
    setMotors(160, -160);
}

void spinLeft(){
    setMotors(-160, 160);
}

void turnRight(){
    setMotors(145, 0);
}

void turnLeft(){
    setMotors(0, 145);
}

void stop(){
    setMotors(0, 0);
}

bool detectJunction(){
    // Are we currently at a junction
    
    return ((line_sensor_readings[0]==1) || (line_sensor_readings[3]==1));
}

Direction getDesiredDirection(int start_node, int end_node) {
    // Get the compass direction between two adjacent nodes

    if ((start_node/5) == (end_node/5)) {
        // nodes are on the same line
        int difference = end_node - start_node;
        if (abs(difference) != 1){
            Serial.println("Not sure how I got here");
        } else {
            return difference > 0 ? east : west;
        }
    } else if ((start_node % 5) == (end_node % 5)) {
        int difference = (end_node / 5) - (start_node / 5);
        return difference > 0 ? north : south;
    } else {
        Serial.println("Not sure how I got here either");
    }
}

Turn getDesiredTurn(Direction start_direction, Direction end_direction) {
    int difference = (((end_direction - start_direction) + 1) % 4) -1; // -1, 0, 1 or 2
    return Turn(difference);
}

void lineFollow() {
    // Line following

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
    // Having already started a turn, continue turning until you hit the perpendicular white line
    
    while (true) {
        // Turn until front sensors are off the line
        updateLineSensorReadings();
        if ((line_sensor_readings[1] == line_sensor_readings[2]) && (line_sensor_readings[2] == 0)) {
            break;
        }
    }
    while (true) {
        // Continue turning until the front sensors are back on a white line
        updateLineSensorReadings();
        if ((line_sensor_readings[1] == line_sensor_readings[2]) && (line_sensor_readings[2] == 1)) {
            break;
        }
    }
}

void makeTurn(Turn turn) {
    switch (turn) {
        case left90:
            spinLeft();
            break;
        case straight:
            goForwards();
            return;
        case right90:
            spinRight();
            break;
        case turn180:
            spinRight();
            break;
    }
    delay(500);
    turnUntilNextLine();
    if (turn == turn180) {
        turnUntilNextLine();
    }
}

void handleJunction() {
    digitalWrite(led_R, HIGH);
    Serial.println("Junction detected");
    int new_current_node;
    path.pop(&new_current_node);
    int next_node;
    path.peek(&next_node);
    current_node = new_current_node;

    Serial.print("Next segment: ");
    Serial.print(current_node);
    Serial.print("-->");
    Serial.print(next_node);
    Serial.println();

    Direction desired_direction = getDesiredDirection(current_node, next_node);
    Turn desired_turn = getDesiredTurn(current_direction, desired_direction);

    Serial.print("Desired direction: ");
    Serial.print(desired_direction);
    Serial.println();
    Serial.print("Desired turn: ");
    Serial.print(desired_turn);
    Serial.println();

    makeTurn(desired_turn);
    goForwards();
    delay(500);
    digitalWrite(led_R, LOW);
}

void setup() {

    Serial.begin(9600);           // set up Serial library at 9600 bps
    Serial.println("Adafruit Motorshield v2 - DC Motor test!");

    if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
        Serial.println("Could not find Motor Shield. Check wiring.");
        while (1);
    }
    Serial.println("Motor Shield found.");

    // Wire.begin();
    // sensor.setTimeout(500);
    // if (!sensor.init())
    // {
    //   Serial.println("Failed to detect and initialize sensor!");
    //   while (1) {}
    // }

    // defining pin modes
    for (int pin : LINE_SENSOR_PINS) {
        pinMode(pin, INPUT);
    }

    pinMode(led_B, OUTPUT);
    pinMode(led_G, OUTPUT);
    pinMode(led_R, OUTPUT);

    pinMode(green_button_pin, INPUT);
    waitForButtonPress();
    // pinMode(inputMagneticPin, INPUT);
    
    // magnetic = false;

    current_node = -1;
    current_direction = north;
    int new_path[10] = { 2, 3, 4, 9, 8, 7, 6, 5, 0, 1 };
    for (int n : new_path) {
        path.push(&n);
    }

    // Serial.println("Testing queue");
    // int i = 0;
    // while (!path.isEmpty()) {
    //     int c;
    //     path.pop(&c);
    //     Serial.print(c);
    //     Serial.print("-->");
    //     int n;
    //     path.peek(&n);
    //     Serial.print(n);
    //     Serial.println();
        // i++;
        // Serial.println(i);
    // }
    // while (true) {
    //     delay(1000);
    // }

    current_status = line_following;

    goForwards();
    delay(1000);
}

void loop(){
    val_green_button = digitalRead(green_button_pin);
    if (val_green_button == HIGH) {
        stop();
        waitForButtonPress();
        goForwards();
    }
    updateLineSensorReadings();
    // for (int reading : line_sensor_readings){
    //     Serial.print(reading);
    // }
    // Serial.println();

    lineFollow();

    if (detectJunction()) {
        handleJunction();
    }

}