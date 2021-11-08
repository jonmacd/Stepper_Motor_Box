// Libraries to use Adafruit seven segment backpack
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"

// INPUT VARIABLES
const int potPin = A3; // Potentiometer pin to set speed
const int dnButt = 12; // Down button pin
const int stButt = 8;  // Stop button pin
const int upButt = 13; // Up button pin
const int zrButt = 7;  // Zero button pin
bool dnState = 0;
bool stState = 1;
bool upState = 0;
bool zrState = 0;
bool lastDnState = 0;
bool lastStState = 0;
bool lastUpState = 0;
bool lastZrState = 0;

// OUTPUT VARIABLES:
const int dnLED = 10; // Down button LED pin
const int stLED = 9;  // Stop button LED pin
const int upLED = 11; // Up button LED pin
const int zrLED = 6;  // Zero button LED pin

// MOTOR VARIABLES
const int enPin = 5; // Stepper driver enable pin
const int drPin = 4; // Stepper driver direction pin
const int plPin = 3; // Stepper driver pulse pin
int maxMotorSpeed = 500;   // Small amount of time low/high for motor pulses
int minMotorSpeed = 10000; // Largest amount of time low/high for motor pulses
int motorSpeed = 0;        // Variable that stores the speed set by the potentiometer
int motorSpeedPerc = 0;
int stepCount = 0;

// POT VARIABLES
int potVal = 0;
int lastPotVal = 0;
const int numReadings = 10; // running average size
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;            // the average
int readings[numReadings];  // the readings from the analog input

Adafruit_7segment matrix = Adafruit_7segment();

// FADE VARIABLES
// define directions for LED fade
#define UP 0
#define DOWN 1
// constants for min and max PWM
const int minPWM = 0;
const int maxPWM = 255;
// State Variable for Fade Direction
byte fadeDirection = UP;
// Global Fade Value
// but be bigger than byte and signed, for rollover
int fadeValue = 0;
// How smooth to fade?
byte fadeIncrement = 5;
// millis() timing Variable, just for fading
unsigned long previousFadeMillis;
// How fast to increment?
int fadeInterval = 10;

void setup() {
  // Start serial and seven segment display
  Serial.begin(9600);
  delay(200);
  matrix.begin(0x70);

  // Input pins
  pinMode(potPin, INPUT);
  pinMode(dnButt, INPUT_PULLUP);
  pinMode(stButt, INPUT_PULLUP);
  pinMode(upButt, INPUT_PULLUP);
  pinMode(zrButt, INPUT_PULLUP);

  // Output pins
  pinMode(dnLED, OUTPUT);
  pinMode(stLED, OUTPUT);
  pinMode(upLED, OUTPUT);
  pinMode(zrLED, OUTPUT);
  digitalWrite(stLED, HIGH);
  pinMode(plPin, OUTPUT);
  pinMode(drPin, OUTPUT);
  pinMode(enPin, OUTPUT);
}

void loop() {
  // Check for button presses
  checkButtons();
  // Read potentiometer to set motor speed
  readPot();  
  matrix.println(motorSpeedPerc);
  matrix.writeDisplay();

  // DOWN  
  // When down button is pressed
  if (dnState && dnState != lastDnState) {
    Serial.println("Down");
    matrix.println("down");
    matrix.writeDisplay();
    digitalWrite(stLED, LOW);
    digitalWrite(drPin, LOW);
    digitalWrite(enPin, LOW);
    // Move motor DOWN continuously until stop button pressed
    while (1) {
      unsigned long currentMillis = millis();
      doTheFade(currentMillis, dnLED);
      // Move the motor
      digitalWrite(plPin, HIGH);
      delayMicroseconds(motorSpeed);
      digitalWrite(plPin, LOW);
      delayMicroseconds(motorSpeed);
      stepCount++;
      if (!digitalRead(stButt)){
        analogWrite(dnLED, 0);
        stopMotor();
        break;
      }
    }
  }

  // STOP
  // When stop button is pressed, stop motor movement
  else if (stState && stState != lastStState) {
    stopMotor();
  }

  // UP (return to home position)
  // When up button is pressed and the motor is at the bottom of it's travel
  else if (upState && upState != lastUpState && stepCount > 0) {
    Serial.println("Up");
    matrix.println("  up");
    matrix.writeDisplay();
    digitalWrite(drPin, HIGH);
    digitalWrite(enPin, LOW);
    digitalWrite(stLED, LOW);
    // Move motor UP continuously until stop button pressed
    while (1) {
      unsigned long currentMillis = millis();
      doTheFade(currentMillis, upLED);
      // Move the motor
      digitalWrite(plPin, HIGH);
      delayMicroseconds(motorSpeed);
      digitalWrite(plPin, LOW);
      delayMicroseconds(motorSpeed);
      stepCount--;
      if (!digitalRead(stButt) || stepCount == 0){
        matrix.println("home");
        matrix.writeDisplay();
        delay(1000);
        analogWrite(upLED, LOW);
        stopMotor();
        break;
      }
    }
  }

  // UP (beyond home position)
  // When up button is pressed and the motor is already at home
  else if (upState && upState != lastUpState && stepCount == 0) {
    Serial.println("Up");
    matrix.println("  up");
    matrix.writeDisplay();
    digitalWrite(drPin, HIGH);
    digitalWrite(enPin, LOW);
    digitalWrite(stLED, LOW);
    // Move motor UP continuously until stop button pressed
    while (1) {
      unsigned long currentMillis = millis();
      doTheFade(currentMillis, upLED);
      // Move the motor
      digitalWrite(plPin, HIGH);
      delayMicroseconds(motorSpeed);
      digitalWrite(plPin, LOW);
      delayMicroseconds(motorSpeed);
      if (!digitalRead(stButt)){
        analogWrite(upLED, LOW);
        stopMotor();
        stepCount = 0;
        break;
      }
    }
  }

  // ZERO
  // Reset the step counter variable to set a home position to return to when motor moves up
  else if (zrState && zrState != lastZrState) {
    Serial.println("Zero");
    matrix.println("zero");
    matrix.writeDisplay();
    stepCount = 0;
    for (int i = 0; i < 255; i = i + 5) {
      analogWrite(zrLED, i);
      delay(2);
    }
    for (int i = 255; i > 0; i = i - 5) {
      analogWrite(zrLED, i);
      delay(2);
    }
    analogWrite(zrLED, 0);
    delay(500);
  }
  delay(25);
}

/*
 * Reads each button input to check if they've been pressed
 * If pressed sets a boolean state variable 
 */
void checkButtons() {
  dnState = !digitalRead(dnButt);
  stState = !digitalRead(stButt);
  upState = !digitalRead(upButt);
  zrState = !digitalRead(zrButt);
}

/*
 * Average analog readings and set the motor speed. Store it as a percentage.
 */
void readPot() {
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(potPin);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
    readIndex = 0;
  }
  potVal = total / numReadings;
  lastPotVal = potVal;
  motorSpeed = map(potVal, 0, 1023, minMotorSpeed, maxMotorSpeed);
  motorSpeedPerc = map(motorSpeed, minMotorSpeed, maxMotorSpeed, 0, 101);
  if (motorSpeedPerc == 101){
    motorSpeedPerc = 100;
  }
}

/*
 * Stops the stepper motor from spinning and resets last up/down state so up/down can be pressed again
 */
void stopMotor(){
  Serial.println("Stop");
  Serial.println(stepCount);
  // STOP MOTOR HERE
  matrix.println("stop");
  matrix.writeDisplay();
  // digitalWrite(enPin, HIGH); //uncomment if disabling the motor when stopped is desired
  digitalWrite(stLED, HIGH);
  delay(500);
  lastDnState = false;
  lastUpState = false;
}

/*
 * Fade an LED up and down using millis so other tasks can be done at the same time
 */
void doTheFade(unsigned long thisMillis, int LED) {
  // is it time to update yet?
  // if not, nothing happens
  if (thisMillis - previousFadeMillis >= fadeInterval) {
    // yup, it's time!
    if (fadeDirection == UP) {
      fadeValue = fadeValue + fadeIncrement;  
      if (fadeValue >= maxPWM) {
        // At max, limit and change direction
        fadeValue = maxPWM;
        fadeDirection = DOWN;
      }
    } else {
      //if we aren't going up, we're going down
      fadeValue = fadeValue - fadeIncrement;
      if (fadeValue <= minPWM) {
        // At min, limit and change direction
        fadeValue = minPWM;
        fadeDirection = UP;
      }
    }
    // Only need to update when it changes
    analogWrite(LED, fadeValue);  
    // reset millis for the next iteration (fade timer only)
    previousFadeMillis = thisMillis;
  }
}