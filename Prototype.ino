/**
 * DrinkMe.ino
 * Copyright (C) 2017 Jiri Dohnalek
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

// TODO - Consider the problem when the user does not places the cup
//        back onto the coaster, how to implement a reminder to notify
//        the user to return the cup on the coaster

#include "hx711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// General configuration variables
// -------------------------------

// The initial volume for the MP3 Player module
int initialVolume = 28;

// Provides access to the millis() override function
extern volatile unsigned long timer0_millis;


// MP3 Player configuration
// ------------------------
SoftwareSerial dftPlayerSoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini DFPlayer;
void printDetail(uint8_t type, int value);


// Newton laws of cooling
// ----------------------

// t0 = surrounding temperature
// t1 = initial liquid temperature
// t2 = desired temperature of the liquid
// t = time it takes to reach target temperature
const double k = 0.056;

// TODO add second temperature sensor to 
// monitor the room temperature to replace the 
// variable t0 with the data from the sensor
double t0=23.00, t1=95.00, t2=48.00;

// Arduino implementation
// t = (-1.0/k) * log((t2-t0)/(t1-t0))


// DS18B20 Settings
// ----------------
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


// Load Cell
// ---------
Hx711 scale(A1, A0);

// 1) Place nothing on the scale, then run the example code, 
// which means reset your arduino that runs the example 
// code above.
// 2) Record the output. that is the offset.
// 3) Place a stand weight, like 1kg(1000g), record the output as w.
// 4) Do the math
// ratio = (w - offset) / 1000
// ratio = (7754789 - 8277286) / 500 = -1045
// 5) Go to setup and edit the 
// scale.setOffset( <insert offset> );
// scale.setScale( <insert ratio> ); 
int loadCellOffset = 8277286;
int loadCellRatio = -1045;


// Miscelaneous
// ------------

// stores the temperature measurement from the 
// previous temperature reading for comparison
double previousTemperature = 0; // degree C

// The temperature have rissen above the defined
// temperature causes action start
double temperatureRise = 1.5; // degree C

// The weight on the platform is above threshold
int triggerWeight = 300; // grams

// The minimum weight on the platform after which
// the platform is considerate as empty
int minimumWeightOnPlatform = 50; // grams

// Target cup weight reduction in percent to 
// trigger the end of the subroutine
float cupWeightReduction = 0.3;


// --------------
// Setup Function 
// --------------

void setup() {
  
  // Start communicatio over serial 
  Serial.begin(115200);
  dftPlayerSoftwareSerial.begin(9600);
 
  // Start the communication with the MP3 Player
  // -------------------------------------------
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  if (!DFPlayer.begin(dftPlayerSoftwareSerial)) {  
    Serial.println(F("Unable to begin DFPlayer"));
    while(true);
  }
  Serial.println(F("DFPlayer Mini Active."));
  DFPlayer.volume(initialVolume);
  Serial.println(F("Setting up device DS18B20 and Load Cell"));

  // Start DS18B20
  // -------------
  sensors.begin();

  // read initial temperature
  sensors.requestTemperatures();
  previousTemperature = (double)sensors.getTempCByIndex(0);
  Serial.println(F("DS18B20 ready"));

  // Read Load Cell
  // --------------
  scale.setOffset(loadCellOffset);
  scale.setScale(loadCellRatio);
  Serial.println(F("Load Cell ready"));

}

// --------------
// Loop Function 
// --------------

void loop() {

  // --------------
  // Read Load Cell
  
  int cupWeight = (int)round(scale.getGram());

  Serial.print(F("weight="));
  Serial.print(cupWeight);
  Serial.println(F("g"));

  // Read DS18B20
  // ------------

  // Send the command to get temperature
  sensors.requestTemperatures();
  double currentTemp = (double)sensors.getTempCByIndex(0);
    
  //--------------------------------
  // place the code to react on 
  // temperature change between tags

  double temperatureDifference = currentTemp - previousTemperature;

  Serial.print(F("temperature (diff.)="));
  Serial.print(temperatureDifference);
  Serial.println(F("C"));

  Serial.print(F("temperature (act.)="));
  Serial.print(currentTemp);
  Serial.println(F("C"));
  Serial.println(F("-------"));


  // Threshold to trigger the start of the subroutines
  if(cupWeight > triggerWeight){
    
    // If there is a rise in temperature, then the hot drink
    // sits on top of the platform start warm drink subroutine
    if(temperatureDifference > temperatureRise){
      Serial.println(F("Started warm drink subroutine"));
      warmDrinkSubRoutine();        
    }
    // Otherwise drink is cold, start cold subroutine 
    else{
      Serial.println(F("Started cold drink subroutine"));
      coldDrinkSubRoutine();
    }
  }

  //--------------------------------
  previousTemperature = currentTemp;

  // In order to save power run the device one time every
  // x times every minute.
  resetMillisAndDelay(60000); 
}

/**
 * Handle the placement of the cold drink on the platform 
 * including all the subsequent actions
 */
void warmDrinkSubRoutine(){

  int cupWeight = (int)round(scale.getGram());

  // The cup will loose on weight as the person drink from the cup
  // the target reduction of the cup is to decrease the weight by 40%
  int targetCupWeight = (int)(cupWeight * (1-cupWeightReduction));

  // calculate the delay until the drink will be in the 
  // ideal temperature according to newton laws of cooling
  int calculatedDelay = (int) ((-1.0/k) * log((t2-t0)/(t1-t0)))* 1000;
  resetMillisAndDelay(calculatedDelay);

  // Start the cold subroutine with initial reminder to drink

  while(true){

    // Assuming if the drink is cold it is reasonable to assume that a person 
    // should drink every 15 min until the cup until the target is met
    // 15 * 60 * 1000 
    resetMillisAndDelay(900000);

    // Start the subroutine to remind the person to drink
    // by playing the sound from the MP3 player and checking if the person
    // have picked up the cup from the platform
   
    while(true){

      // Play the sound before voice command
      DFPlayer.play(1);
      resetMillisAndDelay(6000);
      DFPlayer.stop();
      
      // Play the sound of the voice command
      // to remind the user to drink
      //DFPlayer.play(2);
      //delay(6000);
      //DFPlayer.stop();

      // Wait small amount of time for the user
      // to pick up the cup from the platform
      resetMillisAndDelay(3000);

      // Measure the weight of the cup 
      // The user have picked up the cup from the platform if the 
      // weight recorded on the platform is less than 50g
      if((int)scale.getGram() < minimumWeightOnPlatform){
        break;
      }
      
    }

    // update the cup weight
    cupWeight = (int)round(scale.getGram());
    
    // stop the subroutine when the person have met the required target 
    // but the cup is still on the the coaster    
    if(cupWeight <= targetCupWeight && cupWeight > minimumWeightOnPlatform){
      return;
    }
  }

}


void coldDrinkSubRoutine(){
  
  // Update the weight of the cup
  int cupWeight = (int)round(scale.getGram());

  // The cup will loose on weight as the person drink from the cup
  // the target reduction of the cup is to decrease the weight by 40%
  int targetCupWeight = (int)(cupWeight * (1-cupWeightReduction));

  Serial.print(F("Target cup weight: "));
  Serial.println(targetCupWeight);

  while(true){

    // Update the weight of the cup
    cupWeight = (int)round(scale.getGram());
    Serial.print(F("Last measured weight: "));
    Serial.println(cupWeight);
    
    // The user did not returned the cup onto the platform
    handleEmptyPlatform(cupWeight);
    
    // Stop the subroutine when the person have met the required target but 
    // the cup is still on the the coaster    
    if(cupWeight <= targetCupWeight && cupWeight > minimumWeightOnPlatform){
      Serial.println(F("End of cold drinking sequence"));
      return;
    }
   
    // Assuming, if the drink is cold the person should drink every 15 minutes
    // until the cup hits the target, 15 * 60 * 1000 = 900000 miliseconds
    Serial.println(F("Wait 15 minutes"));
    resetMillisAndDelay(900000);
    
    // Update the latest cup weight
    cupWeight = (int)round(scale.getGram());
    Serial.print(F("Last measured weight: "));
    Serial.println(cupWeight);
    
    // stop the subroutine when the person have met the required target but 
    // the cup is still on the the coaster    
    if(cupWeight <= targetCupWeight && cupWeight > minimumWeightOnPlatform){
      Serial.println(F("End of cold drinking sequence"));
      return;
    }

    while(true){

      Serial.println(F("Reminding the user to drink"));
      // Play the sound before voice command
      playTrack(1, 6000);
      
      // Play the sound of the voice command
      // to remind the user to drink
      //playTrack(1, 6000);

      Serial.println(F("Waiting for user to pick up the cup"));
      // Wait small amount of time for the user
      // to pick up the cup from the platform
      resetMillisAndDelay(3000);

      // Measure the weight of the cup 
      // The user have picked up the cup from the platform if the 
      // weight recorded on the platform is less than 50g
      cupWeight = (int)round(scale.getGram());
      Serial.print(F("Last measured weight: "));
      Serial.println(cupWeight);
      if(cupWeight < 50){
        Serial.println(F("Drinking detected..."));
        break;
      }
    }
  }
  
}


/*
 * Play track from the MP3 Player
 * @param tn, track number
 * @param l, how long to play the track
 */
void playTrack(int tn, int l){
  DFPlayer.play(tn);
  resetMillisAndDelay(l);
  DFPlayer.stop();
}


/**
 * The person have probably taken the cup but have not returned it onto
 * the platform, notify the user to return the cup onto platform
 *  
 * @param weight, latest weight reading
 */
void handleEmptyPlatform(int weight){
  
  while(weight < minimumWeightOnPlatform){

    Serial.println(F("Reminding the user to drink"));
    // Play the sound to remind the user
    playTrack(1, 6000);
      
    weight = (int)round(scale.getGram());

    // User have placed the cup back onto the platform
    if(weight > minimumWeightOnPlatform){
      return;
    }
  }
  
}


/*
 * Set the millis value to avoid the the bug when
 * the millis runs over max int and resets itself to zero
 * preventing issues with long delays in the code
 * 
 * @param new_milis, new value that will override the milliseconds 
 */
void setMillis(unsigned long new_millis){
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis = new_millis;
  SREG = oldSREG;
}


/*
 * Includes the ability to reset the milis
 * and execute the delay 
 * @param milisecondDelay, how long to delay for
 */
void resetMillisAndDelay(int milisecondDelay){
  setMillis(0);
  delay(milisecondDelay);  
}

