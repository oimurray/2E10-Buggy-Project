#include <WiFiS3.h>
#include <PIDController.h>

/////////////////////////////////////
//-calabrate pids
//-need to make the speed slider for the GUI transmit a value as a double or convert the value in arduino
//-make the speed slider update when the buggy is following based on the current speed value in arduino
//-could use a try catch statment to try converting a transmitted string to a double. if it works update speed, if it doesnt update follow/go
/////////////////////////////////////

//pins
const int LEYE = 4;//left IR sencor
const int REYE = 5;//right IR sencor
const int RSPEED = 10;//Right Motor Speed Control
const int LSPEED = 11;//Left Motor Speed Control
const int IN1 = 8;//Motor Control Right
const int IN2 = 9;//Motor Control Right
const int IN3 = 6;//Motor Control Left
const int IN4 = 7;//Motor Control Left
const int ECHO = 1;//ultrasonic reciever
const int TRIG = 0;//untrasonic emitter
const int ENCL = 2;//left wheel encoder
const int ENCR = 3;//right wheel encoder


//constants
const int LMAX = 255;//max speed for left motor(for reference)
const int RMAX = 243;//max speed for right motor(for reference)
const double TURNING_SPEED = 0.7;//speed that the motors on the opposite wheel will be when turning
const double TURNING_WHEEL_SPEED = 0; // speed that the motors on the turning side wheel will be turning
const double FOLLOW_DISTANCE = 15;//distance at which the car will try to follow
const double STOPPING_DISTANCE = 10;//distance at which the car will stop form an object
float MAXSPEED = 0.60;///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define PI 3.141592653589793
#define MOVING_AVERAGE_WINDOW 10

//global veriables
bool Going = false;//true when car is moving
bool Following = false;//true when car is following an object
double Speed = 0.6; //percentage speed of the car
WiFiClient client;
long int distanceTravelled = 0;
volatile long int distanceTravelledR = 0;
volatile long int distanceTravelledL = 0;
volatile int counter = 0;
double distance = 0.0;
int LoopCount = 0;
PIDController pidFollow;
PIDController pidSpeed;
double FKp = 1.0, FKi = 0.1, FKd = 0.5;
double SKp = 100, SKi = 0, SKd = 0.75;

unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long previousDistance = 0;
double refSpeed = 0;
double actualSpeed;
double avgSpeed = 0;
double speedBuffer[MOVING_AVERAGE_WINDOW] = {0};
int speedBufferIndex = 0;


char ssid[] = "Wall-e_Jr";//name of wifi network
char pass[] = "GroupZ06";//password
WiFiServer server(5200);


void CountPulseR();
void CountPulseL();//interupt function to run when the encoders detect a pulse, update distance travelled by wheel
void Go_Forward();
void Go_Left();
void Go_Right();
void Stop();
int Detect_Distance();
void SpeedPID ();
double CalculateSpeed();





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(9600); 
  pidFollow.begin();// initialize the PID instance
  pidSpeed.begin();
  
  //each pin is set to either an input or output pin, IR sensors give inputs, wheel motors take outputs
  pinMode(LEYE, INPUT_PULLUP);
  pinMode(REYE, INPUT_PULLUP);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(RSPEED, OUTPUT);
  pinMode(LSPEED, OUTPUT);
  pinMode(ECHO, INPUT_PULLUP);
  pinMode(TRIG, OUTPUT);
  pinMode(ENCR, INPUT_PULLUP);
  pinMode(ENCL, INPUT_PULLUP);

 
  pidFollow.setpoint(FOLLOW_DISTANCE);// The "goal" the PID controller tries to "reach"
  pidFollow.tune(FKp, FKi, FKd);// Tune the PID, arguments: kP, kI, kD
  pidFollow.limit(0, 1);

  pidSpeed.setpoint(refSpeed);
  pidSpeed.tune(SKp, SKi, SKd);
  pidSpeed.limit(0, 1);


  attachInterrupt(digitalPinToInterrupt(ENCR), CountPulseR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCL), CountPulseL, RISING);
  
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

  currentTime = millis();

  WiFiClient newClient = server.available();//create new client object
  if (newClient && !client) {//checks if there is a new client trying to connect
    client = newClient; //make the sure the client is recorded so it doesnt print new client next loop
    Serial.println("New client connected!");
  }
  
  if (client.connected()) {  //if the client is connected, run the rest of the code, if not, do nothing
    
    if (client.available()) { //checks if there is a packet of data available in the buffer from the client
      ClientInteract();
    }

    

    if(Going){

      DistanceTravelled();//transmit traveled distance
      
      
      if(!Following && (LoopCount % 5)==0){
        if(Detect_Distance() < STOPPING_DISTANCE){
        Stop();
        Serial.println("OBSTICAL!!!");
        client.println("OBSTICAL!!!");
        delay(3000);
        }
      } 

      if(!Following && ((LoopCount % 5) == 0)){
        SpeedPID();
        Serial.print("refSpeed:");
        Serial.print(refSpeed);
        Serial.print(",actualSpeed:");
        Serial.println(actualSpeed);
        client.println('S' + String(actualSpeed));
      }
        
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
          Stop();
        }
      

      if(Following){
        double actDistance = Detect_Distance();
        Speed = 1 - pidFollow.compute(actDistance);
        constrain(Speed, 0, 0.4);
        String objDistance = ('D' + String(actDistance));
        client.println(objDistance);
      }

      LoopCount++;

    }
  
      else if (!Going) {
      Stop();
    }

    
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
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  analogWrite(RSPEED, (RMAX * TURNING_SPEED * Speed));//slow down right motor slightly by TURNING_SPEED
  analogWrite(LSPEED, LMAX * TURNING_WHEEL_SPEED * Speed);//slow down left motor by TURNING_WHEEL_SPEED
}

void Go_Left(){
  //configure h bridge to direct power to both motors
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
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

int Detect_Distance() {
  int duration;
  int distance;
  unsigned long startTime = millis(); // Record the start time
  unsigned long timeLimit = 4;       // Timeout limit in milliseconds

  // Ensure the trigger pin is low
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  // Send a pulse
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Receive pulse with timeout
  duration = pulseIn(ECHO, HIGH, timeLimit * 1000); // Convert timeLimit to microseconds

  // Check if the pulseIn function timed out
  if (duration == 0) {
    //Serial.println("Timeout: Unable to detect distance within the time limit.");
    return FOLLOW_DISTANCE;  // Return follow distance to mean PID will do nothing when timeout occurs
  }

  // Calculate the distance (0.034 being the speed of sound)
  distance = duration * 0.034 / 2;
  return distance;
}



void DistanceTravelled() {
    distanceTravelled = (distanceTravelledR + distanceTravelledL)/2;//average distance between both wheels
     
    if (counter > 20) {//transmit/print every 50 cm
      String SDistance = ('T' + String(distanceTravelled));
      client.println(SDistance);//transmit distance as a string to client
      //Serial.println(distanceTravelled);//print distance to serial monitor
      counter = 0;
    }
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
      else if(data == "FOLLOW") {
        Following = false; //start following an object at constant distance
        Serial.println("Following");
      }
      else if(data == "STOP FOLLOW") {
        Following = true; // stop following operations
        Serial.println("Stop Following");
      }
      else {
        float speedValue = data.toFloat(); // Convert once

        if (speedValue > 0.0 && speedValue <= 1.0) {
            refSpeed = speedValue;
            Serial.println(refSpeed);
        }  
        else {
            Serial.println("INVALID SPEED SETPOINT");
        }
    }
}


void CountPulseR() {
  distanceTravelledR += 5;//quarter of wheel circumfrence added
  counter++;
}
void CountPulseL() {
  distanceTravelledL += 5;//quarter of wheel circumfrence added
  counter++;
}


void SpeedPID() {
  pidSpeed.setpoint(refSpeed);
  actualSpeed = CalculateSpeed();
  avgSpeed = AverageSpeed(actualSpeed);
  double output = pidSpeed.compute(avgSpeed);
  Speed = (constrain(Speed + output, 0, 1));
  Serial.println(Speed);
}

double CalculateSpeed() {
  double currentTime = millis();
  double deltaT = currentTime - previousTime;
  if (deltaT < 100) {
    return actualSpeed;
  }
  double deltaD = distanceTravelled - previousDistance;
  actualSpeed =  10 * (deltaD/deltaT);
  previousTime = currentTime;
  previousDistance = distanceTravelled;
  return actualSpeed;
}

void AddSpeedToBuffer(double newSpeed) {
  speedBuffer[speedBufferIndex] = newSpeed;
  speedBufferIndex = (speedBufferIndex + 1) % MOVING_AVERAGE_WINDOW;
}

double AverageSpeed(double newSpeed) {
  AddSpeedToBuffer(newSpeed);
  double sum = 0;
  for (int i = 0; i < MOVING_AVERAGE_WINDOW; i++) {
    sum += speedBuffer[i];
  }
  return sum / MOVING_AVERAGE_WINDOW;
}