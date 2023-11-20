//Code for search and retrieval of first two blocks

#include <Adafruit_MotorShield.h>
#include "Wire.h"
#include "VL53L0X.h"

//time of flight sensor
VL53L0X sensor; 
uint16_t distance;


Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *leftMotor= AFMS.getMotor(3); //left motor
Adafruit_DCMotor *rightMotor = AFMS.getMotor(4); // right motor
int leftSpeed;
int rightSpeed;

//line sensors
const int L = 6; //back left sensor
int backleft;
const int R = 7; // back right sensor
int backright;
const int F_L = 8; // front right sensor
int frontleft;
const int F_R = 9; // front left sensor
int frontright;

#define MAX_RANG  (520)//the max measurement value of the module is 520cm(a little bit longer than effective max range)
#define ADC_SOLUTION  (1023.0)//ADC accuracy of Arduino UNO is 10bit 

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


bool turning;
int junctionCount = 0;

//robot conditions
int searchcondition;
// 0 - line following 
// 1 - junction detection
// 2 - block found
int depositcondition;
// 0 - line following 
// 1 - junction detection
// 2 - 
int subcondition;
int findCounter = 0;
int depositCounter = 0;

void setup() {

  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Adafruit Motorshield v2 - DC Motor test!");

  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  Wire.begin();
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  // defining pin modes
  pinMode(L, INPUT); 
  pinMode(R, INPUT); 
  pinMode(F_R, INPUT);
  pinMode(F_L, INPUT);

  pinMode(led_B, OUTPUT);
  pinMode(led_G, OUTPUT);
  pinMode(led_R, OUTPUT);

  pinMode(pushGreen, INPUT);
  pinMode(inputMagneticPin, INPUT);

  searchcondition = 0;
  depositcondition = 0;
  subcondition = 0;
  turning = false;

  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(FORWARD);
  rightMotor -> run(FORWARD);
  delay(2500);
}

void loop(){
  backleft = digitalRead(L);
  backright = digitalRead(R);
  frontleft = digitalRead(F_L);
  frontright = digitalRead(F_R);

  distance = sensor.readRangeSingleMillimeters();
  Serial.println(distance);
  
  if ((leftSpeed > 0) || (rightSpeed > 0)) {
    digitalWrite(led_B, ((millis() / 500) % 2));
  }
  else {
    digitalWrite(led_B, LOW);
  }

  if ((findCounter == 0) && (depositCounter == 0)){
    searchFirst();
  }
  else if ((findCounter == 1) && (depositCounter == 0)){
    depositFirst();
  }
  else if ((findCounter == 1) && (depositCounter == 1)){
    searchFirst();
  }
  else if ((findCounter == 2) && (depositCounter == 1)){
    depositFirst();
  }
}

void searchFirst() {
  if (distance < 25) {
    forwards();
    delay(500);
    Stop();
    valMagnetic = digitalRead(inputMagneticPin);
    if(valMagnetic == HIGH){ // magnetic
      digitalWrite(led_R, HIGH); //Turn off led
      delay(5500); // needs to be 5 seconds
      digitalWrite(led_R, LOW); //Turn on led
      delay(1000);// turn LED OFF
    }
    
    // GREEN LED FOR NON-MAGNETIC
    else if(valMagnetic == LOW){ // not magnetic
      digitalWrite(led_G, HIGH); //Turn off led
      delay(5500); // needs to be 5 seconds
      digitalWrite(led_G, LOW); //Turn on led
      delay(1000);// turn LED OFF
    }
  findCounter++;
  }
  if (searchcondition == 0) {
    if (junctionCheck() == true) {
      searchcondition = 1;
      Stop();
      delay(500);
    } else {
      if ((frontleft == 1) && (frontright == 0)) {
        turnLeft();
      } else if ((frontleft == 0) && (frontright == 1)) {
        turnRight();
      } else {
        forwards();
      }
    }
  } else if (searchcondition == 1) {
    if (subcondition == 0) {
      if (junctionDecide() == "left") {
        subcondition = 1;
      }
      if (junctionDecide() == "right") {
        subcondition = 2;
      }
      if (junctionDecide() == "straight") {
        subcondition = 0;
        forwards();
        delay(2000);
      }
    } else {
      if (!turning) {
        turning = true;
        if (subcondition == 1) {
          junctionLeft();
        }
        if (subcondition == 2) {
          junctionRight();
        }
        delay(500);
      } else if (turning) {
        if ((backleft == 0) || (backright == 0)) {
          if (subcondition == 1) {
            junctionLeft();
          }
          if (subcondition == 2) {
            junctionRight();
          }
        } else {
          Stop();
          delay(1000);
          turning = false;
          forwards();
          delay(500);
          if ((frontleft == 1) && (frontright == 0)) {
            turnLeft();
          } else if ((frontleft == 0) && (frontright == 1)) {
            turnRight();
          } else {
            forwards();
          }
          subcondition = 0;
          searchcondition = 0;
          junctionCount++;
        }
      }
    }
  }
}

void depositFirst() {
  return true;
}
void forwards(){
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(FORWARD);
  rightMotor -> run(FORWARD);
  leftSpeed = 255;
  rightSpeed = 255;
}

void junctionRight(){
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(FORWARD);
  rightMotor -> run(BACKWARD);
  leftSpeed = 255;
  rightSpeed = 255;
}

void junctionLeft(){
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(FORWARD);
  leftSpeed = 255;
  rightSpeed = 255;
}

void turnRight(){
  leftMotor -> setSpeed(125);
  rightMotor -> setSpeed(0);
  leftMotor -> run(FORWARD);
  rightMotor -> run(RELEASE);
  leftSpeed = 125;
  rightSpeed = 0;


}

void turnLeft(){
  leftMotor -> setSpeed(0);
  rightMotor -> setSpeed(125);
  leftMotor -> run(RELEASE);
  rightMotor -> run(FORWARD);
  leftSpeed = 0;
  rightSpeed = 125;


}

void Stop(){
  leftMotor -> setSpeed(0);
  rightMotor -> setSpeed(0);
  leftMotor -> run(RELEASE);
  rightMotor -> run(RELEASE);
  delay(100);
  leftSpeed = 0;
  rightSpeed = 0;
  }

bool junctionCheck(){
  if ((backleft == 1) || (backright == 1)){
    return true;
  }
  else {
    return false;
  }
}

String junctionDecide(){

 if (junctionCount == 0) {
    // Turn right
    return "right";
  } 
  else if (junctionCount == 1) {
    // Go straight
    return "straight";
  } 
  else if (junctionCount == 2 || junctionCount == 3) {
    // Turn left
    return "left";
  } else if (junctionCount >= 4 && junctionCount <= 7) {
    // Go straight
    return "straight";
  } else if (junctionCount == 8 || junctionCount == 9) {
    // Turn left
    return "left";
  } else if (junctionCount == 10) {
    // Reset junctionCount
    junctionCount = 0;
    return "straight";

  }
}





