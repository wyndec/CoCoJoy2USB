 /* 
    CoCo Joystick2USB v2.1
    by: Paul Fiscarelli
    
    Copyright (c) 2020-2023, Paul Fiscarelli
    
    Adapted from Matthew Heironimus's ArduinoJoystickLibrary
    https://github.com/MHeironimus/ArduinoJoystickLibrary 

    MIT License

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
      
   CoCo Joystick Pinout:
   =====================
      CoCo Joystick  
           Pin                          
            1 ------------------ X-Axis Potentiometer
            2 ------------------ Y-Axis Potentiometer
            3 ------------------ GND
            4 ------------------ Joystick Button0
            5 ------------------ +5v
            6 ------------------ Joystick Button1
            
     
   CoCo Joystick to Pro Micro connections:
   =======================================
      CoCo Joystick  ------- Pro Micro
           Pin                  Pin        
            1 ------------------ A0
            2 ------------------ A1
            3 ------------------ 23
            4 ------------------ 2
            5 ------------------ 21
            6 ------------------ 3

   Other Pro Micro connections:
   =======================================
        Pro Micro ---------- Function
           Pin                 === 
            4 ------------ Fast-Fire Enable  (in)
            5 ------------ Stick-Switch LED  (out)
           A2 ------------ Fast-Fire Rate    (in)
           A3 ------------ Stick-Switch Sel. (in)
           
*/
 
#include <Joystick.h>

#define BUTTONS 2
#define ENABLE_ANALOG true
#define JOYSTICK_COUNT 2
#define JOYSTICK_SWITCH_THRESHOLD 50

// Setup input pins - map to Pro Micro

// analog x-and-y axis
int X1 = A0;
int Y1 = A1;

// joy button one inputs
int BP1 = 2;
int BR1 = 4;
int FF1 = A2;

// joy button two inputs
int BP2 = 3;
int BR2 = 0;
int FF2 = 0;

// left-right switch input and LED
int joySwitchLED = 5;
int joySwitchSelect = A3;

// global variables for left-right switching
unsigned long joySwitchTimer = 0;
bool joySwitchFlag = false;

/*
 * Joystick Definitions
 * (Joystick ID, Joystick Type,
 *  Button Count, Hat Switch Count,
 *  X, Y, and Z Axis,
 *  Rx, Ry, and Rz Axis,
 *  Rudder, Throttle,
 *  Accelerator, Brake, Steering)
 * 
 */

// description of joystick functions
Joystick_ Joystick[JOYSTICK_COUNT] = {
  Joystick_(0x02, JOYSTICK_TYPE_JOYSTICK, BUTTONS, 0, true, true, false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_JOYSTICK, BUTTONS, 0, true, true, false, false, false, false, false, false, false, false, false)
};

// define our Button class
class Button {
  public:
  int pin = 0;                        // button input
  int rswitch = 0;                    // rapid-fire switch input
  int ranalog = 0;                    // rapid-fire analog read
  int lastState = 0;                  // last state of button push
  int rFireState = 0;                 // last rapid-fire state
  unsigned long rFireCount = 0;       // rapid-fire countdown timer
  unsigned long rFireDelay = 0;       // rapid-fire delay
  
  Button(int bp, int rs, int rr) {
    pin = bp;
    rswitch = rs;
    ranalog = rr;
  }
};


// setup array of joystick buttons
Button Buttons[BUTTONS] ={Button(BP1,BR1,FF1), Button(BP2,BR2,FF2)};


// code setup
void setup() {
  for(int i=0 ; i<BUTTONS ;i++) {
    pinMode(Buttons[i].pin, INPUT_PULLUP);
    pinMode(Buttons[i].rswitch, INPUT_PULLUP);
  }

  Joystick[0].begin();
  Joystick[1].begin();
  if (ENABLE_ANALOG) {
    Joystick[0].setXAxisRange(-512, 512);
    Joystick[0].setYAxisRange(-512, 512);
    Joystick[1].setXAxisRange(-512, 512);
    Joystick[1].setYAxisRange(-512, 512);
  }

  pinMode(joySwitchLED, OUTPUT);
  pinMode(joySwitchSelect, INPUT_PULLUP);
  digitalWrite(joySwitchLED, HIGH);
}


/* joyStickState(): This routine is for handling current state of the joystick.
 *  This will set X-axis and Y-axis values based on analog reads of respective inputs.
 *  A separate call within the routine will also check the status of the joystick buttons.
 */
void joyStickState() {
  if (ENABLE_ANALOG) {

    // select which joystick output is enabled
    if (!joySwitchFlag){
      Joystick[0].setXAxis(analogRead(X1) - 512);
      Joystick[0].setYAxis(analogRead(Y1) - 512);
    }
    else {
      Joystick[1].setXAxis(analogRead(X1) - 512);
      Joystick[1].setYAxis(analogRead(Y1) - 512);
    }
   
  }
  // go check joystick buttons
  joyButtonStates();
}


/* joyStickSwitchState(): This routine is for detecting state of Left-Right momentary switch.
 *  The momentary switch allows toggling between left and right joystick outputs. An LED is also
 *  toggled to indicate which output is currently enabled.
 */
void joyStickSwitchState() {
  
  // check if momentary switch is down - if so, start timer
  if (!digitalRead(joySwitchSelect) and (joySwitchTimer == 0)){
    joySwitchTimer = millis();
  }
  else {
    joySwitchTimer = 0;
  }
  
  // check if momentary button is held for some time - toggle joystick selection and LED indicator if beyond threshold
  while (!digitalRead(joySwitchSelect)){
    if ((joySwitchTimer > 0) and ((millis() - joySwitchTimer) >= JOYSTICK_SWITCH_THRESHOLD)){
      clearJoyButtons();
      digitalWrite(joySwitchLED, joySwitchFlag);
      joySwitchFlag = !joySwitchFlag;
      joySwitchTimer = 0;
    }
  }
}


/* clearJoyButtons(): This routine clears button states of currently selected joystick.
 *  This routine is needed to clear any current button presses before switching output
 *  to other joystick.
 */
void clearJoyButtons() {
  
  int joystickNumber = 0;

  // select which joystick output is enabled
  if (!joySwitchFlag) {
    joystickNumber = 0;
  }
  else {
    joystickNumber = 1;
  }

  for (int i = 0; i < BUTTONS; i++) {
    Joystick[joystickNumber].setButton(i, LOW);
  }
}


/* joyButtonStates(): This routine is for handling current state of joystick buttons.
 *  This will set X-axis and Y-axis values based on analog reads of respective inputs.
 *  A separate call within the routine will also check the status of the joystick buttons.
 */
void joyButtonStates() {
  
  int joystickNumber = 0;

  // select which joystick output is enabled
  if (!joySwitchFlag) {
    joystickNumber = 0;
  }
  else {
    joystickNumber = 1;
  }
  
  // iterate through buttons
  for (int i = 0; i < BUTTONS; i++) {
    int rapidFire = !digitalRead(Buttons[i].rswitch);
    int currentState = !digitalRead(Buttons[i].pin);

    if ((currentState) && (rapidFire)) {
      int fireRate = analogRead(Buttons[i].ranalog);
      Buttons[i].rFireDelay = (fireRate / 2);
      
      if ((millis() - Buttons[i].rFireCount) > Buttons[i].rFireDelay) {
        Buttons[i].rFireCount = millis(); 
        Joystick[joystickNumber].setButton(i, !Buttons[i].rFireState);
        Buttons[i].rFireState = !Buttons[i].rFireState; 
      }
    }
    else if ((!currentState) && (Buttons[i].rFireState)) {
      Joystick[joystickNumber].setButton(i, currentState);
      Buttons[i].lastState = currentState;
      Buttons[i].rFireState = currentState;
    }
    else if (currentState != Buttons[i].lastState) {
      Joystick[joystickNumber].setButton(i, currentState);
      Buttons[i].lastState = currentState;
    }
  } 
}


/* main loop()
 *  
 */
void loop() {
  joyStickState();
  joyStickSwitchState();
  delay(50);
}
