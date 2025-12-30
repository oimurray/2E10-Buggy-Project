import controlP5.*;
import processing.net.*;

Client myClient;
String data;

ControlP5 p5;

//Global Veriables
boolean Stopped = true;
boolean Following = false;
float referenceSpeed;
float maxSpeed = 0.6;/////////////////////////tune this


int LastSecond;//veriable for the clock
int Time = 0;

//Telemetory Veriables
double distanceTraversed = 0;
float CurrentSpeed = 0;
float obsticalDistance = 0;



Button GoButton;
Button FollowButton;
Slider SpeedSlider;



void setup(){
 
  size(500, 800);  
  background(32);
 
  //setup for wifi
   myClient = new Client(this, "192.168.4.1", 5200); // Connect to Arduino AP

    if (myClient.active()) {
        println("Connected to Arduino!");
    }
    else {
        println("Failed to connect.");
    }
 
 
  //telemetory text and seperations
  stroke(255, 255, 255);
  strokeWeight(5);
  line(0, 80, 500, 80);
  line(183, 0, 183, 80);
  line(326, 0, 326, 80);
  fill(32);
  strokeWeight(3);
  rect(60, 300, 130, 50);
  rect(300, 300, 130, 50);
  fill(255);
  textSize(16);
  strokeWeight(2);
  line(0, 20, 500, 20);
  text("Speed", 70, 15);
  text("Time", 235, 15);
  text("Distance Traversed", 350, 15);
  LastSecond = second();
  line(60, 315, 190, 315);
  line(300, 315, 430, 315);
  text("Reference Speed", 70, 315);
  text("Obstical Distance", 310, 315);
 
 
  //Go buttons
  p5 = new ControlP5(this);
  GoButton = p5.addButton("Go");
  GoButton.setPosition(60, 650);
  GoButton.setSize(150, 50);
  GoButton.setOff();
 
 
  //Follow Button
  FollowButton = p5.addButton("Follow");
  FollowButton.setPosition(300, 650);
  FollowButton.setSize(150, 50);
 
 
  //Speed Slider
 SpeedSlider = p5.addSlider("Speed");
 SpeedSlider.setPosition(230, 180);
 SpeedSlider.setSize(40, 400);
 SpeedSlider.setRange(0.00001, maxSpeed);
 SpeedSlider.setValue(0);
 SpeedSlider.setLabelVisible(false);
 
 
}



void draw(){

 
  if(myClient.available() > 0){
    String data = myClient.readStringUntil('\n');
    println(data);
    if (data != null) {
      try {
        if(data.charAt(0) == 'T'){
            data = data.substring(1);
            double disTraversed = Double.parseDouble(data);// Attempt to convert to double
            Update_Distance(disTraversed);//print the distance to the UI
            Update_Speed();//Update the speed displayed
        }
        else if(data.charAt(0) == 'D'){
          data = data.substring(1);
          obsticalDistance = Float.parseFloat(data);
          Update_ObDistance();
          
        }
        else if(data.charAt(0) == 'S'){
          data = data.substring(1);
          CurrentSpeed = Float.parseFloat(data);
          Update_Speed();
        
        }
        }
        catch (NumberFormatException e) {
              println("WARNING: OBSTACLE DETECTED!");// If it fails, it must be a string therefore it is the obstacle detection
        }
    }
  }
 
  //Go Button Controls
if(GoButton.isPressed()){
  Update_Go_Button();
}
 
 
  //Follow Button Controls
if(FollowButton.isPressed()){
  Update_Follow_Button();
}


  // Code for the slider transmition
  if(referenceSpeed != SpeedSlider.getValue() && !Following){
  Update_Slider();
  }
 
 
  //Code for the clock
  if(LastSecond != second()){
    Update_Clock();
  }
 

 
}










///////////////////
/////Functions/////
///////////////////
void Update_Go_Button(){
 
  Stopped = !Stopped;

 
  if(Stopped){
    println("Stopped");
    myClient.write("STOP\n");//Transmit Stop
    GoButton.setCaptionLabel("Go");
  }
 
  if(!Stopped){
   println("Going");
   myClient.write("GO\n");//Stansmits Go
   GoButton.setCaptionLabel("Stop");
  }
 
  delay(250);
 
}


void Update_Follow_Button(){

Following = !Following;

 
  if(Following){
    println("Following");
    myClient.write("STOP FOLLOW\n");//Transmit STOP FOLLOW
    FollowButton.setCaptionLabel("Stop Follow");
  }
 
  if(!Following){
   println("Not Following");
   myClient.write("FOLLOW\n");//Stansmits fOLLOW
   FollowButton.setCaptionLabel("Follow");
  }
 
  delay(250);

}


void Update_Clock(){
 
  //clear clock
    stroke(32);
    fill(32);
    rect(190, 24, 130, 50);
   
    LastSecond = second();
    Time++;
   
    textSize(28);
    fill(255);
    stroke(255);
    String STime = "t + " + nf(Time, 3);
    text(STime, 255 - textWidth(STime) / 2, 60);

}

void Update_Distance(double disTraversed){
 
  stroke(32);
  fill(32);
  rect(335, 24, 160, 50);
 
  textSize(28);
  fill(255);
  stroke(255);


  String SDistance = disTraversed / 100 + "m";//distance in m is distance cm / 100
  text(SDistance, 400 - textWidth(SDistance) / 2, 60);//draw the text in the box labelled distance
  println(SDistance);//print distance to console
}

void Update_Speed(){
 
  stroke(32);
  fill(32);
  rect(5, 24, 170, 50);
 
  textSize(28);
  fill(255);
  stroke(255);
 
  String SSpeed = nf(CurrentSpeed, 0, 2) + "m/s";
  text(SSpeed, 100 - textWidth(SSpeed) / 2, 60);
  println(SSpeed);
 
}





void Update_Slider(){

  referenceSpeed = SpeedSlider.getValue();
  String SRefSpeed = nf(referenceSpeed, 0, 2) + "\n";
  myClient.write(SRefSpeed);//tansmits speed
  println(SRefSpeed);
  
  stroke(32);
  fill(32);
  rect(75, 320, 100, 25);
 
  textSize(28);
  fill(255);
  stroke(255);
 
  String refSpeed = nf(referenceSpeed, 0, 2) + "m/s";
  text(refSpeed, 125 - textWidth(refSpeed) / 2, 340);


}


void Update_ObDistance(){

  stroke(32);
  fill(32);
  rect(315, 320, 100, 25);
 
  textSize(28);
  fill(255);
  stroke(255);
  
  String SDistance = nf(obsticalDistance, 0, 2) + "m";
  text(SDistance, 360 - textWidth(SDistance) / 2, 340);
  

}