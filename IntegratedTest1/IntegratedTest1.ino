//Code for search and retrieval of first two blocks

#include <Adafruit_MotorShield.h>
#include "Wire.h"
#include "VL53L0X.h"

//time of flight sensor
VL53L0X sensor; 
uint16_t distance = 0;


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
int rjunctionCount = 0;
int rjunctionCount2 = 0;

int turndegree = 0;
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
// 0 - going straight
// 1 - going left
//2 - going right
int findCounter = 0;
int depositCounter = 0;
bool magnetic;

void forwards(){
  leftMotor -> setSpeed(230);
  rightMotor -> setSpeed(204);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(BACKWARD);
  leftSpeed = 230;
  rightSpeed = 204;
}

void junctionRight(){
  leftMotor -> setSpeed(169);
  rightMotor -> setSpeed(150);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(FORWARD);
  leftSpeed = 169;
  rightSpeed = 150;
}

void junctionLeft(){
  leftMotor -> setSpeed(169);
  rightMotor -> setSpeed(150);
  leftMotor -> run(FORWARD);
  rightMotor -> run(BACKWARD);
  leftSpeed = 169;
  rightSpeed = 150;
}

void turnRight(){
  leftMotor -> setSpeed(200);
  rightMotor -> setSpeed(0);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(RELEASE);
  leftSpeed = 125;
  rightSpeed = 0;


}

void turnLeft(){
  leftMotor -> setSpeed(0);
  rightMotor -> setSpeed(200);
  leftMotor -> run(RELEASE);
  rightMotor -> run(BACKWARD);
  leftSpeed = 0;
  rightSpeed = 145;


}

void Stop(){
  leftMotor -> setSpeed(0);
  rightMotor -> setSpeed(0);
  leftMotor -> run(RELEASE);
  rightMotor -> run(RELEASE);
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

void rotate180() {
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(FORWARD);
  rightMotor -> run(BACKWARD);
  delay(2520);
  leftSpeed = 255;
  rightSpeed = 255;
}

void rotate90R() {
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(FORWARD);
  rightMotor -> run(BACKWARD);
  delay(1260);
  leftSpeed = 255;
  rightSpeed = 255;
}
void rotate90L() {
  leftMotor -> setSpeed(255);
  rightMotor -> setSpeed(255);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(FORWARD);
  delay(1260);
  leftSpeed = 255;
  rightSpeed = 255;
}

String j0Decide2() {
  rotate180();
}
String j1Decide2() {
  rotate180();
}
String j2Decide2() {
  rotate180();
}
String j3Decide2() {
  rotate180();
}
String j4Decide2() {
  rotate90R();
}
String j5Decide2(){
  rotate90L();
}
String j6Decide2() {
  rotate180(); 
}
String j7Decide2() {
  rotate180(); 
}
String j8Decide2() {
  rotate180(); 
}
String j9Decide2(){
  return;
}

String j0Decide() {
  if (rjunctionCount2== 0){
    return "stop";
  }  
}

String j1Decide() {
  if (rjunctionCount2== 0) {
    // Turn left
    return "left";
  } 
  else if (rjunctionCount2== 1) {
    // Go straight
    return "stop";
}
}

String j2Decide() {
  if (rjunctionCount2== 0) {
    // 
    return "straight";
  } 
  else if (rjunctionCount2== 1) {
    // Go straight
    return "left";
  } 
  else if (rjunctionCount2== 2) {
    // Turn left
    return "stop";
}
}

String j3Decide() {
  if (rjunctionCount2== 0) {
    // Turn right
    return "right";
  } 
  else if (rjunctionCount2== 1) {
    // Go straight
    return "straight";
  } 
  else if (rjunctionCount2== 2) {
    // Turn left
    return "left";

  } else if (rjunctionCount2== 3) {
    // Go straight
    return "stop";
  } 

}

String j4Decide() {
  if (rjunctionCount2== 0) {
    // Turn right
    return "right";
  } 
  else if (rjunctionCount2== 1) {
    // Go straight
    return "left";
  } 
  else if (rjunctionCount2== 2) {
    // Turn left
    return "stop";
}
}

String j5Decide(){
  if(rjunctionCount2== 0){
    // Go straight
    return "straight";
  } else if(rjunctionCount2== 1){
    return "stop";
  }
}

String j6Decide(){
  if(rjunctionCount2== 0){
    // Turn right
    return "right";
  } else if (rjunctionCount2== 1) {
    // Go straight
    return "straight";
  } else if (rjunctionCount2== 2) {
    // stop
    return "stop";
}
}

String j7Decide() {
  if(rjunctionCount2== 0){
    // Go straight
    return "straight";
  }
  else if(rjunctionCount2== 1){
    // Turn right
    return "right";
  } else if (rjunctionCount2== 2) {
    // Go straight
    return "straight";
  } else if (rjunctionCount2== 3) {
    // stop
    return "stop";
  } 
}

String j8Decide() {
  if (rjunctionCount2== 0) {
    // Turn right
    return "right";
  } 
  else if (rjunctionCount2== 1) {
    // Go straight
    return "straight";
  } 
  else if (rjunctionCount2== 2) {
    // Turn left
    return "right";

  } else if (rjunctionCount2== 3) {
    // Go straight
    return "straight";
  } else if (rjunctionCount2== 4) {
    // stop
    return "stop";
  } 
}

String j9Decide(){
  if(rjunctionCount2== 0){
    // turn right
    return "right";
  } else if(rjunctionCount2== 1){
    // stop
    return "stop";
  }
}

void j0Retrieve(){
  if (subcondition == 0) {
      if (j0Decide() == "left") {
        subcondition = 1;
      }
      else if (j0Decide() == "right") {
        subcondition = 2;
      }
      else if (j0Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j0Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j1Retrieve(){
  if (subcondition == 0) {
      if (j1Decide() == "left") {
        subcondition = 1;
      }
      else if (j1Decide() == "right") {
        subcondition = 2;
      }
      else if (j1Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j1Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j2Retrieve(){
  if (subcondition == 0) {
      if (j2Decide() == "left") {
        subcondition = 1;
      }
      else if (j2Decide() == "right") {
        subcondition = 2;
      }
      else if (j2Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j2Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j3Retrieve(){
  if (subcondition == 0) {
      if (j3Decide() == "left") {
        subcondition = 1;
      }
      else if (j3Decide() == "right") {
        subcondition = 2;
      }
      else if (j3Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j3Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j4Retrieve(){
  if (subcondition == 0) {
      if (j4Decide() == "left") {
        subcondition = 1;
      }
      else if (j4Decide() == "right") {
        subcondition = 2;
      }
      else if (j4Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j4Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j5Retrieve(){
  if (subcondition == 0) {
      if (j5Decide() == "left") {
        subcondition = 1;
      }
      else if (j5Decide() == "right") {
        subcondition = 2;
      }
      else if (j5Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j5Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j6Retrieve(){
  if (subcondition == 0) {
      if (j6Decide() == "left") {
        subcondition = 1;
      }
      else if (j6Decide() == "right") {
        subcondition = 2;
      }
      else if (j6Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j6Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j7Retrieve(){
  if (subcondition == 0) {
      if (j7Decide() == "left") {
        subcondition = 1;
      }
      else if (j7Decide() == "right") {
        subcondition = 2;
      }
      else if (j7Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j7Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j8Retrieve(){
  if (subcondition == 0) {
      if (j8Decide() == "left") {
        subcondition = 1;
      }
      else if (j8Decide() == "right") {
        subcondition = 2;
      }
      else if (j8Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j8Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
}

void j9Retrieve(){
  if (subcondition == 0) {
      if (j9Decide() == "left") {
        subcondition = 1;
      }
      else if (j9Decide() == "right") {
        subcondition = 2;
      }
      else if (j9Decide() == "straight") {
        subcondition = 0;
        forwards();
        delay(1000);
      }
      else if (j9Decide() == "stop") {
        subcondition = 0;
        Stop();
        delay(1000);
        depositcondition = 3;
        return;
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
        delay(200);
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
          depositcondition = 1;
          rjunctionCount2++;
        }
      }
    }
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
  pinMode(L, INPUT); 
  pinMode(R, INPUT); 
  pinMode(F_R, INPUT);
  pinMode(F_L, INPUT);

  // pinMode(led_B, OUTPUT);
  // pinMode(led_G, OUTPUT);
  // pinMode(led_R, OUTPUT);

  // pinMode(pushGreen, INPUT);
  // pinMode(inputMagneticPin, INPUT);

  searchcondition = 0;
  depositcondition = 0;
  subcondition = 0;
  turning = false;
  
  magnetic = false;

  leftMotor -> setSpeed(230);
  rightMotor -> setSpeed(204);
  leftMotor -> run(BACKWARD);
  rightMotor -> run(BACKWARD);
  delay(1000);
}

void loop(){
  backleft = digitalRead(L);
  backright = digitalRead(R);
  frontleft = digitalRead(F_L);
  frontright = digitalRead(F_R);

  //distance = sensor.readRangeSingleMillimeters();
  //Serial.println(distance);
  Serial.println(backleft);
  Serial.println(backright);
  
  // if ((leftSpeed > 0) || (rightSpeed > 0)) {
  //   digitalWrite(led_B, ((millis() / 500) % 2));
  // }
  // else {
  //   digitalWrite(led_B, LOW);
  // }

  if ((findCounter == 0) && (depositCounter == 0)){
    searchFirst();
    Serial.println("Search 1");
  }
  else if ((findCounter == 1) && (depositCounter == 0)){
    depositFirst();
    Serial.println("Deposit 1");


  }
  else if ((findCounter == 1) && (depositCounter == 1)){
    searchFirst();
    Serial.println("Search 1");
  }
  else if ((findCounter == 2) && (depositCounter == 1)){
    depositFirst();
    Serial.println("Search 1");
  }
}

void searchFirst() {
  // if (distance < 25) {
  //   forwards();
  //   delay(500);
  //   Stop();
  //   valMagnetic = digitalRead(inputMagneticPin);
  //   if(valMagnetic == HIGH){ // magnetic
  //     digitalWrite(led_R, HIGH); //Turn off led
  //     delay(5500); // needs to be 5 seconds
  //     digitalWrite(led_R, LOW); //Turn on led
  //     delay(1000);// turn LED OFF
  //     magnetic = true;
  //   }
    
  //   // GREEN LED FOR NON-MAGNETIC
  //   else if(valMagnetic == LOW){ // not magnetic
  //     digitalWrite(led_G, HIGH); //Turn off led
  //     delay(5500); // needs to be 5 seconds
  //     digitalWrite(led_G, LOW); //Turn on led
  //     delay(1000);// turn LED OFF
  //     magnetic = false;
  //   }
  // findCounter++;
  // rjunctionCount = junctionCount;
  // junctionCount = 0;
  // return;
  // }
  if (searchcondition == 0) {
    if (junctionCheck() == true) {
      searchcondition = 1;
      Stop();
      delay(1000);
    } else {
      Serial.println("Line correcting");
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
        delay(1000);
        searchcondition = 0;
        junctionCount++;
      }
    } else {
      if (!turning) {
        turning = true;
        if (subcondition == 1) {
          Stop();
          junctionLeft();
          delay(700);
        }
        else if (subcondition == 2) {
          Stop();
          junctionRight();
          delay(700);
        }
      } else if (turning) {
        if ((frontleft == 0) || (frontright == 0)) {
          if (subcondition == 1) {
            junctionLeft();
          }
          if (subcondition == 2) {
            junctionRight();
          }
        } else {
  
          turning = false;
          forwards();
          delay(200);
          subcondition = 0;
          searchcondition = 0;
          junctionCount++;
        }
      }
    }
  }
}

void depositFirst() {
  if (depositcondition == 0) {
    switch (rjunctionCount) {
        case 0:
            j0Decide2();
            break;
        case 1:
            j1Decide2();
            break;
        case 2:
            j2Decide2();
            break;
        case 3:
            j3Decide2();
            break;
        case 4:
            j4Decide2();
            break;
        case 5:
            j5Decide2();
            break;
        case 6:
            j6Decide2();
            break;
        case 7:
            j7Decide2();
            break;
        case 8:
            j8Decide2();
            break;
        case 9:
            j9Decide2();
            break;
    }
    depositcondition = 1;
  }
  else if (depositcondition == 1) {
     if (junctionCheck() == true) {
      depositcondition = 2;
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
  }
  else if (depositcondition == 2) {
    switch (rjunctionCount) {
        case 0:
            j0Retrieve();
            break;
        case 1:
            j1Retrieve();
            break;
        case 2:
           j2Retrieve();
            break;
        case 3:
            j3Retrieve();
            break;
        case 4:
            j4Retrieve();
            break;
        case 5:
            j5Retrieve();
            break;
        case 6:
            j6Retrieve();
            break;
        case 7:
            j7Retrieve();
            break;
        case 8:
            j8Retrieve();
            break;
        case 9:
            j9Retrieve();
            break;
    }
  }
  else if (depositcondition == 3) {
    forwards();
    delay(750);
    if (magnetic == true){
      rotate90R();
      forwards();
      delay(3000);
      Stop();
      delay(1000);
      rotate180();
      forwards();
      rotate90L();
      Stop();
      delay(5000);
      forwards();
      delay(250);
    }
    else if (magnetic == false){
      rotate90L();
      forwards();
      delay(3000);
      Stop();
      delay(1000);
      rotate180();
      forwards();
      rotate90R();
      Stop();
      delay(5000);
      forwards();
      delay(250);
    }
    magnetic = false;
    depositCounter++;
    rjunctionCount = 0;
    rjunctionCount2 = 0;
    
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
