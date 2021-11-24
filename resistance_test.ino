/**
 * Security System Monitored Input Tester
 * By: Shane McIntosh
 * http://rexfault.net
 * 
 * This tool is used to quickly test and identify issues with a monitored input on a modern
 * Security panel that uses a typical 1k/2k or 1k/.5k for active/inactive state.
 * This tool is better than a multimeter because it allows a single technician to connect the device at the panel,
 * then go to the input device and test it and come back and see what happened which can't be done by a tech alone without additional tools
 * Please find the schematic on my site at http://rexfault.net
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RESISTOR_IN A0
#define GROUND_LEAD_IN 10
#define GROUND_LEAD_IN_2 A7
#define R1 2150 //2.2KOhm resistor metered at 2.1553
#define MODE_SWITCH_IN 11
#define MODE_BLUEBACK HIGH
#define MODE_BLACKBACK LOW

//Uncomment this define for Serial Debug Output
#define DEBUG_

int input_voltage = 0;
float resistance = 0.0;

//Defines and variables for the LCD 20x04 display using the LiquidCrystal_I2C library
//Don't actually needs to specify the pins since the nano only has one I2C bus but I include them here just for anyone wondering what pins they are
#define SCL 5
#define SCA 4 
LiquidCrystal_I2C outputDisplay(0x27,20,4);

float calculateResistance(float knownResistor, float vIn) {

    float Vout = 0;
    float R2 = 0;
    float buffer = 0;
    
    buffer = vIn * 5.0;
    Vout = (buffer)/1024.0;
    buffer = (5.0/Vout) - 1.0;
    R2 = knownResistor * buffer;
    return R2;
}

int readInput(float active, float inactive, int in_pin) {

      //We allow for some lee-way in regards to the valid resistances, it's pretty much never you get an exact value 
      //Typically you get a percentage of accuracy with resistors, we're allowing +/- 1% accuracy here
      float active_resistance_min = (active - ((0.01 * active) * 10)) + 215;
      float active_resistance_max = (active + ((0.01 * active) * 10)) + 235; //220ohms is a resistor in the circuit for detecting shorts
      float inactive_resistance_min = (inactive - ((0.01 * inactive)* 10)) +  215;
      float inactive_resistance_max = (inactive + ((0.01 * inactive) * 10)) + 235; //220ohms is a resistor in the circuit for detecting shorts   
      
      input_voltage = analogRead(in_pin);

      #ifdef DEBUG
      Serial.print("Input Voltage is " );
      Serial.println(input_voltage);
      #endif
      
      resistance = calculateResistance(R1, input_voltage);
      if ((resistance <= 300) && (resistance >= 1)) {
        return 3; //Input is Shorted or no resistor There is typically some resistance on the line just due to internal resistance of copper and resistors used in the circuit
        //We allow about 300 ohms for cable length if 300 ohms or less resistance then we got a short
      }
      

      #ifdef DEBUG
      Serial.print("Resistance is ");
      Serial.println(resistance);
      Serial.print("Active Range is ");
      Serial.print(active_resistance_min);
      Serial.print(" -> ");
      Serial.println(active_resistance_max);
      Serial.print("Inactive Range is ");
      Serial.print(inactive_resistance_min);
      Serial.print(" -> ");
      Serial.println(inactive_resistance_max);
      #endif
      
      
      /*if ((input_voltage <= 10) && ((isinf(resistance) || resistance > 20000))) {
        return 6;
      }*/
      
      if (input_voltage <= 100)  {
        return 4; //Input either Open or Grounded
      }
      else if ((resistance >= active_resistance_min) && (resistance <= active_resistance_max)) {
        return 1; //Input Active
      }
      else if ((resistance >= inactive_resistance_min) && (resistance <= inactive_resistance_max)) {
        return 2; //Input inactive
      }
      else {
        return 5;
      }
}

/**
 * Reads the ground check digital pin and returns the value.
 * 
 * Returns true if the pin is pulled low meaning there is a ground fault or false otherwise. 
 */
bool checkGroundFault(int digitalPin1, int digitalPin2) {
  bool groundCheck = digitalRead(digitalPin1);
  int groundCheck2 = analogRead(digitalPin2);

  #ifdef DEBUG
  Serial.print("Ground Check Pin is: ");
  Serial.println(groundCheck);
  Serial.print("Ground Check Pin2 is: ");
  Serial.println(groundCheck2);
  #endif
  
  if ((groundCheck == HIGH) && (groundCheck2 >87)) {
   
    #ifdef DEBUG
    Serial.println("Ground Check Returns False");
    #endif
    
    return false;
  }
  else {

    #ifdef DEBUG
    Serial.println("Ground Check Returns TRUE");
    #endif
    
    return true;
  }
}

/**
 * Updates the output based on the Resistor State
 * This function controls either the LEDs or LCD Display output
 * 
 * resistorState can be one of the following:
 * 1 - DC/Contact State Active 
 * 2 - DC/Contact State Inactive
 * 3 - DC/Contact Line Shorted (Read as 2 on the analog input)
 * 4 - DC/Contact Line Grounded/Open (Read as 0 on the analog Input)
 * 5 - Incorrect Resistance (not active/inactive values)
 */
void updateOutput(int resistorState, LiquidCrystal_I2C displayObj) {
  //Serial.print("Resistor State is: ");
  displayObj.setCursor(0,3);
  if (resistorState == 1) {
    //Serial.println("Active");
    displayObj.print("      Active    ");
  }
  else if (resistorState == 2) {
    //Serial.println("Inactive");
    displayObj.print("     Inactive      ");
  }
  else if (resistorState == 3) {
    //Serial.println("Shorted");
    displayObj.print("      Shorted      ");
  }
  else if (resistorState == 4) {
    //Serial.println("Open/Grounded");
    if (checkGroundFault(GROUND_LEAD_IN, GROUND_LEAD_IN_2) == true) {
      //Serial.println("Ground Fault");
      displayObj.print("   Ground Fault    ");
    }
    else {
      //Serial.println("Open Leads");
      displayObj.print("    Open Leads     ");
    }
    //displayObj.print("   Ground Fault   ");
    
  }
  else if (resistorState == 5) {
    //Serial.println("Invalid Resistor");
    displayObj.print("  Invalid Resistor    ");
  }
  else if (resistorState == 6) {

    displayObj.print("    Open Leads     ");
  }
}

void setup() {

  pinMode(MODE_SWITCH_IN, INPUT);
  pinMode(GROUND_LEAD_IN, INPUT_PULLUP); //Set to use pullup resistor, will be pulled low if there's a ground fault.
  pinMode(RESISTOR_IN, INPUT_PULLUP);
  //pinMode(GROUND_LEAD_IN_2, INPUT_PULLUP); //Set to use pullup resistor, will be pulled low if there's a ground fault. 
  #ifdef DEBUG
  Serial.begin(9600); //Only used in the non-LCD version of the app and for debugging
  #endif

  //Show the splash screen
  outputDisplay.init();
  outputDisplay.begin(20,4); //Initialize Display
  outputDisplay.backlight(); //Turn on the backlight
  outputDisplay.setCursor(0,0);
  outputDisplay.print("    Input Tester   ");
  outputDisplay.setCursor(0,1);
  outputDisplay.print("    Created by:    ");
  outputDisplay.setCursor(0,2);
  outputDisplay.print("   Shane McIntosh");
  delay(3000); //Wait 3 seconds after showing the splash screen;

  outputDisplay.home();
  outputDisplay.clear();
  outputDisplay.print("    Input Mode:    ");
  outputDisplay.setCursor(0,2);
  outputDisplay.print("   Input State:   ");
  
}

void loop() {

  bool mode = digitalRead(MODE_SWITCH_IN);
  int resState = 0;

  outputDisplay.setCursor(0,1);
  if (mode == MODE_BLUEBACK) {
    
    #ifdef DEBUG
    Serial.println("Mode is Blue Back");
    #endif

    outputDisplay.print("    Blue Back     ");
    resState = readInput(2000, 1000, RESISTOR_IN);
  }
  else if (mode == MODE_BLACKBACK) {

    #ifdef DEBUG
    Serial.println("Mode is Black Back");
    #endif
    
    outputDisplay.print("    Black Back    ");
    resState = readInput(500, 1000, RESISTOR_IN);
  }
  
  outputDisplay.setCursor(0,2);
  
  updateOutput(resState, outputDisplay);
  delay(500);
  
  #ifdef DEBUG
  delay(3000);
  #endif

}
