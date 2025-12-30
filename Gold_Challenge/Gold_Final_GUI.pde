import processing.net.*;
import java.util.ArrayList;

PImage stopSign, slowSign, fastSign, leftSign, rightSign;
PImage driverNeutral, driverFast, driverSlow, driverStop, driverLeft, driverRight;
PImage currentSign, currentDriver;
PImage backgroundImg;

Client myClient;
String[] log = new String[6];
boolean isGoing = false;
float currentSpeed = 0.0;
String lastSpeedCommand = "NORMAL";  // Still used for the driver avatar, but not for color.

ArrayList<PVector> positions = new ArrayList<PVector>();

// === Odometry plot area constants ===
float mapX = 700;
float mapY = 350;
float mapW = 350;
float mapH = 280;

void setup() {
  size(1100, 650);
  textFont(createFont("Arial", 16));

  myClient = new Client(this, "192.168.4.1", 5200);

  for (int i = 0; i < log.length; i++) {
    log[i] = "";
  }

  backgroundImg = loadImage("bg_f1.jpg");
  backgroundImg.resize(width, height);

  stopSign     = loadImage("stop.png");
  slowSign     = loadImage("slow.png");
  fastSign     = loadImage("fast.png");
  leftSign     = loadImage("left.png");
  rightSign    = loadImage("right.png");

  driverNeutral = loadImage("driver_neutral.png");
  driverFast    = loadImage("driver_fast.png");
  driverSlow    = loadImage("driver_slow.png");
  driverStop    = loadImage("driver_stop.png");
  driverLeft    = loadImage("driver_left.png");
  driverRight   = loadImage("driver_right.png");

  currentSign   = null;
  currentDriver = driverNeutral;
}

void draw() {
  image(backgroundImg, 0, 0);

  // Dark overlay
  fill(0, 200);
  noStroke();
  rect(0, 0, width, height);

  // === Header ===
  fill(255, 0, 0);
  rect(0, 0, width, 60);
  fill(255);
  textAlign(CENTER);
  textSize(32);
  text("Wall-E Jr. F1 Dashboard", width / 2, 40);

  // === Signs + Driver Display (Aligned on same row) ===
  textAlign(CENTER);
  textSize(20);
  fill(255);

  // Detected Sign on the left
  text("Last Detected Sign", 160, 130);
  if (currentSign != null) {
    image(currentSign, 90, 150, 140, 140);
  }

  // Driver Status on the right
  text("Driver Status", width - 160, 130);
  if (currentDriver != null) {
    image(currentDriver, width - 225, 150, 150, 170);
  }

  // === Current Speed (center) ===
  fill(255);
  textSize(22);
  text("Current Speed", width / 2, 130);

  // ---------------------------------------------
  // SPEED COLOR BASED ON currentSpeed VALUE
  // ---------------------------------------------
  float sp = currentSpeed;  // m/s
  color speedColor;
  if      (sp < 0.2)   speedColor = color(255, 165, 0);  // Orange(slow)
  else if (sp < 0.3)   speedColor = color(0,   255, 0);   // Green (medium)
  else                 speedColor = color(255,   0, 0);   // Red (fast)

  // Display the speed text
  fill(speedColor);
  textSize(30);
  text(nf(currentSpeed, 1, 2) + " m/s", width / 2, 170);

  // Speed bar
  float speedBarMax = 5;    // Arbitrary top speed for bar mapping
  float speedBarWidth = map(currentSpeed, 0, speedBarMax, 0, 300);
  noStroke();
  fill(100);
  rect(width / 2 - 150, 190, 300, 10);
  fill(speedColor);
  rect(width / 2 - 150, 190, speedBarWidth, 10);

  // === GO/STOP Button ===
  fill(isGoing ? color(220, 0, 0) : color(0, 180, 0));
  stroke(255);
  strokeWeight(2);
  rect(width / 2 - 70, 240, 140, 60, 20);
  fill(255);
  textSize(24);
  textAlign(CENTER);
  text(isGoing ? "STOP" : "GO", width / 2, 275);

  // === Pit Log ===
  fill(255);
  textAlign(LEFT);
  textSize(18);
  text("Pit Log:", 50, 350);
  for (int i = 0; i < log.length; i++) {
    text(log[i], 50, 365 + i * 22);
  }

  // === Process Incoming Data ===
  if (myClient.available() > 0) {
    String data = myClient.readStringUntil('\n');
    if (data != null) {
      data = data.trim();
      processData(data);
    }
  }

  // === Draw Odometry Plot ===
  drawOdometryPlot();
}

// ---------------------------------------------------------------
void mousePressed() {
  // Check if GO/STOP button is clicked
  if (mouseX > width / 2 - 70 && mouseX < width / 2 + 70 && mouseY > 240 && mouseY < 300) {
    isGoing = !isGoing;
    myClient.write((isGoing ? "GO" : "STOP") + "\n");
    addToLog(isGoing ? "Command Sent: GO" : "Command Sent: STOP");
  }
  // Check if the user clicks inside the odometry box => Reset the path
  else if (mouseX > mapX && mouseX < mapX + mapW && mouseY > mapY && mouseY < mapY + mapH) {
    positions.clear();
    addToLog("Odometry path cleared");
  }
}

// ---------------------------------------------------------------
void processData(String data) {
  data = data.trim();
  
  if (data.startsWith("TAG:")) {
    String tag = data.substring(4);
    switch(tag) {
      case "1":
        currentSign = stopSign;
        currentDriver = driverStop;
        lastSpeedCommand = "SLOW"; // Still updates the driver icon, e.g. 'STOP' icon
        addToLog("Tag #1: STOP");
        break;
      case "2":
        currentSign = slowSign;
        currentDriver = driverSlow;
        lastSpeedCommand = "SLOW";
        addToLog("Tag #2: SLOW");
        break;
      case "3":
        currentSign = fastSign;
        currentDriver = driverFast;
        lastSpeedCommand = "FAST";
        addToLog("Tag #3: FAST");
        break;
      case "4":
        currentSign = leftSign;
        currentDriver = driverLeft;
        lastSpeedCommand = "NORMAL";
        addToLog("Tag #4: LEFT");
        break;
      case "5":
        currentSign = rightSign;
        currentDriver = driverRight;
        lastSpeedCommand = "NORMAL";
        addToLog("Tag #5: RIGHT");
        break;
      default:
        currentSign = null;
        currentDriver = driverNeutral;
        lastSpeedCommand = "NORMAL";
        addToLog("Unknown tag detected");
    }
  }
  else if (data.startsWith("DATA:")) {
    try {
      // Parse the consolidated data string
      // Format is "DATA:speed,posX,posY,heading"
      String[] values = split(data.substring(5), ',');
      if (values.length == 4) {
        // Extract speed
        currentSpeed = float(values[0]);
        
        // Extract position and heading
        float xMm = float(values[1]);
        float yMm = float(values[2]);
        float headingD = float(values[3]);
        
        // Add to positions list for plotting
        positions.add(new PVector(xMm, yMm));
      }
    } catch (Exception e) {
      println("Data parsing error: " + e);
    }
  }
  else if (data.startsWith("SPEED:")) {
    // Backward compatibility for the older data format
    try {
      currentSpeed = float(data.substring(6));
    } catch (Exception e) {
      println("Speed parsing error: " + e);
    }
  }
  else if (data.startsWith("POS:")) {
    // Backward compatibility for the older data format
    String coords = data.substring(4);
    String[] parts = split(coords, ',');
    if (parts.length == 3) {
      float xMm = float(parts[0]);
      float yMm = float(parts[1]);
      float headingD = float(parts[2]);
      positions.add(new PVector(xMm, yMm));
    }
  }
}

// ---------------------------------------------------------------
// Dynamically scales all points so they fit within mapW x mapH.
void drawOdometryPlot() {
  pushMatrix();
  pushStyle();

  // Draw bounding rectangle
  fill(255, 50);
  stroke(255);
  rect(mapX, mapY, mapW, mapH);

  // If no positions, just return after drawing empty box
  if (positions.size() == 0) {
    popStyle();
    popMatrix();
    return;
  }

  // Find bounding box in the data domain
  float minX = Float.MAX_VALUE;
  float maxX = Float.MIN_VALUE;
  float minY = Float.MAX_VALUE;
  float maxY = Float.MIN_VALUE;
  for (PVector pos : positions) {
    if (pos.x < minX) minX = pos.x;
    if (pos.x > maxX) maxX = pos.x;
    if (pos.y < minY) minY = pos.y;
    if (pos.y > maxY) maxY = pos.y;
  }

  // If we only have one point or they are all the same => avoid divide by zero
  float rangeX = maxX - minX;
  float rangeY = maxY - minY;
  if (rangeX < 0.0001) rangeX = 1;
  if (rangeY < 0.0001) rangeY = 1;

  // We apply a small margin (0.9) so it doesn't touch edges
  float margin = 0.9;
  float scaleFactor = margin * min(mapW / rangeX, mapH / rangeY);

  // The midpoint in data domain
  float centerX = (minX + maxX) / 2.0;
  float centerY = (minY + maxY) / 2.0;

  // Translate to the center of that rectangle
  translate(mapX + mapW / 2, mapY + mapH / 2);

  // Draw the center indicator (represents data bounding box center)
  fill(255, 0, 0);
  noStroke();
  ellipse(0, 0, 5, 5);

  // Draw the path
  stroke(0, 255, 0);
  noFill();
  beginShape();
  for (PVector pos : positions) {
    float px = (pos.x - centerX) * scaleFactor;
    float py = -(pos.y - centerY) * scaleFactor; // invert y
    vertex(px, py);
  }
  endShape();

  // Draw last position
  PVector lastPos = positions.get(positions.size() - 1);
  float px = (lastPos.x - centerX) * scaleFactor;
  float py = -(lastPos.y - centerY) * scaleFactor;
  fill(0, 255, 255);
  noStroke();
  ellipse(px, py, 6, 6);

  popStyle();
  popMatrix();`
}

// ---------------------------------------------------------------
void addToLog(String msg) {
  for (int i = log.length - 1; i > 0; i--) {
    log[i] = log[i - 1];
  }
  log[0] = msg;
}