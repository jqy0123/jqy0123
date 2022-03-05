//Servo Setup
  #include <Servo.h>
  Servo servoL;
  Servo servoR;  
  int servoPosL=0;
  int servoPosR=0;

//LCD Setup 
  #include <LiquidCrystal_I2C.h>
  #include <math.h>
  LiquidCrystal_I2C lcd(0x27, 20, 4);

//Pin Setup
//!!NOTE!! DO NOT CHANGE PIN NUMBERS FOR PINA/PINB DUE TO INTERRUPT
  const int PinA = 3; //Pin A on rotary encoder
  const int PinB = 4; //Pin B on rotary encoder
  const int PinSW = 7; //Switch pin on rotary encoder
  const int PinML = 5; //Pin connected to servo for selection A
  const int PinMR = 6; //Pin connected to servo for selection B

//Variable Setup
  int selection = 0; //Defines what selection the user is choosing on each menu screen
  int nSwitch = 1; //New state of rotary encoder switch (state being read in this iteration)
  int oSwitch = 1; //Old state of rotary encoder switch (read last iteration) (USE: to see if button is still being pushed or if it is a new push)
  int edit = 0; //Defines whether the user is changing the pressure/IE value using the rotary encoder or toggling through selections on screens 1 and 2
  int pressureL = 25; //Left Patient pressure
  int pressureR = 25; //Right Patient pressure
  int pressureV = 25; //Ventilator pressure
  int maxPressure; //Used to determine whether the left or right patient pressure is higher (USE: to reqyure vent pressure to be at least highest patient pressure)  
  volatile int virtualPosition = 50; //Variable used to read turning of the rotary encoder dial
  int lastVP = 50; //Compares to new position to determine if rotary encoder has been turned during this iteration
  int VP; //Converts virtual position to an integer to be used in comparison
  int IEI = 1; //Inspiratory component of the I:E ratio
  int IEE = 3; //Expiratory component of the I:E ratio
  int IEcounter = 0; //Counter to cycle between preset I:E ratios
  int IEIv = 1; // Ventilator inspiratory input for I:E ratio (DOES NOT CHANGE)
  float IEEv = 1; //Ventilator expiratory input for I:E ratio (CHANGES)
  int screenMode = 0; //Determines which screen the user is currently on, total of three screens (0 = pressure screen, 1 = IE screen, 1 = start screen)
  int selectMode = 0; //Allows the user to choose between starting the device (0), stopping the device (1), and going back to the info screen (2)
  int lastScreenMode = 0; //Checks to see if the last setting for the screen matches the current screen (USE: clears and displays new text on new screen setting)
  int motorMode = 0; //Determines whether to turn the motors on or not
  int timeSec; //Time in seconds * 10 
  int modulusTime; //Calculates place within the cycle of IE ratio
  int cycleTime; //Total time for one cycle
  float stage1; //Calculates length of first stage of the cycle
  float stage2; //Calculates length of second stage of the cycle
  float stage3; //Calculates length of third stage of cycle
  int motorPressureA; //Sets pressure when motors are started and will not change until device is started again, despite input changes
  int motorPressureB; //Sets pressure when motors are started and will not change until device is started again, despite input changes
  int motorIEE; //Sets IE ratio when motors are started and will not change until device is started again, despite input changes
  int motorDegreeL; //Calculates what degree the servo motors should turn to provide the corresponding pressure (CALIBRATION CURVE)
  int motorDegreeR; //Calculates what degree the servo motors should turn to provide the corresponding pressure (CALIBRATION CURVE)
  int closedPosR = 90; //Degree that servo motor needs to turn to be completely closed
  int closedPosL = 90; //Degree that servo motor needs to turn to be completely closed



void setup() {
  // put your setup code here, to run once:
    //Serial monitor
      Serial.begin(9600);
    //LCD screen initialization
      lcd.init(); 
      lcd.init(); 
      lcd.backlight();
    //LCD Pressure screen writing
      lcd.setCursor(0,0);
      lcd.print("Left Patient:");  
      lcd.setCursor(0,1); 
      lcd.print("Right Patient:"); 
      lcd.setCursor(15,0);
      lcd.print(pressureL);
      lcd.setCursor(15,1);
      lcd.print(pressureR);
      lcd.setCursor(0,2);
      lcd.print("Vent Pressure: ");
      lcd.setCursor(15,2);
      lcd.print(pressureV);
      lcd.setCursor(0,3);
      lcd.print("Next");     
    //Rotary encoder initialization
      pinMode (PinA, INPUT);
      pinMode (PinB, INPUT);
      pinMode (PinSW, INPUT_PULLUP);
    //Attach the interrupts
      attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);
    //Motor initialization
      pinMode (PinML, OUTPUT);
      pinMode (PinMR, OUTPUT);
      servoL.attach(PinML);
      servoR.attach(PinMR);
}


// ------------------------------------------------------------------
// INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT
// Set up for the interrupt pins that the rotary encoder uses 
// ------------------------------------------------------------------
void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(PinB) == LOW)
    {
      virtualPosition++ ; // Could be -5 or -10
    }
    else {
      virtualPosition-- ; // Could be +5 or +10
    }

    // Restrict value from 0 to +100
    //virtualPosition = min(100, max(0, virtualPosition));


  }
  // Keep track of when we were here last (no more than every 5ms)
  lastInterruptTime = interruptTime;
}

void loop() {  
  VP = (int) virtualPosition; //Converts the interrupt position to an integer for comparison
  nSwitch = digitalRead(PinSW); //Reading whether button is being pushed
  switch (screenMode) {
    case 0: //Pressure screen: allows user to set pressure for left and right patient as well as the pressure of the ventilator
      if (lastScreenMode != screenMode) { //PDisplays new screen text
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Left Patient:");  
        lcd.setCursor(0,1); 
        lcd.print("Right Patient:"); 
        lcd.setCursor(15,0);
        lcd.print(pressureL);
        lcd.setCursor(15,1);
        lcd.print(pressureR);
        lcd.setCursor(0,2);
        lcd.print("Vent Pressure: ");
        lcd.setCursor(15,2);
        lcd.print(pressureV);
        lcd.setCursor(0,3);
        lcd.print("Next");
        lastScreenMode = screenMode;
        selection = 0;
        edit = 0;
      }    
      if (nSwitch == 0 && oSwitch != nSwitch && selection < 3) { //If button is pushed, switch to editing the value or stop editing the value (depending on initical cond.)
        if (edit == 0) {
          edit = 1;
        }
        else {
          edit = 0;
        }
      }
      else if (nSwitch == 0 && oSwitch != nSwitch && selection == 3) { //If button is pushed when user is on "next" selection, move to next screen
        screenMode = 1;
      }

      switch (edit) { 
        case 0: //When rotary encoder dial is turned, change the selection value
          if (VP != lastVP) {
            if ((VP < lastVP) && (selection < 3) ) {
              selection = selection + 1;
            }
            else if ((VP < lastVP) && (selection == 3) ) {
              selection = 0;
            }
            if (VP > lastVP && selection > 0 ) {
              selection = selection - 1;
            }
            else if (VP > lastVP && selection == 0 ) {
              selection = 3;
            }
          }
          lcd.setCursor(0,selection);
          lcd.blink();
          break;
        case 1:  //When rotary enooder dial is turned, change the pressure values for the corresponding patient
          maxPressure = pressureL > pressureR;
          lcd.setCursor(15,selection);
          lcd.blink();
          if (VP != lastVP) {
            if (VP > lastVP) {
              switch (selection) {
                case 0:
                  if (pressureL > 0) {
                    pressureL = pressureL - 1;
                    lcd.setCursor(15,0);
                    lcd.print(pressureL); 
                    if (maxPressure == 1 && pressureV > pressureL) {
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);                        
                    } 
                  }                   
                  break;
                case 1:
                  if (pressureR > 0) {
                    pressureR = pressureR - 1;
                    lcd.setCursor(15,1);
                    lcd.print(pressureR); 
                    if (maxPressure == 0 && pressureV > pressureR) {
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);                        
                    }                     
                  }                    
                  break;
                case 2:
                  if (pressureV > 0) {
                    if (maxPressure == 1 && pressureV > pressureL) {
                      pressureV = pressureV - 1;
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);     
                    }
                    else if (maxPressure == 0 && pressureV > pressureR) {
                      pressureV = pressureV - 1;
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);                          
                    }
                  }                
                  break; 
                case 3: 
                  edit = 0;
                  break;
              }            
            }
            if (VP < lastVP) {
              switch (selection) {
                case 0:
                  if (pressureL < 50) {
                    pressureL = pressureL + 1;
                    lcd.setCursor(15,selection);
                    lcd.print(pressureL);  
                    if (maxPressure == 1 && pressureV < pressureL) {
                      pressureV = pressureL; 
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);                      
                    }
                  }               
                  break;
                case 1:
                  if (pressureR < 50) {
                    pressureR = pressureR + 1;
                    lcd.setCursor(15,selection);
                    lcd.print(pressureR); 
                    if (maxPressure == 0 && pressureV < pressureR) {
                      pressureV = pressureR;
                      lcd.setCursor(15,2);
                      lcd.print(pressureV);
                    }
                  }                 
                  break;
                case 2:
                  if (pressureV < 50) {
                    pressureV = pressureV + 1;
                    lcd.setCursor(15,selection);
                    lcd.print(pressureV);      
                  }        
                  break; 
              } 
            }
          }          
          break;  
      }    
    break;
  case 1: //IE Screen: displays IE ratio and selections, corresponding vent ratio needed, start menu selection, and back selection
    if (screenMode != lastScreenMode) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("I:E:");
      lcd.setCursor(11,0);
      lcd.print(IEI);
      lcd.print(":");
      lcd.print(IEE);
      lcd.setCursor(1,1);
      lcd.print("Vent I:E:");
      lcd.setCursor(11,1);
      lcd.print(IEIv);
      lcd.print(":");
      lcd.print(IEEv,0);
      lcd.setCursor(0,2);
      lcd.print("Start Menu");
      lcd.setCursor(0,3);
      lcd.print("Back");
      lastScreenMode = screenMode;
      selection = 0;
      edit = 0;
    }
    if (nSwitch == 0 && oSwitch != nSwitch) {
      switch (selection) {
        case 0:
          if (edit == 0) { //Switch to 'edit' mode for IE ratio
            edit = 1;
          }
          else {
            edit = 0;
          }
          break;
        case 2: //Selects start menu and switches to this menu
          screenMode = 2;
          break;
        case 3: //Selects 'back' and switches back to pressure menu
          screenMode = 0;
          break;
        }  
    }
    switch (edit) {
      case 0: 
        lcd.setCursor(0,selection);
        lcd.blink();
        if (VP != lastVP) { //Toggles between selections on IE screen
          if (VP < lastVP && selection == 0) {
            selection = 2;
          }
          else if (VP < lastVP && selection == 2) {
            selection = 3;
          }
          else if (VP < lastVP && selection == 3) {
            selection = 0;
          }
          else if (VP > lastVP && selection == 0) {
            selection = 3;
          }
          else if (VP > lastVP && selection == 2) {
            selection = 0;
          }
          else if (VP > lastVP && selection == 3) {
            selection = 2;
          }
        }
        break;
      case 1: //Allows user to toggle between the three preset IE ratios: 1:3, 1:4, 1:5
        lcd.setCursor(13,0);
        lcd.blink();
        if (VP != lastVP) {
          if ((VP < lastVP) && (IEcounter < 2) ) {
            IEcounter = IEcounter + 1;
          }
          else if ((VP < lastVP) && (IEcounter == 2) ) {
            IEcounter = 0;
          }
          if (VP > lastVP && IEcounter > 0 ) {
            IEcounter = IEcounter - 1;
          }
          else if (VP > lastVP && IEcounter == 0 ) {
            IEcounter = 2;
          }
        }
        switch (IEcounter) {
          case 0: 
            IEE = 3;
            IEEv = 1;
            lcd.setCursor(13,0);
            lcd.print(IEE);
            lcd.setCursor(13,1);
            lcd.print(IEEv,0);
            lcd.print("  ");
            break;
          case 1:
            IEE = 4;
            IEEv = 1.5;
            lcd.setCursor(13,0);
            lcd.print(IEE);
            lcd.setCursor(13,1);
            lcd.print(IEEv,1);          
            break;
          case 2:
            IEE = 5;
            IEEv = 2;
            lcd.setCursor(13,0);
            lcd.print(IEE);
            lcd.setCursor(13,1);
            lcd.print(IEEv,0);
            lcd.print("  ");          
            break;
        }
      break;
    }
    break;
  case 2: //Start Menu: shows start device, stop device, and back
    if (lastScreenMode != screenMode) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Start Device");
      lcd.setCursor(0,1);
      lcd.print("Stop Device");
      lcd.setCursor(0,2);
      lcd.print("Back");
      lastScreenMode = screenMode;
      selection = 0;
      edit = 0;
    }
    lcd.setCursor(0,selection);
    lcd.blink();
    if (VP != lastVP) {
      if (VP < lastVP) {
        switch (selection) {
          case 0:
            selection = 1;
            break;
          case 1:
            selection = 2;
            break;
          case 2:
            selection = 0;
            break;
        }
      }
      if (VP > lastVP) {
        switch (selection) {
          case 0:
            selection = 2;
            break;
          case 1:
            selection = 0;
            break;
          case 2:
            selection = 1;
            break;
        }
      }      
    }
    if (nSwitch == 0 && oSwitch != nSwitch) { //If button is pushed, make a change
      switch (selection) { //If start is pushed, set pressures and IE ratio and don't change this until device is started again and start motors
        case 0:
          motorMode = 1;
          screenMode = 0;
          motorPressureA = pressureL;
          motorPressureB = pressureR;
          motorIEE = IEE;
          break;
        case 1: //If stop is pushed, turn motors off
          motorMode = 0;
          screenMode = 0;
          break;
        case 2: //Back button pushed takes you back to previous screen (IE screen)
          screenMode = 1;
          break;
      }
    }
    break;
  }
timeSec = millis() / 100; 
cycleTime = 10 + (motorIEE * 10); //Length of one cycle
modulusTime = timeSec % cycleTime; //Remainder when dividing time by cycle time
switch (motorMode) {
  case 0:  //Do nothing if motors are started
    break;
  case 1: //If motors are activated
        motorPressureA = (double) motorPressureA; 
        motorPressureB  = (double) motorPressureB;
        float ventPressure = pressureV;
        float PdropL = motorPressureA/ventPressure;
        float PdropR = motorPressureB/ventPressure;
        int motorDegreeL = (99.859 * pow(PdropL, 3)) - (140.205 * pow(PdropL, 2)) - (49.655 * PdropL) + 90; //Calculating pressure to degree
        int motorDegreeR = (99.859 * pow(PdropR, 3)) - (140.205 * pow(PdropR, 2)) - (49.655 * PdropR) + 90; //Calculating pressure to degree
        cycleTime = (double) cycleTime;

        float stage1 = IEI;        // first patient insp
        float stage2 = IEI+IEEv;   // first patient exp
        float stage3 = 2*IEI+IEEv; // second patient insp

        if (modulusTime >= 0 && modulusTime < stage1*10) {
          servoL.write(closedPosL);
          servoR.write(motorDegreeR);  
        }
        else if (modulusTime >= stage1 && modulusTime < stage2*10) {
          servoL.write(closedPosL);
          servoR.write(closedPosR);
        }
        else if (modulusTime >= stage2 && modulusTime < stage3*10) {
          servoL.write(motorDegreeL);
          servoR.write(closedPosR);
        }
        else {
          servoL.write(closedPosL);
          servoR.write(closedPosR);
        }
        Serial.println(motorDegreeL);
        Serial.println(motorDegreeR);       
        Serial.println(PdropL);
        Serial.println(PdropR);
        Serial.println(stage1);       
        Serial.println(stage2);
        Serial.println(stage3);
        
        
    break;
}
lastVP = VP; //Turn last variables to new variables before starting new iteration
oSwitch = nSwitch; 
}
