#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>


// Try 0x3F if 0x27 shows blank blocks
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Motor Pins
const int EN1 = 11, IN1 = 7, IN2 = 6;
const int IN3 = 4, IN4 = 3, EN2 = 5;
const int GRIPPER_PIN = 10;

Servo gripper;

// Motion function declarations
void driveForward(int speed = 180);
void driveReverse(int speed = 180);
void stopMotors();

void setup() {
  Serial.begin(9600); // UART Link with Scout Robot A
  
  // LCD Setup
  Wire.begin();
  lcd.init(); 
  lcd.backlight();
  
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  gripper.attach(GRIPPER_PIN); 
  gripper.write(0); // Initialize open position

  // Boot Test Display
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("EXECUTER BOT");
  lcd.setCursor(0, 1); 
  lcd.print("Awaiting Target");
}

void loop() {
  if (Serial.available() > 0) {
    String packet = Serial.readStringUntil('\n');
    packet.trim();

    // Robot A sends: "TASK_ASSIGNED,basketDist,rackDist"
    if (packet.startsWith("TASK_ASSIGNED")) {
      
      // Parse parameters split by commas
      int firstComma = packet.indexOf(',');
      int secondComma = packet.indexOf(',', firstComma + 1);

      if (firstComma != -1 && secondComma != -1) {
        int basketDist = packet.substring(firstComma + 1, secondComma).toInt();
        int rackDist = packet.substring(secondComma + 1).toInt();

        // 1. Acknowledge Received Target D
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("Target Acquired");
        lcd.setCursor(0, 1); 
        lcd.print("B:" + String(basketDist) + "cm R:" + String(rackDist) + "cm");
        delay(1500);

        // 2. Drive Forward to Basket position based on basketDist
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("Navigating to");
        lcd.setCursor(0, 1); 
        lcd.print("Basket Target...");
        
        int driveTime = basketDist * 40; // ~40ms per cm target distance
        driveForward(180);
        delay(driveTime);
        stopMotors();
        delay(400);

        // 3. Pick Item (Close Gripper)
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("Action: Pickup");
        lcd.setCursor(0, 1); 
        lcd.print("Closing Gripper");
        
        for (int a = 0; a <= 90; a += 10) { 
          gripper.write(a); 
          delay(20); 
        }
        delay(500);

        // 4. Drive Forward toward Storage Rack based on rackDist
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("Moving to Rack");
        lcd.setCursor(0, 1); 
        lcd.print("Loading Target...");
        
        driveForward(180);
        delay(500);
        stopMotors();
        delay(300);

        // 5. Place Item on Rack (Open Gripper)
        for (int a = 90; a >= 0; a -= 10) { 
          gripper.write(a); 
          delay(20); 
        }
        delay(500);

        // 6. Reverse step to clear rack structure
        driveReverse(150);
        delay(400);
        stopMotors();

        // 7. Send Completion Handshake back to Scout Robot A
        Serial.println("MISSION_COMPLETE");

        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print("TASK COMPLETE!");
        lcd.setCursor(0, 1); 
        lcd.print("Standing By...");
      }
    }
  }
}

// --- MOTION CONTROL FUNCTIONS ---
void driveForward(int speed) {
  analogWrite(EN1, speed); 
  analogWrite(EN2, speed);
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, HIGH);
}

void driveReverse(int speed) {
  analogWrite(EN1, speed); 
  analogWrite(EN2, speed);
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); 
  digitalWrite(IN4, LOW);
}

void stopMotors() {
  analogWrite(EN1, 0); 
  analogWrite(EN2, 0);
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); 
  digitalWrite(IN4, LOW);
}