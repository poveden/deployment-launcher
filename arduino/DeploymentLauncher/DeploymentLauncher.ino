/**
  Deployment launcher

  Contributors:
  - Dan Figueras
  - Jorge Poveda

  Required libraries:
  - Timer1 (http://playground.arduino.cc/Code/Timer1)
**/

#include <TimerOne.h>

#define DEBUG

#ifdef DEBUG
  #define TRACE(x) Serial.print(x)
  #define TRACELN(x) Serial.println(x)
#else
  #define TRACE(x)
  #define TRACELN(x)
#endif


// Pin configuration

const int switch1Pin = 2;             // Switch 1
const int switch2Pin = 4;             // Switch 2
const int deployButtonPin = 7;        // Deploy button

const int armedLedPin = 11;           // "Armed" LED (Requires PWM pin)
const int armFailedLedPin = 8;        // "Arm failed" LED
const int deployLedPin = 12;          // "Deploy enabled" LED
const int timerLedPin = LED_BUILTIN;  // Arming timer LED

const long armingTimeoutMs = 300;     // Arming timeout in milliseconds.


// Armed states

typedef enum armedState_t {
  UNARMED,
  ARMING,
  ARMED,
  ARMFAILED
} armedState_t;

armedState_t armedState = UNARMED;
bool timerExpired = false;
bool deployed = false;


//-------------------------------------------

void setup() {
  pinMode(armedLedPin, OUTPUT);
  pinMode(armFailedLedPin, OUTPUT);
  pinMode(timerLedPin, OUTPUT);
  pinMode(deployLedPin, OUTPUT);

  pinMode(switch1Pin, INPUT);
  pinMode(switch2Pin, INPUT);
  pinMode(deployButtonPin, INPUT);

  Timer1.attachInterrupt(timerIsr);
  Timer1.initialize(0);
  Timer1.stop();

  Serial.begin(9600);
  TRACELN("Deployment launcher started.");
}


void loop() {
  armedStateHandler();
  deployButtonHandler();
  armedLedHandler();
}

void timerStart() {
  TRACELN("timerStart");
  timerExpired = false;
  Timer1.setPeriod(armingTimeoutMs * 1000);
  digitalWrite(timerLedPin, HIGH);
}

void timerStop() {
  TRACELN("timerStop");
  Timer1.stop();
  digitalWrite(timerLedPin, LOW);
}

void timerIsr() {
  TRACELN("timerIsr");
  timerStop();
  timerExpired = true;
  setArmedState(ARMFAILED);
}

void armedStateHandler() {
  int switch1State = digitalRead(switch1Pin);
  int switch2State = digitalRead(switch2Pin);

  int armSwitchCount = 0;
  if (switch1State == HIGH) armSwitchCount++;
  if (switch2State == HIGH) armSwitchCount++;

  armedState_t newArmedState = armedState;
  
  switch (armedState) {
    case UNARMED:
      if (armSwitchCount == 1) {
        timerStart();
        newArmedState = ARMING;
      } else if (armSwitchCount == 2) {
        newArmedState = ARMED;
      }
      break;
    case ARMING:
      if (armSwitchCount == 0) {
        timerStop();
        newArmedState = UNARMED;
      } else if (armSwitchCount == 2 && !timerExpired) {
        timerStop();
        newArmedState = ARMED;
      }
      break;
    case ARMED:
      if (armSwitchCount != 2) {
        setDeployedFlag(false);
        newArmedState = (armSwitchCount != 0) ? ARMFAILED : UNARMED;
      }
      break;
    case ARMFAILED:
      if (armSwitchCount == 0) {
        newArmedState = UNARMED;
      }
      break;
  }

  if (newArmedState != armedState) {
    setArmedState(newArmedState);
  }
}

void deployButtonHandler() {
  if (armedState != ARMED || deployed) return;
  
  int deployButtonState = digitalRead(deployButtonPin);

  if (deployButtonState == HIGH) {
    setDeployedFlag(true);
  }
}

void setDeployedFlag(bool isDeployed) {
  if (isDeployed) {
    Serial.println("DEPLOY!!!!!!!!!!!!!");
    deployed = true;
    digitalWrite(deployLedPin, HIGH);
  } else {
    deployed = false;
    digitalWrite(deployLedPin, LOW);
  }
}

void setArmedState(armedState_t newArmedState) {
  armedState = newArmedState;
  TRACE("Moving to armed state ");
  TRACELN(armedState);
  
  switch (armedState) {
    case UNARMED:
      digitalWrite(armFailedLedPin, LOW);
      break;
    case ARMING:
      break;
    case ARMED:
      digitalWrite(armFailedLedPin, LOW);
      break;
    case ARMFAILED:
      digitalWrite(armFailedLedPin, HIGH);
      break;
  }
}

const long pulseLengthMs = 700;
const int pulsePower = 64;
int armedLedLevel = 0;

void armedLedHandler() {
  if (armedState == ARMED) {
    float rad = PI * (millis() % pulseLengthMs) / pulseLengthMs;
    armedLedLevel = pulsePower - abs(cos(rad)) * pulsePower; // Peak-shaped light curve.
    
    analogWrite(armedLedPin, armedLedLevel);
  } else {
    analogWrite(armedLedPin, LOW);
  }
}
