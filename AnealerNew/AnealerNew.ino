#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <Wire.h>


//encoder knob pins
#define CLK 4
#define DT 5
#define SW 6

//TB6600 PINS (STEPPIN IS PUL+ ON TB6000)
//Stepper controller is set to 32 Microsteps
#define DIRPIN 8
#define STEPPIN 9
#define ENBRELAY 7

//Toggle switches
#define ENBSERVO 11
#define ENBMOTOR 12

//Controls servo speed (white wire on servo)
#define SERVOPIN 3


//define our other methods/function
void incrementIndex();
void menuControl();
void motorSwitchControl();
void servoSwitchControl();


//setup our Stepper 
AccelStepper stepper(1, STEPPIN, DIRPIN); // direction Digital 9 (CCW), pulses Digital 8 (CLK)

//setup our LCD - This assumes LCD is connected to I2C on SDA, SCL
LiquidCrystal_I2C lcd(0x27, 20, 4);


//stepper status lets us know if the stepper motor is currently moving
bool stepperStatus = false;

// timeToBounce is used to let us know when the motor needs to back up. 
bool itsTimeToBounce = false;

// indexInProcess let's us know that an index cycle is is process (indexing forward, bounce)
bool indexInProcess = false;

// when menu item we are currently viewing
int menuPosition = 0;

//these are used by the encoder knob code to track clock state and changes and encoder button presses
int currentStateCLK;
int previousStateCLK;
unsigned long lastButtonPress = 0;
unsigned long time_now = 0;



//unsigned long motor_status_timer = 0;

int directionToRun = 1;

//time in milliseconds to anneal the cartridge
int annealingTime = 3000;
int custAnealState = 3000;

long motordisplayTime = 1000;

int editmode = 0;
int scrnvalue;
int srrndec;

//motor movement settings
int indexsteps = 798;
int bouncesteps = 24;
int maxspeed = 1000;
int maxaccelleration = 10600;
int backspeed = 100;
int backdelay = 250;


char* menu[] = { "Anneal Time", "Rotation Speed", "Calibrate", "Bounce", "Select Preset" };
int currentMenuPosition = 0;


bool motorenabled = false;


//set up the servo speed and display speed
Servo myservo;
int servoSpeed = 180;
int servoDisplaySpeed = 29;

//initialize our presets
typedef struct {
  int index;
  String presetName;
  String displayValue;
  int value;
} AnnealingPreset;

const AnnealingPreset annealingPresets[]{
  {0, "Custom", "*", 3000},
  {1, "308 Win", "3.1", 3100},
  {2, "30-06", "3.2", 3200},
  {3, "6.5 Creed", "2.8", 2800},
  {4, "243 Win", "2.9", 2900},
  {5, "300 Win Mag", "3.4", 3400},
  {6, "556/223", "2.6", 2600},
  {7, "6.8SPC", "2.7", 2700},
  {8, "1sec", "1", 1000}
};
int presetCount = 9;
int currentlySelectedPreset = 0;

void setup() {

  //ATTACH OUR SERVO 
  myservo.attach(SERVOPIN);

  //SET OUR PINMODES FOR ENCODER
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  //SET OUR TOGGLES TO PULLUP
  pinMode(ENBSERVO, INPUT_PULLUP);
  pinMode(ENBMOTOR, INPUT_PULLUP);

  //ENABLE OUR STEPPER
  digitalWrite(ENBRELAY, HIGH);


  //SET UP OUR STEPPER
  stepper.setMaxSpeed(100); //SPEED = Steps / second
  stepper.setAcceleration(100); //ACCELERATION = Steps /(second)^2
  stepper.enableOutputs();
  motorenabled = true;


  //initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print(menu[currentMenuPosition]);

  //set our clocks 
  previousStateCLK = digitalRead(CLK);
  time_now = millis();
  //motor_status_timer = millis();
}

void loop() {

  menuControl();
  servoSwitchControl();



  currentStateCLK = digitalRead(CLK);


  stepperStatus = stepper.run();

  //forward index is done, time to bounce back
  if (indexInProcess == true && stepperStatus == false && itsTimeToBounce == false)
  {
    itsTimeToBounce = true;
    bounceBackward();

  }
  else if (indexInProcess == true && stepperStatus == false && itsTimeToBounce == true)  //bounce back is done reset
  {
    //in this stage, we are done indexing, our brass is under the flame
    itsTimeToBounce = false;
    indexInProcess = false;
    time_now = millis();

    //update the motor running display (remove plus sign)
    lcd.setCursor(15, 1);
    lcd.print(" ");
    lcd.setCursor(0, 0);
  }

  if (indexInProcess == false) {
    motorSwitchControl(stepperStatus);
    incrementIndex();

  }


}

void incrementIndex() {

  if (millis() - time_now > annealingTime) {
    indexInProcess = true;

    //update our screen to show plus sign signifying that motore should be running
    lcd.setCursor(15, 1);
    lcd.print("+");
    lcd.setCursor(0, 0);

    //motor_status_timer = millis();
    runmotor(indexsteps);
  }
  /*else {
    lcd.setCursor(15, 1);
    lcd.print(" ");
    lcd.setCursor(0, 0);
  }*/

  /*if (millis() - motor_status_timer > motordisplayTime) {
    lcd.setCursor(15, 1);
    lcd.print(" ");
    lcd.setCursor(0, 0);
    motor_status_timer = millis();
  }*/
}

void bounceBackward() {

  if (motorenabled == true )
  {
    stepper.enableOutputs();
    stepper.setMaxSpeed(maxspeed);
    stepper.setAcceleration(maxaccelleration);
    delay(backdelay);
    stepper.setMaxSpeed(backspeed);
    stepper.move(-1 * bouncesteps);
  }

}

void runmotor(int steps) {

  if (motorenabled == true)
  {
    stepper.enableOutputs();
    stepper.setMaxSpeed(maxspeed);
    stepper.setAcceleration(maxaccelleration);
    stepper.move(steps + bouncesteps);
  }


}

void servoSwitchControl() {
  if (digitalRead(ENBSERVO) == HIGH) {
    //90 is the "stop" message
    myservo.write(90);
  }
  else {
    myservo.write(servoSpeed);
  }
}

void motorSwitchControl(bool running) {

  if (!running) {
    if (digitalRead(ENBMOTOR) == HIGH) {
      digitalWrite(ENBRELAY, HIGH);
      motorenabled = false;

    }
    else {
      digitalWrite(ENBRELAY, LOW);
      motorenabled = true;

    }
  }
}

void menuControl() {

  //menu selection
  if (currentStateCLK != previousStateCLK && currentStateCLK == 1 && editmode == 0) {

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(DT) != currentStateCLK) {
      menuPosition--;
    }
    else {
      menuPosition++;
    }

    if (menuPosition > 4) {
      menuPosition = 0;
    }
    if (menuPosition < 0) {
      menuPosition = 4;
    }

    if (menuPosition != currentMenuPosition) {
      currentMenuPosition = menuPosition;
      lcd.clear();
      lcd.print(menu[currentMenuPosition]);
    }
  }



  //value changes
  if (currentStateCLK != previousStateCLK && currentStateCLK == 1 && editmode == 1) {
    switch (currentMenuPosition) {
    case 0: //annealing time menu
      if (digitalRead(DT) != currentStateCLK)
      {
        annealingTime += 100;
        currentlySelectedPreset = 0; //move to custom preset
      }
      else {
        annealingTime -= 100;
        currentlySelectedPreset = 0; //move to custom preset
      }
      scrnvalue = annealingTime / 1000;
      srrndec = annealingTime % 1000 / 100;
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(scrnvalue);
      lcd.print(".");
      lcd.print(srrndec);
      lcd.print(" sec");
      lcd.setCursor(0, 0);
      custAnealState = annealingTime;
      break;

    case 1: //servo speed
      if (digitalRead(DT) != currentStateCLK)
      {
        //if less than 9 we can go faster!
        if (servoDisplaySpeed < 30) {
          servoDisplaySpeed += 1;
          servoSpeed = servoDisplaySpeed + 105;
        }
      }
      else {
        if (servoDisplaySpeed > 1) {
          servoDisplaySpeed -= 1;
          servoSpeed = servoDisplaySpeed + 105;
        }
      }
      lcd.setCursor(0, 1);
      lcd.print("              ");
         lcd.setCursor(0, 1);
         lcd.print("Value: ");    
      lcd.print(servoDisplaySpeed);
     // lcd.print(servoSpeed);
      lcd.setCursor(0, 0);
      break;

    case 2: //calibration
      if (digitalRead(DT) != currentStateCLK)
      {
        directionToRun = -1;
      }
      else {
        directionToRun = 1;
      }
      stepper.enableOutputs();
      stepper.setAcceleration(100);
      stepper.move(directionToRun * 8);
      stepper.runToPosition();

      break;

    case 3: //bounce steps
      if (digitalRead(DT) != currentStateCLK)
      {
        bouncesteps += 1;
      }
      else {

        bouncesteps -= 1;
      }

      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(bouncesteps);
      lcd.setCursor(0, 0);


      break;
    case 4:
      //presets menu
      
      if (digitalRead(DT) != currentStateCLK)
      {
        //move forward through presets, if at the end, go back to beginning (circular menu)
        if (currentlySelectedPreset < presetCount-1)
        {
          currentlySelectedPreset += 1;
        }
        else {
          currentlySelectedPreset = 0;
        }
      }
      else {
        //move backwards through presets. If at beginning (0), jump to end (circular menu)
        if (currentlySelectedPreset ==0)
        {
          currentlySelectedPreset =presetCount-1;
        }
        else {
          currentlySelectedPreset -= 1;
        }
      }

      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
    //  lcd.print(annealingPresets[currentlySelectedPreset].presetName + "=" + annealingPresets[currentlySelectedPreset].displayValue);
     // lcd.setCursor(0, 0);
      if(currentlySelectedPreset!=0){
          annealingTime = annealingPresets[currentlySelectedPreset].value;
                lcd.print(annealingPresets[currentlySelectedPreset].presetName + "=" + annealingPresets[currentlySelectedPreset].displayValue);
               lcd.setCursor(0, 0);
      }else{
        annealingTime = custAnealState;
             scrnvalue = annealingTime / 1000;
      srrndec = annealingTime % 1000 / 100;
      lcd.setCursor(0, 1);
      lcd.print(annealingPresets[currentlySelectedPreset].presetName + "=");
      lcd.print(scrnvalue);
      lcd.print(".");
      lcd.print(srrndec);
      lcd.print(" sec");
            
            lcd.setCursor(0, 0);
      }

      //presets
      break;
    default:
      break;
    }

  }

  
  previousStateCLK = currentStateCLK;
  int btnState = digitalRead(SW);

  //these cases are for when the encoder button is pressed.
  if (btnState == LOW) {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again
    if (millis() - lastButtonPress > 50) {
      if (editmode == 1) {
        editmode = 0;
        lcd.clear();
        lcd.print(menu[currentMenuPosition]);
      }
      else {
        editmode = 1;
        switch (currentMenuPosition) {
        case 0: //dwell time menu
          scrnvalue = annealingTime / 1000;
          srrndec = annealingTime % 1000 / 100;
          lcd.setCursor(0, 1);
          lcd.print("Value: ");
          lcd.print(scrnvalue);
          lcd.print(".");
          lcd.print(srrndec);
          lcd.print(" sec");
          lcd.setCursor(0, 0);
          break;
        case 1:
          lcd.setCursor(0, 1);
          lcd.print("Value: ");
          lcd.print(servoDisplaySpeed);
          lcd.setCursor(0, 0);
          break;
        case 2:
          break;
        case 3:

          lcd.setCursor(0, 1);
          lcd.print("Value: ");
          lcd.print(bouncesteps);

          lcd.setCursor(0, 0);

          break;

        case 4:
           //annealer presets
           
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          if(currentlySelectedPreset!=0){
          annealingTime = annealingPresets[currentlySelectedPreset].value;
                lcd.print(annealingPresets[currentlySelectedPreset].presetName + "=" + annealingPresets[currentlySelectedPreset].displayValue);
               lcd.setCursor(0, 0);
      }else{
        annealingTime = custAnealState;
             scrnvalue = annealingTime / 1000;
      srrndec = annealingTime % 1000 / 100;
      lcd.setCursor(0, 1);
      lcd.print(annealingPresets[currentlySelectedPreset].presetName + "=");
      lcd.print(scrnvalue);
      lcd.print(".");
      lcd.print(srrndec);
      lcd.print(" sec");
            
            lcd.setCursor(0, 0);
      }

          break;
        default:
          break;
        }
      }
    }
    lastButtonPress = millis();
  }



}
