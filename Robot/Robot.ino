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
const int led_R = 4; // red LED
const int led_G = 5; // green LED
const int led_B = 10; // blue LED

//hall sensor
const int inputMagneticPin = 3; // choose the input pin
int valMagnetic; // variable for reading the pin status

//push buttons
const int pushGreen = 2; // push button pin

// Robot attributes
enum Direction { north, east, south, west };
Direction current_direction;
int current_node;
cppQueue path(sizeof(int));
enum Status { line_following, turning };
Status current_status;


int update_line_sensor_readings() {
    for (int i = 0; i < 4; i++) 
    {
        line_sensor_readings[i] = digitalRead(LINE_SENSOR_PINS[i]);
    }
}

void set_motors(int new_left_speed, int new_right_speed) {
    left_speed = new_left_speed * MOTOR_SPEED_FACTOR[0];
    right_speed = new_right_speed * MOTOR_SPEED_FACTOR[1];
    left_motor -> setSpeed(abs(left_speed));
    right_motor -> setSpeed(abs(right_speed));
    left_motor -> run((left_speed == 0) ? RELEASE : (left_speed > 0) ? BACKWARD : FORWARD);
    right_motor -> run((right_speed == 0) ? RELEASE : (right_speed > 0) ? BACKWARD : FORWARD);
}

void forwards(){
    set_motors(230, 230);
}
void junctionRight(){
    set_motors(169, -169);
}

void junctionLeft(){
    set_motors(-169, 169);
}

void turnRight(){
    set_motors(200, 0);
}

void turnLeft(){
    set_motors(0, 200);
}

void Stop(){
    set_motors(0, 0);
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

    // pinMode(led_B, OUTPUT);
    // pinMode(led_G, OUTPUT);
    // pinMode(led_R, OUTPUT);

    // pinMode(pushGreen, INPUT);
    // pinMode(inputMagneticPin, INPUT);
    
    // magnetic = false;

    forwards();
    delay(1000);

    current_node = -1;
    current_direction = north;
    int new_path[10] = { 2, 3, 4, 9, 8, 7, 6, 5, 0, 1 };
    for (int node : new_path) {
        path.push(node);
    }
    current_status = line_following;

}

void loop(){
    update_line_sensor_readings();
    for (int reading : line_sensor_readings){
        Serial.print(reading);
    }
    Serial.println();

    // Line following
    if ((line_sensor_readings[1] == 1) && (line_sensor_readings[2] == 0)) {
        // Deviating right
        turnLeft();
      } else if ((line_sensor_readings[1] == 0) && (line_sensor_readings[2] == 1)) {
        // Deviating left
        turnRight();
      } else {
        forwards();
      }

}