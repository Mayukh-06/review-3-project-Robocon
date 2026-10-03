#include <Servo.h>

Servo s;

// Motor & Driver Pins (L293D)
int EN1 = 6, IN1 = 4, IN2 = 3;
int EN2 = 5, IN3 = 7, IN4 = 8;

// Ultrasonic servo Pins
int trig = 11, echo = 10, servoPin = 9;

// Discrete LED Pins
int ledRed = 13;
int ledYellow = 12;
int ledGreen = 2;

// --- CLASS 1: Mobile Base Driver ---
class RoverBase {
  public:
    void initial() {
      pinMode(EN1, OUTPUT); 
      pinMode(IN1, OUTPUT); 
      pinMode(IN2, OUTPUT); 
      pinMode(EN2, OUTPUT);
      pinMode(IN3, OUTPUT);
      pinMode(IN4, OUTPUT);
    }
    void driveForward(int speed) {
      digitalWrite(IN1, HIGH); 
      digitalWrite(IN2, LOW);
      analogWrite(EN1, speed);
      
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      analogWrite(EN2, speed);
    }
    void driveReverse(int speed) {
      digitalWrite(IN1, LOW); 
      digitalWrite(IN2, HIGH);
      analogWrite(EN1, speed);
      
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      analogWrite(EN2, speed);
    }
    void stop() {
      digitalWrite(IN1, HIGH); 
      digitalWrite(IN2, LOW);
      analogWrite(EN1, 0);
      
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      analogWrite(EN2, 0);
    }
};

// --- CLASS 2: Discrete Status LED Indicator ---
class StatusIndicator {
  public:
  
    void initial() {
      pinMode(ledRed, OUTPUT);
      pinMode(ledYellow, OUTPUT);
      pinMode(ledGreen, OUTPUT);
      offState();
    }

    void offState() {
      digitalWrite(ledRed, LOW);
      digitalWrite(ledYellow, LOW);
      digitalWrite(ledGreen, LOW);
    }

    void showSearching() {
      offState();
      digitalWrite(ledYellow, HIGH); // Yellow = Actively Searching Arena
    }

    void showTargetFound() {
      offState();
      digitalWrite(ledRed, HIGH);    // Red = Target Acquired / Stopped
    }

    void showComplete() {
      offState();
      digitalWrite(ledGreen, HIGH);  // Green = Dispatched / Mission Success
    }
};

// --- CLASS 3: Turret Scanner ---
class TargetScanner {
  public:
    void initial(int pin) {
      pinMode(trig, OUTPUT); 
      pinMode(echo, INPUT);
      s.attach(pin);
    }
  
    int pingAt(int angle) {
      s.write(angle);
      delay(200);
      
      digitalWrite(trig, LOW); 
      delayMicroseconds(2);
      digitalWrite(trig, HIGH); 
      delayMicroseconds(10);
      digitalWrite(trig, LOW);
      
      int duration = pulseIn(echo, HIGH, 25000);
      int distance = duration * 0.034 / 2;
      
      return distance;
    }
};

// Object Instantiations
RoverBase base;
StatusIndicator status;
TargetScanner scanner;

enum State { 
  Searching, 
  Dispatching, 
  ReturningToBase, 
  atBase
};

State currentState = Searching;

void setup() {
  Serial.begin(9600); // Bidirectional UART
  base.initial();
  status.initial();
  scanner.initial(servoPin);
  
  status.showSearching(); // Yellow LED on
}

void loop() {
  if (currentState == Searching) {
    status.showSearching(); // Yellow LED = Searching
    
    // Drive forward into arena
    base.driveForward(150);
    delay(500);
    base.stop();

    long basketDist = scanner.pingAt(30);
    delay(5000);// Low scan (Floor Basket)
    long rackDist = scanner.pingAt(120);
    delay(5000);// High scan (Storage Rack)

    if (basketDist < 25 && rackDist < 40) {
      status.showTargetFound(); // Red LED = Target Locked!
      currentState = Dispatching;
      
      // Send coordinate payload to Executor Robot B
      Serial.print("TASK_ASSIGNED,");
      Serial.print(basketDist); 
      Serial.print(",");
      Serial.println(rackDist);
    }
  } 
  else if (currentState == Dispatching) {
    // Listen on UART for confirmation from Executor B
    if (Serial.available() > 0) {
      String reply = Serial.readStringUntil('\n');
      if (reply.startsWith("MISSION_COMPLETE")) {
        status.showComplete(); // Green LED = Mission Complete!
        currentState = ReturningToBase;
      }
    }
  } 
  else if (currentState == ReturningToBase) {
    // Reverse back to starting base
    base.driveReverse(150);
    delay(1000); 
    base.stop();
    currentState = atBase;
  }
}