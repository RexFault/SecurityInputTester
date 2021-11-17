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

#define RESISTOR_IN A0
#define GROUND_LEAD_IN 10
#define R1 2150 //2.2KOhm resistor metered at 2.1553
#define MODE_SWITCH_IN 11
#define MODE_BLUEBACK HIGH
#define MODE_BLACKBACK LOW

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
      float active_resistance_min = (active - ((0.01 * active) * 10)) + 225;
      float active_resistance_max = (active + ((0.01 * active) * 10)) + 225; //220ohms is a resistor in the circuit for detecting shorts
      float inactive_resistance_min = (inactive - ((0.01 * inactive)* 10)) +  225;
      float inactive_resistance_max = (inactive + ((0.01 * inactive) * 10)) + 225; //220ohms is a resistor in the circuit for detecting shorts   
      
      int input_voltage = analogRead(in_pin);

      /*
      Serial.println("In Voltage is " );
      Serial.println(input_voltage);
      */
      
      float resistance = calculateResistance(R1, input_voltage);
      if ((resistance <= 300) && (resistance >= 1)) {
        return 3; //Input is Shorted or no resistor There is typically some resistance on the line just due to internal resistance of copper and resistors used in the circuit
        //We allow about 300 ohms for cable length if 300 ohms or less resistance then we got a short
      }
      
      /*
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
      */
      
      
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
bool checkGroundFault(int digitalPin) {
  bool groundCheck = digitalRead(digitalPin);
  if (groundCheck == HIGH)
    return false;
  else
    return true;
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
void updateOutput(int resistorState) {
  Serial.print("Resistor State is: ");
  
  if (resistorState == 1) {
    Serial.println("Active");
  }
  else if (resistorState == 2) {
    Serial.println("Inactive");
  }
  else if (resistorState == 3) {
    Serial.println("Shorted");
  }
  else if (resistorState == 4) {
    //Serial.println("Open/Grounded");
    if (checkGroundFault(GROUND_LEAD_IN)) {
      Serial.println("Ground Fault");
    }
    else {
      Serial.println("Open Leads");
    }
  }
  else if (resistorState == 5) {
    Serial.println("Invalid Resistor");
  }
}

void setup() {

  pinMode(MODE_SWITCH_IN, INPUT);
  pinMode(GROUND_LEAD_IN, INPUT_PULLUP); //Set to use pullup resistor, will be pulled low if there's a ground fault.
  Serial.begin(9600);
  
}

void loop() {

  //int vIn = analogRead(RESISTOR_IN);
  //int unknownResistance = calculateResistance(R1, vIn);

  bool mode = digitalRead(MODE_SWITCH_IN);
  int resState = 0;

  if (mode == MODE_BLUEBACK) {
    Serial.println("Mode is Blue Back");
    resState = readInput(2000, 1000, RESISTOR_IN);
  }
  else if (mode == MODE_BLACKBACK) {
    Serial.println("Mode is Black Back");
    resState = readInput(500, 1000, RESISTOR_IN);
  }
  updateOutput(resState);

  delay(3000);

}
