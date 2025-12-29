#include <WiFiS3.h>

// Huskylens library
#include "HUSKYLENS.h"
#include "SoftwareSerial.h"

#include <Wire.h>


//pins
const int LEYE = 4;//left IR sencor
const int REYE = 5;//right IR sencor
const int RSPEED = 10;//Right Motor Speed Control
const int LSPEED = 11;//Left Motor Speed Control
const int IN1 = 8;//Motor Control Right
const int IN2 = 9;//Motor Control Right
const int IN3 = 6;//Motor Control Left
const int IN4 = 7;//Motor Control Left
const int ENCL = 2;//left wheel encoder
const int ENCR = 3;//right wheel encoder

//constants
const int LMAX = 255;//max speed for left motor(for reference)
const int RMAX = 235;//max speed for right motor(for reference)
const double TURNING_SPEED = 0.85;//speed that the motors on the opposite wheel will be when turning
const double TURNING_WHEEL_SPEED = 0; // speed that the motors on the turning side wheel will be turning

// *** ADDED FOR GUI MAP ***
const int TICKS_PER_REV = 4;
const float WHEEL_DIAMETER = 63.0;
const float WHEEL_CIRCUM = 3.14159f * WHEEL_DIAMETER;
const float TRACK_WIDTH = 131.0;


//global veriables
bool Going = false;//true when car is moving
double Speed = 0.6; //percentage speed of the car
WiFiClient client;
int signNum = 0;//stores the number sign that has been seen
bool signDetected = false;
int previousT = 0;


char ssid[] = "Wall-e_Jr";//name of wifi network
char pass[] = "GroupZ06";//password
WiFiServer server(5200);


// Added for calculating speed
volatile long leftEncoderTicks = 0;
volatile long rightEncoderTicks = 0;

// *** ADDED FOR GUI MAP ***

float posX = 0.0f;
float posY = 0.0f;
float heading = 0.0f;

unsigned long lastOdoSendTime = 0;
const unsigned long ODO_SEND_INTERVAL = 500;

// Speed calculation vars
unsigned long previousTime = 0;
double previousDistance = 0.0;
double actualSpeed = 0.0;
double totalDistanceTravelledMm = 0;


void Go_Forward();
void Go_Left();
void Go_Right();
void Stop();
void ClientInteract();
void Junction();

// *** ADDED FOR APRILTAGS ***
void checkAprilTags();


//Huskylens definitions
HUSKYLENS huskylens;   // global huskeylens object
int tag_ID = 0;    // AprilTags counter


void handleLeftEncoder() {
  leftEncoderTicks++;
}

void handleRightEncoder() {
  rightEncoderTicks++;
}





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(9600); 

    Wire.begin(); // start I2C
  
  // Initialize HuskyLens
  while (!huskylens.begin(Wire)) {
    Serial.println("HuskyLens initialization failed. Check connections.");
    delay(1000);
  }
  
  // Set HuskyLens to Tag Recognition mode
  huskylens.writeAlgorithm(ALGORITHM_TAG_RECOGNITION);
  Serial.println("HuskyLens initialized in Tag Recognition mode!");
  
  
  //each pin is set to either an input or output pin, IR sensors give inputs, wheel motors take outputs
  pinMode(LEYE, INPUT_PULLUP);
  pinMode(REYE, INPUT_PULLUP);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(RSPEED, OUTPUT);
  pinMode(LSPEED, OUTPUT);

  pinMode(ENCL, INPUT_PULLUP);
  pinMode(ENCR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCL), handleLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCR), handleRightEncoder, RISING);

  WiFi.beginAP(ssid, pass);
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address:");
  Serial.println(ip);
  server.begin();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void loop() {

  WiFiClient newClient = server.available();//create new client object
  if (newClient && !client) {//checks if there is a new client trying to connect
    client = newClient; //make the sure the client is recorded so it doesnt print new client next loop
    Serial.println("New client connected!");
  } if (client.connected()) {  //if the client is connected, run the rest of the code, if not, do nothing

    
    if (client.available()) { //checks if there is a packet of data available in the buffer from the client
      ClientInteract();
    }
      
    

    if(Going){

         // *** ADDED FOR APRILTAGS ***
        // Check for recognized tags first (each loop)
        checkAprilTags();
      
        
      //go forward if on white line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==HIGH){

          Go_Forward();
       
        }
        
         //turn left is left sensor is off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==HIGH){
          Go_Left();
        }

        //turn right if right sencor is off the line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==LOW){
          Go_Right();
        }

        //stop if both sencors are off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==LOW){
          Junction();
        }

    }
  
      else if (!Going) {
      Stop();
    }

     updateOdometry();
    sendPositionIfNeeded();
}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




//Function Definitions
void Go_Forward(){
  //configure h bridge to direct power to both motors
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(RSPEED, RMAX * Speed);//set speed of motors
  analogWrite(LSPEED, LMAX * Speed);
}

void Go_Right(){
  //configure h bridge to direct power to both motors
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(LSPEED, LMAX * TURNING_WHEEL_SPEED * Speed);//slow down right motor by TURNING_WHEEL_SPEED
  analogWrite(RSPEED, (RMAX * TURNING_SPEED * Speed));//slow down left motor slightly by TURNING_SPEED
}
 


void Go_Left(){
  //configure h bridge to direct power to both motors
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(RSPEED, RMAX * TURNING_WHEEL_SPEED * Speed);//slow down right motor by TURNING_WHEEL_SPEED
  analogWrite(LSPEED, (LMAX * TURNING_SPEED * Speed));//slow down left motor slightly by TURNING_SPEED
}

void Stop(){
  //configure h bridge to stop power to both motors
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  }

void ClientInteract() {
    String data = client.readStringUntil('\n');//reads transmitted string from the gui
      if (data == "STOP") {
        Going = false; //exit normal running of buggy
        Serial.println("Stopping");
      }
      else if(data == "GO") {
        Going = true; // re-enter normal running
        Serial.println("Going");
      }      
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////
// +++ ADDED FOR APRILTAGS +++
// checkAprilTags() function handles 5 IDs: Stop, Slow, Fast, Turn Left, Turn Right
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void checkAprilTags() {
  // Minimum width in pixels to accept a tag as "close"
  const int minTagWidth = 12;
  int currentT = millis();
  int deltaT = currentT - previousT;
  if (deltaT >= 600) {
  // Ask HuskyLens for a new frame and check for recognized data
  if (huskylens.request() && huskylens.isLearned() && huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();
    int tagID = result.ID;
    int tagWidth = result.width; // perceived size

    Serial.print("Detected Tag ID: ");
    Serial.print(tagID);
    Serial.print(" with width: ");
    Serial.println(tagWidth);

    // Only process tag if it's close enough
    if (tagWidth >= minTagWidth) {
      switch (tagID) {

        case 1: 
          // Tag #1 => STOP
          Going = false; 
          Stop();
          Serial.println("AprilTag #1 => STOP");
           if (client && client.connected()) client.println("TAG:1");
          break;

        case 2:
          // Tag #2 => Slow Speed
          Going = true;
          Speed = 0.55;  // e.g. 30% speed
          Serial.println("AprilTag #2 => Slow speed (0.55)");
           if (client && client.connected()) client.println("TAG:2");
          break;

        case 3:
          // Tag #3 => Fast Speed
          Going = true;
          Speed = 0.65;  // e.g. 70% speed
          Serial.println("AprilTag #3 => Fast speed (0.7)");
           if (client && client.connected()) client.println("TAG:3");
          break;

        case 4:
          // Tag #4 => Turn Left pivot
          Going = true;
          Serial.println("AprilTag #4 => Turn Left pivot");
          signNum = -1;
           if (client && client.connected()) client.println("TAG:4");
          break;

        case 5:
          // Tag #5 => Turn Right pivot
          Going = true;
          Serial.println("AprilTag #5 => Turn Right pivot");
          signNum = 1;
           if (client && client.connected()) client.println("TAG:5");
          break;

        default:
          // Unrecognized tag
          Serial.println("Unknown tag ID");
          break;
      }
    } else {
      Serial.println("Tag ignored (too small / far away).");
    }

    tag_ID = 0; // Reset after processing
  }
  previousT = currentT;
}
}


void Junction(){
  //Speed = 0.55;

  if(signNum== 1){
      //analogWrite(RSPEED, RMAX * 0.25 * Speed);//slow down right motor by TURNING_WHEEL_SPEED
      //nalogWrite(LSPEED, (LMAX * TURNING_SPEED * Speed));//slow down left motor slightly by TURNING_SPEED
      //delay(300);
      int start = millis();
      int recent = start;
      int deltaT = 0;
      while (deltaT <= 3000) {
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==HIGH){
          ////find line function
          Go_Forward();
        }
        
         //turn left is left sensor is off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==HIGH){
          Go_Left();
        }

        //turn right if right sencor is off the line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==LOW){
          Go_Right();
        }
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==LOW){
          Go_Left();
          delay(80);
          Go_Forward();
        }
        recent = millis();
        deltaT = recent - start;
      }
      signNum = 0;
  }
  else if(signNum == -1){
      //analogWrite(LSPEED, LMAX * 0.25 * Speed);//slow down right motor by TURNING_WHEEL_SPEED
      //analogWrite(RSPEED, (RMAX * TURNING_SPEED * Speed));//slow down left motor slightly by TURNING_SPEED
      //delay(300);
      int start = millis();
      int recent = start;
      int deltaT = 0;
      while (deltaT <= 3000) {
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==HIGH){
          ////find line function
          Go_Forward();
        }
        
         //turn left is left sensor is off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==HIGH){
          Go_Left();
        }

        //turn right if right sencor is off the line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==LOW){
          Go_Right();
        }
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==LOW){
          Go_Right();
          delay(80);
          Go_Forward();
        }
        recent = millis();
        deltaT = recent - start;
        
      }
      signNum = 0;
  }
  else {
      Go_Forward();   
  }
  Speed = 0.6;
  
 }

 void updateOdometry() {
  static long prevLeftTicks = 0;
  static long prevRightTicks = 0;

  long currentLeftTicks = leftEncoderTicks;
  long currentRightTicks = rightEncoderTicks;

  long deltaLeft = currentLeftTicks - prevLeftTicks;
  long deltaRight = currentRightTicks - prevRightTicks;

  prevLeftTicks = currentLeftTicks;
  prevRightTicks = currentRightTicks;

  if (deltaLeft == 0 && deltaRight == 0) return;
  
  // convert ticks to linear diatance in mm
  float distLeft = (float)deltaLeft / TICKS_PER_REV * WHEEL_CIRCUM;
  float distRight = (float)deltaRight / TICKS_PER_REV * WHEEL_CIRCUM;

  // avg forward movement of the buggy
  float distAvg = (distLeft + distRight) * 0.5f;

  totalDistanceTravelledMm += distAvg;

  // estimate the change in orientation
  // turning angle
  float deltaTheta = (distRight - distLeft) / TRACK_WIDTH;
  heading += deltaTheta;
  if (heading > 3.14159f) heading -= 2.0f * 3.14159f;
  if (heading < -3.14159f) heading += 2.0f * 3.14159f;

  if (fabs(deltaTheta) < 1e-6) {
    posX += distAvg * cos(heading);
    posY += distAvg * sin(heading);
  } 
  else {
    float r = distAvg / deltaTheta;

    //intermediate heading angle, representing the buggy’s average orientation during a turn. It improves the accuracy of your (posX, posY) calculation while turning.
    float headingMid = heading - (deltaTheta * 0.5f);
    posX += r * (-sin(headingMid) + sin(headingMid + deltaTheta));
    posY += r * (cos(headingMid) - cos(headingMid + deltaTheta));
  }
}


double CalculateSpeed() {
  double currentTime = millis();
  double deltaT = currentTime - previousTime;
  if (deltaT < 100) return actualSpeed;

  double deltaD = totalDistanceTravelledMm - previousDistance;
  actualSpeed = (deltaD / deltaT); // mm/ms = m/s

  previousTime = currentTime;
  previousDistance = totalDistanceTravelledMm;

  return actualSpeed;
}

void sendPositionIfNeeded() {
  unsigned long now = millis();
  if (now - lastOdoSendTime >= ODO_SEND_INTERVAL) {
    String message;

    if (Going) {
      updateOdometry();
      float speedMs = CalculateSpeed();
      float headingDeg = heading * 180.0f / 3.14159f;

      message += "DATA:";
      message += String(speedMs, 2);
      message += ",";
      message += String(posX, 2);
      message += ",";
      message += String(posY, 2);
      message += ",";
      message += String(headingDeg, 1);
    } else {
      message = "SPEED:0.00";
    }

    if (client && client.connected()) {
      client.println(message);
    }

    lastOdoSendTime = now;
  }
}