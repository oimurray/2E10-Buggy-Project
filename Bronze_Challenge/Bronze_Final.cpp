/*#include <Adafruit_GFX.h>
#include <Arduino_LED_Matrix.h>
#include <gallery.h>*/


#include <WiFiS3.h>

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
const int LMAX = 245;//max speed for left motor(for reference)
const int RMAX = 255;//max speed for right motor(for reference)
const double TURNING_SPEED = 0.92;//speed that the motors on the opposite wheel will be when turning
const double TURNING_WHEEL_SPEED = 0; // speed that the motors on the turning side wheel will be turning
const double FOLLOW_DISTANCE = 15;//distance at which the car will try to follow
const double STOPPING_DISTANCE = 15;//distance at which the car will stop form an object
#define PI 3.141592653589793

//global veriables
bool Going = false;//true when car is moving
bool Following = false;//true when car is following an object
double Speed = 0.6; //percentage speed of the car
WiFiClient client;
long int distanceTravelled = 0;
volatile long int distanceTravelledR = 0;
volatile long int distanceTravelledL = 0;
volatile int counter = 0;
//ArduinoLEDMatrix matrix;
double distance = 0.0;
int LoopCount = 0;


char ssid[] = "Wall-e_Jr";//name of wifi network
char pass[] = "2E10Project";//password
WiFiServer server(5200);

void CountPulseR();
void CountPulseL();//interupt function to run when the encoders detect a pulse, update distance travelled by wheel
void Go_Forward();
void Go_Left();
void Go_Right();
void Stop();
void Change_Speed(double Percentage);//changes speed by a number
void Set_Speed(double Percentage);//sets speed to a number
int Detect_Distance();
//void DisplayLED();

void setup() {
  Serial.begin(9600);
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

  attachInterrupt(digitalPinToInterrupt(ENCR), CountPulseR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCL), CountPulseL, RISING);

  Serial.begin(9600);
  WiFi.beginAP(ssid, pass);
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address:");
  Serial.println(ip);
  server.begin();
  //matrix.begin();
}




void loop() {

  WiFiClient newClient = server.available();//create new client object
  if (newClient && !client) {//checks if there is a new client trying to connect
    client = newClient; //make the sure the client is recorded so it doesnt print new client next loop
    Serial.println("New client connected!");
  }
  
  if (client.connected()) {  //if the client is connected, run the rest of the code, if not, do nothing
    //Serial.println("loop");
    

    if (client.available()) { //checks if there is a packet of data available in the buffer from the client
      ClientInteract();
    }

    

    if(Going && !Following){///////////////////////////////////////////////////////Line Following Loop

      DistanceTravelled();
      
      Serial.println("RUNNING");
      //detect distance and stop if less then 10cm
      
      if((LoopCount % 5)==0){
        if(Detect_Distance() < STOPPING_DISTANCE){
        Stop();
        Serial.println("OBSTICAL!!!");
        client.println("OBSTICAL!!!");
        //Serial.println(Detect_Distance());
        delay(3000);
        }
      }
      else{
      //go forward if on white line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==HIGH){

          Go_Forward();
          //Serial.println("GOING FORWARD");
        }

        //turn left is left sensor is off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==HIGH){
          Go_Left();
          //Serial.println("GOING LEFT");
        }

        //turn right if right sencor is off the line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==LOW){
          Go_Right();
          //Serial.println("GOING RIGHT");
        }

        //stop if both sencors are off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==LOW){
          Stop();
          //Serial.println("STOPPING");
        }
      }

      LoopCount++;
    }


    
  //Follow Loop
  if(Going && Following){

    double Proximity;
    Proximity = Detect_Distance();

    if(Proximity > FOLLOW_DISTANCE){
      Change_Speed(0.05);////////////////////////////////////change this value
    }

    if(Proximity < FOLLOW_DISTANCE){
      Change_Speed(-0.05); /////////////////////////////////////////////change this value
    }

    Serial.println("FOLLOWING");
      //detect distance and stop if less then 10cm
      
      if(Proximity < STOPPING_DISTANCE/2){
        Stop();
        Serial.println("OBSTICAL!!!");
        client.println("OBSTICAL!!!");
        //Serial.println(Detect_Distance());
      }
      else{
      //go forward if on white line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==HIGH){

          Go_Forward();
          //Serial.println("GOING FORWARD");
        }

        //turn left is left sensor is off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==HIGH){
          Go_Left();
          //Serial.println("GOING LEFT");
        }

        //turn right if right sencor is off the line
        if(digitalRead(REYE)==HIGH && digitalRead(LEYE)==LOW){
          Go_Right();
          //Serial.println("GOING RIGHT");
        }

        //stop if both sencors are off the line
        if(digitalRead(REYE)==LOW && digitalRead(LEYE)==LOW){
          Stop();
          //Serial.println("STOPPING");
        }
      }








  }
    else if (!Going) {
      Stop();
    }
  }
}






//Function Definitions
void Go_Forward(){
  //configure h bridge to direct power to both motors
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(10);
  digitalWrite(IN2, LOW);

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

int Detect_Distance(){

  int duration;
  int distance;
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  //send pulse
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // recieve pulse
  duration = pulseIn(ECHO, HIGH);

  // Calculate the distance(0.034 being the speed of sound)
  distance = duration * 0.034 / 2;

  // Print the distance to the Serial Monitor
  //Serial.println(distance);

 return distance;
 
}

void DistanceTravelled() {
    if (counter > 20) {//transmit/print every 50 cm 
      distanceTravelled = (distanceTravelledR + distanceTravelledL)/2;//average distance between both wheels
      client.println(distanceTravelled);//transmit distance as a string to client
      Serial.println(distanceTravelled);//print distance to serial monitor
      counter = 0;
    } 
}


void ClientInteract() {
    String data = client.readStringUntil('\n');//reads transmitted string from the gui
      if (data == "STOP") {
        Going = false; //exit normal running of buggy
        Serial.println("Stopping");
      }
      if (data == "GO") {
        Going = true; // re-enter normal running
        Serial.println("Going");
      }
}

/*void DisplayLED (){
  if (distanceTravelled % 20 == 0) {//print distance every 20 cm
    double LEDdistance = distanceTravelled/100;
    matrix.clear(); // Clear the matrix
    matrix.print(LEDdistance, 1); // Display distance with 1 decimal place
    matrix.show(); // Update the matrix
  }
}*/

void CountPulseR() {
  distanceTravelledR += 5;//quarter of wheel circumfrence added
  counter++;
}
void CountPulseL() {
  distanceTravelledL += 5;//quarter of wheel circumfrence added
  counter++;
}


void Change_Speed(double Percentage){
  
  Speed += Percentage;
  
  }

  
void Set_Speed(double Percentage){

  Speed = Percentage;

}