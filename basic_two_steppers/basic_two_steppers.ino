#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>
#include <FastLED.h>

#include <FastLED.h>

#define LED_PIN     8
#define NUM_LEDS    120
#define LED_BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define LED_COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define LED_UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

#include <CommandParser.h>
typedef CommandParser<> MyCommandParser;
MyCommandParser parser;

#define STEPS_PER_REV 200
#define MICROSTEP_RATIO 5
#define PULLEY_DIAMETER_MM 20
#define WORKSPACE_X_MM 950
#define WORKSPACE_Y_MM 250
#define SINGLE_STEP_BELT_MM PULLEY_DIAMETER_MM / (STEPS_PER_REV / MICROSTEP_RATIO)

#define LIMIT_DEBOUNCE_MICROS 10

#define X_AXIS_NEG_LIMIT_PIN 30
#define X_AXIS_POS_LIMIT_PIN 26
#define Y_AXIS_NEG_LIMIT_PIN 24
#define Y_AXIS_POS_LIMIT_PIN 28

#define STEPPER1_DIR_CONST 1
#define STEPPER2_DIR_CONST -1
AccelStepper stepper1(AccelStepper::DRIVER, 44, 48);
AccelStepper stepper2(AccelStepper::DRIVER, 50, 52);

Servo drink1;
Servo drink2;
Servo drink3;
Servo drink4;
Servo drink5;
Servo drink6;
Servo drink7;
Servo drink8;
Servo drink9;
Servo drink10;
#define DRINK_1_SERVO_PIN 7
#define DRINK_2_SERVO_PIN 10//3
#define DRINK_3_SERVO_PIN 3//12
#define DRINK_4_SERVO_PIN 9
#define DRINK_5_SERVO_PIN 12
#define DRINK_6_SERVO_PIN 4
#define DRINK_7_SERVO_PIN 5 
#define DRINK_8_SERVO_PIN 6
#define DRINK_9_SERVO_PIN 11
#define DRINK_10_SERVO_PIN 2

#define DRINK_1_MIN_POS 0
#define DRINK_2_MIN_POS 0
#define DRINK_3_MIN_POS 0
#define DRINK_4_MIN_POS 0
#define DRINK_5_MIN_POS 0
#define DRINK_6_MIN_POS 0
#define DRINK_7_MIN_POS 0
#define DRINK_8_MIN_POS 0
#define DRINK_9_MIN_POS 0
#define DRINK_10_MIN_POS 0

#define DRINK_1_MAX_POS 150
#define DRINK_2_MAX_POS 150
#define DRINK_3_MAX_POS 150
#define DRINK_4_MAX_POS 150
#define DRINK_5_MAX_POS 150
#define DRINK_6_MAX_POS 150
#define DRINK_7_MAX_POS 150
#define DRINK_8_MAX_POS 150
#define DRINK_9_MAX_POS 150
#define DRINK_10_MAX_POS 150

#define DRINK_SERVO_TIME_MS 2500

// These drink coords are NOT microstep scaled
#define DRINK_1_Y_CORD 20//NB: let's assume for now that they are roughly colinear
#define DRINK_2_Y_CORD 575

#define DRINK_1_X_CORD 400
#define DRINK_3_X_CORD 900
#define DRINK_5_X_CORD 1375
#define DRINK_7_X_CORD 1900
#define DRINK_9_X_CORD 2400

#define DRINK_2_X_CORD 450
#define DRINK_4_X_CORD 950
#define DRINK_6_X_CORD 1400
#define DRINK_8_X_CORD 1900
#define DRINK_10_X_CORD 2380

#define PUMP_RIGHT_X_CORD 2750
#define PUMP_LEFT_X_CORD 100
#define PUMP_RIGHT_Y_CORD 590
#define PUMP_LEFT_Y_CORD 590

#define ALL_PUMP_DIR_PIN 47
#define PUMP1_STEP_PIN 45
#define PUMP1_SLEEP_PIN 43
#define PUMP2_STEP_PIN 22
#define PUMP2_SLEEP_PIN 33
#define PUMP3_STEP_PIN 39
#define PUMP3_SLEEP_PIN 35
#define PUMP4_STEP_PIN 25
#define PUMP4_SLEEP_PIN 31
#define PUMP5_STEP_PIN 23
#define PUMP5_SLEEP_PIN 27
#define PUMP6_STEP_PIN 29
#define PUMP6_SLEEP_PIN 37

AccelStepper pump1(AccelStepper::DRIVER, PUMP1_STEP_PIN, ALL_PUMP_DIR_PIN);
AccelStepper pump2(AccelStepper::DRIVER, PUMP2_STEP_PIN, ALL_PUMP_DIR_PIN);
AccelStepper pump3(AccelStepper::DRIVER, PUMP3_STEP_PIN, ALL_PUMP_DIR_PIN);
AccelStepper pump4(AccelStepper::DRIVER, PUMP4_STEP_PIN, ALL_PUMP_DIR_PIN);
AccelStepper pump5(AccelStepper::DRIVER, PUMP5_STEP_PIN, ALL_PUMP_DIR_PIN);
AccelStepper pump6(AccelStepper::DRIVER, PUMP6_STEP_PIN, ALL_PUMP_DIR_PIN);

//MultiStepper steppers;
// Temp variables, to be removed
int currentDirection = -1;
int moveIncrements = 20000;
unsigned long startTime;

bool x_axis_neg_lim;
bool x_axis_pos_lim;
bool y_axis_neg_lim;
bool y_axis_pos_lim;

float x_max_steps; //this is the number of steps in the x from limit to limit.
float y_max_steps; //this is the number of steps in the y from limit to limit.

void updateLimitSwitches(){
  //NB: limit switches are active low
  x_axis_neg_lim = !digitalRead(X_AXIS_NEG_LIMIT_PIN);
  x_axis_pos_lim = !digitalRead(X_AXIS_POS_LIMIT_PIN);
  y_axis_neg_lim = !digitalRead(Y_AXIS_NEG_LIMIT_PIN);
  y_axis_pos_lim = !digitalRead(Y_AXIS_POS_LIMIT_PIN);

 if (x_axis_neg_lim || x_axis_pos_lim || y_axis_neg_lim || y_axis_pos_lim) {
   delayMicroseconds(LIMIT_DEBOUNCE_MICROS);

    x_axis_neg_lim &= !digitalRead(X_AXIS_NEG_LIMIT_PIN);
    x_axis_pos_lim &= !digitalRead(X_AXIS_POS_LIMIT_PIN);
    y_axis_neg_lim &= !digitalRead(Y_AXIS_NEG_LIMIT_PIN);
    y_axis_pos_lim &= !digitalRead(Y_AXIS_POS_LIMIT_PIN);
  }
}

void printLimitSwitches(){
  Serial.print(x_axis_neg_lim);
  Serial.print("\t");
  Serial.print(x_axis_pos_lim);
  Serial.print("\t");
  Serial.print(y_axis_neg_lim);
  Serial.print("\t");
  Serial.print(y_axis_pos_lim);
  Serial.print("\t");
  Serial.println();
}

MultiStepper steppersLinked;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);

  pinMode(PUMP1_SLEEP_PIN, OUTPUT);
  pinMode(PUMP2_SLEEP_PIN, OUTPUT);
  pinMode(PUMP3_SLEEP_PIN, OUTPUT);
  pinMode(PUMP4_SLEEP_PIN, OUTPUT);
  pinMode(PUMP5_SLEEP_PIN, OUTPUT);
  pinMode(PUMP6_SLEEP_PIN, OUTPUT);
  digitalWrite(PUMP1_SLEEP_PIN, LOW);
  digitalWrite(PUMP2_SLEEP_PIN, LOW);
  digitalWrite(PUMP3_SLEEP_PIN, LOW);
  digitalWrite(PUMP4_SLEEP_PIN, LOW);
  digitalWrite(PUMP5_SLEEP_PIN, LOW);
  digitalWrite(PUMP6_SLEEP_PIN, LOW);

  while (!Serial);

  parser.registerCommand("MOVETO", "i", &moveToCommandImpl);
  parser.registerCommand("DISPENSE", "ii", &dispenseCommandImpl);
  parser.registerCommand("PUMP", "ii", &pumpCommandImpl);
  parser.registerCommand("HOME", "", &moveToHomePosition);
  parser.registerCommand("TEST", "", &builtinSelfTest);

  startTime = millis();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  
  pinMode(X_AXIS_NEG_LIMIT_PIN, INPUT);
  pinMode(X_AXIS_POS_LIMIT_PIN, INPUT);
  pinMode(Y_AXIS_NEG_LIMIT_PIN, INPUT);
  pinMode(Y_AXIS_POS_LIMIT_PIN, INPUT);

  // Configure each stepper
  stepper1.setAcceleration(10 * MICROSTEP_RATIO);
  stepper1.setMaxSpeed(1000 * MICROSTEP_RATIO);
  stepper2.setAcceleration(10 * MICROSTEP_RATIO);
  stepper2.setMaxSpeed(1000 * MICROSTEP_RATIO);

  stepper1.setSpeed(10);
  stepper2.setSpeed(10);

  pump1.setAcceleration(100);
  pump1.setMaxSpeed(500);
  pump2.setAcceleration(100);
  pump2.setMaxSpeed(500);
  pump3.setAcceleration(200);
  pump3.setMaxSpeed(500);
  pump4.setAcceleration(200);
  pump4.setMaxSpeed(500);
  pump5.setAcceleration(100);
  pump5.setMaxSpeed(500);
  pump6.setAcceleration(100);
  pump6.setMaxSpeed(500);

  Serial.println("STEPPER DIRECTION CONSTANTS");
  Serial.println(STEPPER1_DIR_CONST);
  Serial.println(STEPPER2_DIR_CONST);

  steppersLinked.addStepper(stepper1);
  steppersLinked.addStepper(stepper2);

  drink1.attach(DRINK_1_SERVO_PIN);
  drink2.attach(DRINK_2_SERVO_PIN);
  drink3.attach(DRINK_3_SERVO_PIN);
  drink4.attach(DRINK_4_SERVO_PIN);
  drink5.attach(DRINK_5_SERVO_PIN);
  drink6.attach(DRINK_6_SERVO_PIN);
  drink7.attach(DRINK_7_SERVO_PIN);
  drink8.attach(DRINK_8_SERVO_PIN);
  drink9.attach(DRINK_9_SERVO_PIN);
  drink10.attach(DRINK_10_SERVO_PIN);
  drink1.write(DRINK_1_MIN_POS);
  drink2.write(DRINK_2_MIN_POS);
  drink3.write(DRINK_3_MIN_POS);
  drink4.write(DRINK_4_MIN_POS);
  drink5.write(DRINK_5_MIN_POS);
  drink6.write(DRINK_6_MIN_POS);
  drink7.write(DRINK_7_MIN_POS);
  drink8.write(DRINK_8_MIN_POS);
  drink9.write(DRINK_9_MIN_POS);
  drink10.write(DRINK_10_MIN_POS);

  FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  LED_BRIGHTNESS );
  
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

  updateLimitSwitches();
  printLimitSwitches();
  delay(500);

/*
  while(true){
    updateLimitSwitches();
    printLimitSwitches();
    delay(500);
  }
*/
  //calibrate();
  home();
  ChangePalettePeriodically();
  static uint8_t LEDstartIndex = 0;
  LEDstartIndex = LEDstartIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors( LEDstartIndex);
  FastLED.show();

  Serial.println("$ DONE");
}

const int MAX_SERIAL_BUFFER = 128; // Maximum size for the serial input buffer

char serialBuffer[MAX_SERIAL_BUFFER];
size_t serialIndex = 0;

const uint8_t header[4] = { 'F', 'O', 'O', ' ' };

void loop() {
  updateLimitSwitches();

  ChangePalettePeriodically();
  static uint8_t LEDstartIndex = 0;
  LEDstartIndex = LEDstartIndex + 1; /* motion speed */
  char line[128];

  /*while(true) {
    if (Serial.available() == 0){
      break;
    } // wait for new serial data
    uint8_t b = Serial.read();
    bool looksLikeHeader = false;
    if(b == header[0]) {
      looksLikeHeader = true;
      for(int i = 1; looksLikeHeader && (i < sizeof(header)); i++) {
        while(Serial.available() == 0){} // wait for new serial data
        b = Serial.read();
        if(b != header[i]) {
          // whoops, not a match, this no longer looks like a header.
          looksLikeHeader = false;
        }
      }
    }

    if(looksLikeHeader) {
      // hey, we read all the header bytes!  Yay!  Now read the frame data
      size_t lineLength = Serial.readBytesUntil('\n', line, 127);
      line[lineLength] = '\0';
      Serial.println(line);
      char response[MyCommandParser::MAX_RESPONSE_SIZE];
      parser.processCommand(line, response);
      Serial.println("$ DONE");
      Serial.flush();
      //l bytes are read into led buffer. Now we can break out of while loop
      break;
    }
  }
  */

  if (Serial.available() > 0) {
    Serial.println(Serial.available());
    size_t lineLength = Serial.readBytesUntil('\n', line, 127);
    line[lineLength] = '\0';

    // NB: we don't need a response at all really
    Serial.println(line);
    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.flush();
    Serial.println("$ DONE");
  }

  FillLEDsFromPaletteColors( LEDstartIndex);
  //FastLED.show();
  //FastLED.delay(1000 / LED_UPDATES_PER_SECOND);
  delay(1000 / LED_UPDATES_PER_SECOND);
  
}

void builtinSelfTest(){
  Serial.println("Sending drink servos to defaults...");
  delay(5000);

  for (int i = 1; i <= 10; i++) {
    Serial.print("Goto drink #");
    Serial.println(i);
    moveToDrinkPos(i);
    actuateDrinkServo(i, 1);
  }

  for (int i = 101; i <= 106; i++) {
    moveToDrinkPos(i);
    pump(i - 100, 50);
  }
}

#define HOMING_SPEED 700
#define X_AXIS_NEG_CONST -1
#define X_AXIS_POS_CONST 1

void calibrate(){

  home();

  // Now we need to find the other x limit and note it's position in steps
  Serial.println("FINDING X LIMIT");
  unsigned long StartTimeXLimit = millis();
  stepper1.setSpeed(HOMING_SPEED * MICROSTEP_RATIO * STEPPER1_DIR_CONST * X_AXIS_POS_CONST);
  stepper2.setSpeed(HOMING_SPEED * MICROSTEP_RATIO * STEPPER2_DIR_CONST * X_AXIS_POS_CONST);
  while(!x_axis_pos_lim){
    updateLimitSwitches();
    stepper1.runSpeed();
    stepper2.runSpeed();
  }
  unsigned long stopTimeXLimit = millis();

  x_max_steps = abs(stepper1.currentPosition());
  Serial.println(stepper1.currentPosition());
  Serial.println(stepper2.currentPosition());
  Serial.print(stopTimeXLimit - StartTimeXLimit);
  Serial.println(" ms to find target");
  
  // Now find y limit
  Serial.println("FINDING Y LIMIT");
  stepper1.setSpeed(-1 * HOMING_SPEED * MICROSTEP_RATIO * STEPPER1_DIR_CONST);
  // We multiply by -1 here to move rightwards
  stepper2.setSpeed(1 * HOMING_SPEED * MICROSTEP_RATIO * STEPPER2_DIR_CONST);
  while(!y_axis_pos_lim){
    updateLimitSwitches();
    stepper1.runSpeed();
    stepper2.runSpeed();
  }

  // NB this calculation is wrong but we don't use it anywhere
  y_max_steps = max(stepper1.currentPosition(), stepper2.currentPosition()) - x_max_steps;
  Serial.println(stepper1.currentPosition());
  Serial.println(stepper2.currentPosition());

  Serial.println("Go to home....");
  moveToHomePosition();

  Serial.println("Calibration complete!");
  delay(2000);

}

//find the home position and resets the steppers
void home() {
  // First find our x home
  Serial.println("FINDING X HOME");
  stepper1.setSpeed(HOMING_SPEED * MICROSTEP_RATIO * STEPPER1_DIR_CONST * X_AXIS_NEG_CONST);
  stepper2.setSpeed(HOMING_SPEED * MICROSTEP_RATIO * STEPPER2_DIR_CONST * X_AXIS_NEG_CONST);

  updateLimitSwitches();
  while(!x_axis_neg_lim){
    updateLimitSwitches();
    stepper1.runSpeed();
    stepper2.runSpeed();
  }

  stepper1.stop();
  stepper2.stop();

  printLimitSwitches();

  // Now find y home
  Serial.println("FINDING Y HOME");
  stepper1.setSpeed(1 * HOMING_SPEED * MICROSTEP_RATIO * STEPPER1_DIR_CONST);
  // We multiply by 1 here to move leftwards
  stepper2.setSpeed(-1 * HOMING_SPEED * MICROSTEP_RATIO * STEPPER2_DIR_CONST);

  updateLimitSwitches();
  while(!y_axis_neg_lim){
    updateLimitSwitches();
    stepper1.runSpeed();
    stepper2.runSpeed();
  }

  Serial.println(stepper1.currentPosition());
  Serial.println(stepper2.currentPosition());
  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);
  Serial.println(stepper1.currentPosition());
  Serial.println(stepper2.currentPosition());
}

void moveToTargetPositionLinkedBlocking(int x, int y) {
  int target_x = (x - y) * MICROSTEP_RATIO;
  int target_y = (-x - y) * MICROSTEP_RATIO;

  /*if (target_x > x_max_steps) {
    Serial.println("PANIC! Attempt to drive past x bounds!");
    while(true) {
      // Loop forever
    }
  }*/

  /*if (target_y > y_max_steps) {
    Serial.println("PANIC! Attemot to drive past y bounds!");
    while (true) {
      // Loop forever
    }
  }*/

  moveToTargetStepsLinkedBlocking(target_x, target_y);
}

void moveToTargetStepsLinkedBlocking(long step1Target, long step2Target) {
  long positions[2];
  positions[0] = step1Target;
  positions[1] = step2Target;

  steppersLinked.moveTo(positions);
  steppersLinked.runSpeedToPosition();
}

void moveToHomePosition(){
  moveToTargetStepsLinkedBlocking(0, 0);
  home();
}

void actuateDrinkServo(int drinkNum, int num){
  for (int i = 0; i < num; i++){
    switch(drinkNum){
      case 1:
        Serial.println("Drink 1");
        drink1.write(DRINK_1_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink1.write(DRINK_1_MIN_POS);
        break;
      case 2:
        Serial.println("Drink 2");
        drink2.write(DRINK_2_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink2.write(DRINK_2_MIN_POS);
        break;
      case 3:
        Serial.println("Drink 3");
        drink3.write(DRINK_3_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink3.write(DRINK_3_MIN_POS);
        break;
      case 4:
        Serial.println("Drink 4");
        drink4.write(DRINK_4_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink4.write(DRINK_4_MIN_POS);
        break;
      case 5:
        Serial.println("Drink 5");
        drink5.write(DRINK_5_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink5.write(DRINK_5_MIN_POS);
        break;
      case 6:
        Serial.println("Drink 6");
        drink6.write(DRINK_6_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink6.write(DRINK_6_MIN_POS);
        break;
      case 7:
        Serial.println("Drink 7");
        drink7.write(DRINK_7_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink7.write(DRINK_7_MIN_POS);
        break;
      case 8:
        Serial.println("Drink 8");
        drink8.write(DRINK_8_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink8.write(DRINK_8_MIN_POS);
        break;
      case 9:
        Serial.println("Drink 9");
        drink9.write(DRINK_9_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink9.write(DRINK_9_MIN_POS);
        break;
      case 10:
        Serial.println("Drink 10");
        drink10.write(DRINK_10_MAX_POS);
        delay(DRINK_SERVO_TIME_MS);
        drink10.write(DRINK_10_MIN_POS);
        break;
      }
      delay(1500);
    }
}

void moveToDrinkPos(int drinkNum) {
    switch(drinkNum){
    case 1:
      moveToTargetPositionLinkedBlocking(DRINK_1_X_CORD, DRINK_1_Y_CORD);
      break;
    case 2:
      moveToTargetPositionLinkedBlocking(DRINK_2_X_CORD, DRINK_2_Y_CORD);
      break;
    case 3:
      moveToTargetPositionLinkedBlocking(DRINK_3_X_CORD, DRINK_1_Y_CORD);
      break;
    case 4:
      moveToTargetPositionLinkedBlocking(DRINK_4_X_CORD, DRINK_2_Y_CORD);
      break;
    case 5:
      moveToTargetPositionLinkedBlocking(DRINK_5_X_CORD, DRINK_1_Y_CORD);
      break;
    case 6:
      moveToTargetPositionLinkedBlocking(DRINK_6_X_CORD, DRINK_2_Y_CORD);
      break;
    case 7:
      moveToTargetPositionLinkedBlocking(DRINK_7_X_CORD, DRINK_1_Y_CORD);
      break;
    case 8:
      moveToTargetPositionLinkedBlocking(DRINK_8_X_CORD, DRINK_2_Y_CORD);
      break;
    case 9:
      moveToTargetPositionLinkedBlocking(DRINK_9_X_CORD, DRINK_1_Y_CORD);
      break;
    case 10:
      moveToTargetPositionLinkedBlocking(DRINK_10_X_CORD, DRINK_2_Y_CORD);
      break;
    case 101:
    case 102:
    case 103:
      moveToTargetPositionLinkedBlocking(PUMP_RIGHT_X_CORD, PUMP_RIGHT_Y_CORD);
      break;
    case 104:
    case 105:
    case 106:
      moveToTargetPositionLinkedBlocking(PUMP_LEFT_X_CORD, PUMP_LEFT_Y_CORD);
      break;
    default:
      break;
  }
}

#define PUMP1_MLS_TO_STEPS -55
#define PUMP2_MLS_TO_STEPS -55
#define PUMP3_MLS_TO_STEPS -55
#define PUMP4_MLS_TO_STEPS 55
#define PUMP5_MLS_TO_STEPS 55
#define PUMP6_MLS_TO_STEPS 55

void pump(int pumpNumber, int pumpAmountMls) {
  switch (pumpNumber) {
    case 1:
      Serial.println("Pump 1");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP1_SLEEP_PIN, HIGH);
      pump2.enableOutputs();
      pump1.move(pumpAmountMls * PUMP1_MLS_TO_STEPS);
      pump1.runToPosition();
      pump1.disableOutputs();
      digitalWrite(PUMP1_SLEEP_PIN, LOW);
      break;
    case 2:
      Serial.println("Pump 2");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP2_SLEEP_PIN, HIGH);
      pump2.enableOutputs();
      pump2.move(pumpAmountMls * PUMP2_MLS_TO_STEPS);
      pump2.runToPosition();
      pump1.disableOutputs();
      digitalWrite(PUMP2_SLEEP_PIN, LOW);
      break;
    case 3:
      Serial.println("Pump 3");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP3_SLEEP_PIN, HIGH);
      pump3.enableOutputs();
      pump3.move(pumpAmountMls * PUMP3_MLS_TO_STEPS);
      pump3.runToPosition();
      pump3.disableOutputs();
      digitalWrite(PUMP3_SLEEP_PIN, LOW);
      break;
    case 4:
      Serial.println("Pump 4");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP4_SLEEP_PIN, HIGH);
      pump4.enableOutputs();
      pump4.move(pumpAmountMls * PUMP4_MLS_TO_STEPS);
      pump4.runToPosition();
      pump4.disableOutputs();
      digitalWrite(PUMP4_SLEEP_PIN, LOW);
      break;
    case 5:
      Serial.println("Pump 5");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP5_SLEEP_PIN, HIGH);
      pump5.enableOutputs();
      pump5.move(pumpAmountMls * PUMP5_MLS_TO_STEPS);
      pump5.runToPosition();
      pump5.disableOutputs();
      digitalWrite(PUMP5_SLEEP_PIN, LOW);
      break;
    case 6:
      Serial.println("Pump 6");
      Serial.println(pumpAmountMls);
      digitalWrite(PUMP6_SLEEP_PIN, HIGH);
      pump6.disableOutputs();
      pump6.move(pumpAmountMls * PUMP6_MLS_TO_STEPS);
      pump6.runToPosition();
      pump6.enableOutputs();
      digitalWrite(PUMP6_SLEEP_PIN, LOW);
      break;
    default:
      break;
  }
}

void moveToCommandImpl(MyCommandParser::Argument *args, char *response) {
  moveToDrinkPos(args[0].asInt64);

  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void dispenseCommandImpl(MyCommandParser::Argument *args, char *response) {
  actuateDrinkServo(args[0].asInt64, args[1].asInt64);

  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void pumpCommandImpl(MyCommandParser::Argument *args, char *response) {
  pump(args[0].asInt64, args[1].asInt64);

  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;  
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}

// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};