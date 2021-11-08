// INPUT VARIABLES
const int potPin = A0;
const int dnButt = A1;
const int stButt = A2;
const int upButt = A3;
const int zrButt = A4;
bool dnState = 0;
bool stState = 1;
bool upState = 0;
bool zrState = 0;
bool lastDnState = 0;
bool lastStState = 0;
bool lastUpState = 0;
bool lastZrState = 0;

//OUTPUT VARIABLES:
const int dnLED = 5;
const int stLED = 9;
const int upLED = 10;
const int zrLED = 11;

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
  Serial.begin(9600);
  pinMode(potPin, INPUT);
  pinMode(dnButt, INPUT_PULLUP);
  pinMode(stButt, INPUT_PULLUP);
  pinMode(upButt, INPUT_PULLUP);
  pinMode(zrButt, INPUT_PULLUP);
  pinMode(dnLED, OUTPUT);
  pinMode(stLED, OUTPUT);
  pinMode(upLED, OUTPUT);
  pinMode(zrLED, OUTPUT);
  digitalWrite(stLED, HIGH);
}

void loop() {
  checkButtons();

  // When down button true and not equal to last state
  if (dnState && dnState != lastDnState) {
    Serial.println("Down");
    digitalWrite(stLED, LOW);
    // Move motor DOWN continuously until stop button pressed
    while (1) {
      unsigned long currentMillis = millis();
      doTheFade(currentMillis, dnLED);
      if (!digitalRead(stButt)){
        analogWrite(dnLED, 0);
        break;
      }
    }
  }
  // When stop button is pressed, stop any motor movement, reset the Dn and Up last states
  else if (stState && stState != lastStState) {
    Serial.println("Stop");
    // STOP MOTOR HERE
    digitalWrite(stLED, HIGH);
    lastDnState = false;
    lastUpState = false;
    // delay(250);
  }
  // When up button true and not equal to last state
  else if (upState && upState != lastUpState) {
    Serial.println("Up");
    digitalWrite(stLED, LOW);
    // Move motor UP continuously until stop button pressed
    while (1) {
      unsigned long currentMillis = millis();
      doTheFade(currentMillis, upLED);
      if (!digitalRead(stButt)){
        analogWrite(upLED, LOW);
        break;
      }
    }
  }
  else if (zrState && zrState != lastZrState) {
    Serial.println("Zero");
    // SET HOME POSITION HERE
    for (int i = 0; i < 255; i = i + 5) {
      analogWrite(zrLED, i);
      delay(2);
    }
    for (int i = 255; i > 0; i = i - 5) {
      analogWrite(zrLED, i);
      delay(2);
    }
    analogWrite(zrLED, 0);
  }
  delay(50);
}

// Read each input button and change the state variables:
void checkButtons(){
  dnState = !digitalRead(dnButt);
  stState = !digitalRead(stButt);
  upState = !digitalRead(upButt);
  zrState = !digitalRead(zrButt);
}

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