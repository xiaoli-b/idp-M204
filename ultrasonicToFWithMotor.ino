#define MAX_RANG (520)//the max measurement value of the module is 520cm(a little bit longer
#define ADC_SOLUTION (1023.0)//ADC accuracy of Arduino UNO is 10bit
#include <Adafruit_MotorShield.h>
#include "Arduino.h"
#include "Wire.h"
#include "VL53L0X.h"
VL53L0X sensor;

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *motor1 = AFMS.getMotor(3);
// You can also make another motor on port M2
Adafruit_DCMotor *motor2 = AFMS.getMotor(4);

int sensityPin = A3; // select the input pin for ultrasonic
void setup() {

  // ultrasonic

 // Serial init 
 Serial.begin(9600);
  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

   // Set the speed to start, from 0 (off) to 255 (max speed)
  motor1->setSpeed(255);
  motor2->setSpeed(255);
  motor1->run(FORWARD);
  motor2->run(FORWARD);
  // turn on motor
  motor1->run(RELEASE);
  motor2->run(RELEASE);


  // time of flight
  Wire.begin();
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

}
float dist_t, sensity_t; // ultrasonic
void loop() {

  // reading ultrasonic
 // read the value from the sensor:
sensity_t = analogRead(sensityPin);
 // turn the ledPin on
dist_t = sensity_t * MAX_RANG / ADC_SOLUTION;//

// reading ToF

// if ultrasonic is too low - we've seen the block
  
if(dist_t < 100){
  Serial.print("tick");

  motor1->run(FORWARD);
  motor2->run(BACKWARD);
  // i = 200;

  // motor1->setSpeed(i);
  // motor2->setSpeed(i);
  // delay(10);


  for (i=0; i<255; i++) {
    motor1->setSpeed(i);
    motor2->setSpeed(i);
    delay(4.5);
  }
  for (i=255; i!=0; i--) {
    motor1->setSpeed(i);
    motor2->setSpeed(i);
    delay(4.5);
  }

  motor1 -> run(FORWARD);
  motor2 -> run(FORWARD);
  
}

// if tof is too low - we have the block

else if (sensor.readRangeSingleMillimeters < 25){
  myMotor -> setSpeed(0);
  myOtherMotor -> setSpeed(0);
  myMotor -> run(RELEASE);
  myOtherMotor -> run(RELEASE);

}
Serial.print(dist_t,0);
Serial.println("cm");
delay(500);
}

Serial.print(sensor.readRangeSingleMillimeters());
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); 
  }


