// Arduino Line Follower Robot Code

#include <Adafruit_MotorShield.h>
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *myMotor = AFMS.getMotor(3); //left motor
Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(4); // right motor
int L_S = 6;
int R_S = 7;
int F2_S = 8;
int F_S = 10;
bool turning = false;
int junctionCount = 0;

void setup(){ 
  pinMode(L_S, INPUT); 
  pinMode(R_S, INPUT); 
  pinMode(F2_S, INPUT);
  pinMode(F_S, INPUT);

  Serial.begin(9600);           // set up Serial library at 9600 bps
  Serial.println("Adafruit Motorshield v2 - DC Motor test!");

  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);
  delay(2500);
}

void loop(){
  LineFollow();
}

void forward(){
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);
}

void junctionRight(){
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(BACKWARD);
  delay(1260);
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);
  delay(1100);
}

void junctionLeft(){
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(BACKWARD);
  myOtherMotor -> run(FORWARD);
  delay(1260);
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);
  delay(950);
}

void turnRight(){
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(220);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);


}
void turnRight2(){
  myMotor -> setSpeed(255);
  myOtherMotor -> setSpeed(200);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);


}
void turnLeft(){
  myMotor -> setSpeed(220);
  myOtherMotor -> setSpeed(255);
  myMotor -> run(FORWARD);
  myOtherMotor -> run(FORWARD);


}

void Stop(){
  myMotor -> setSpeed(0);
  myOtherMotor -> setSpeed(0);
  myMotor -> run(RELEASE);
  myOtherMotor -> run(RELEASE);
  delay(50);
}

void LineFollow(){
  if((digitalRead(F_S) == 1)&&(digitalRead(F2_S) == 1)){
    forward();
  }
  if((digitalRead(R_S) == 1) && (digitalRead(L_S) == 1) ){
    JunctionDetection();
  }
  if((digitalRead(R_S) == 1) && (digitalRead(L_S) == 0) && (digitalRead(F_S) == 1)&&(digitalRead(F2_S) == 1) ){
    JunctionDetection();
  }
  if((digitalRead(L_S) == 1) && (digitalRead(R_S) == 0) && (digitalRead(F_S) == 1)&&(digitalRead(F2_S) == 1)){
    JunctionDetection();
  } 
  
  
  if((digitalRead(F2_S) == 0) ){
    turnRight();
    

    // Adjust the delay as needed based on your robot's behavior
  }
  if((digitalRead(F_S) == 0) ){
    turnLeft();
     // Adjust the delay as needed based on your robot's behavior
  }
  

  // Print junction count to serial monitor
  Serial.print("Junction Count: ");
  Serial.println(junctionCount);

  // Perform actions based on junctionCount
  
}

void JunctionDetection(){

 if (junctionCount == 0 ) {
    // Turn right
    junctionRight();
    delay(500); // Adjust the delay as needed based on your robot's behavior
  } 
  else if (junctionCount == 1) {
    // Go straight
    forward();
    delay(500);
  } 
  else if (junctionCount == 2 || junctionCount == 3) {
    // Turn left
    junctionLeft();
    delay(500); // Adjust the delay as needed based on your robot's behavior
  } else if (junctionCount >= 4 && junctionCount <= 7) {
    // Go straight
    forward();
    delay(250);
  } else if (junctionCount == 8 || junctionCount == 9) {
    // Turn left
    junctionLeft();
    delay(500); // Adjust the delay as needed based on your robot's behavior
  } else if (junctionCount == 10) {
    // Reset junctionCount
    junctionCount = 0;
  }
  junctionCount++;
}
