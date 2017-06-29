/**
 * DrinkMeProject.ino
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

// TODO - Consider adding second DS18B20 temperature sensor to calculate
//        the time for the drink to cool with higher precision

// TODO - How to handle the gap between the drinking and putting it back
//        onto the coaster. How long? What is the procedure?

// TODO - Consider https://github.com/bogde/HX711 library with the ability
//        to send the HX711 chip to lower power


#include "hx711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"


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
double t0=23.00, t1=95.00, t2=48.00;

// Constant to use for the calculation
const double k = 0.056;

// Arduino implementation
// t -> time it takes to reach target temperature
// t = (-1.0/k) * log((t2-t0)/(t1-t0))


// DS18B20 configuration
// ----------------

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


// Load cell configuration
// -----------------------

Hx711 scale(A1, A0);

// Calibration process:
// https://github.com/aguegu/ardulibs/tree/master/hx711
long loadCellOffset = 8277286;
float loadCellRatio = -1045;


// Miscelaneous variables
// ----------------------


// Provides access to reset the millis() function
extern volatile unsigned long timer0_millis;


// The initial volume for the MP3 Player module
int initialVolume = 28;


// Stores the temperature measurement from the previous temperature reading
// for comparison
double previousTemperature = 0; // degree C


// The temperature have rissen above the defined temperature causes action start
double temperatureRise = 1.5; // degree C


// The weight on the platform is above threshold
int triggerWeight = 350; // grams


// The minimum weight on the platform after which the platform is considerate
// as empty
int minimumWeightOnPlatform = 50; // grams


// The cup will loose on weight as the person drink from the cup the target
// reduction of the cup is to decrease the weight by 30% which is reasonable
// portion of the liquid in the mug, or glass
float cupWeightReduction = 0.3;


// One cycle - check the weight and temperature determine if there is a
// weight on the platform
unsigned long oneCycle = 60000; // 60 seconds


// Assuming if the drink is cold it is reasonable to assume that a person
// should drink every 15 min until the cup until the target is met
// 15 * 60 * 1000
unsigned long delayBetweenColdDrinks = 900000;


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

  // Read initial temperature
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

    // If there is a rise in temperature, then the hot drink sits on top of the
    // platform start warm drink subroutine
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

  // In order to save power run the device one time every x times every minute.
  resetMillisAndDelay(oneCycle);
}

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

  }

}

/**
 * Handle the placement of the cold drink on the platform including all the
 * subsequent actions
 */
void coldDrinkSubRoutine(){

  // Update the weight of the cup
  int cupWeight = (int)round(scale.getGram());

  // Target weight to stop the subroutine
  int targetCupWeight = (int)(cupWeight * (1-cupWeightReduction));

  Serial.print(F("Target cup weight: "));
  Serial.println(targetCupWeight);

  while(true){

    // Get the most updated weight of the cup and also handle
    // when the user did not returned the cup onto the platform
    cupWeight = handleEmptyPlatform((int)round(scale.getGram()));

    Serial.print(F("Last measured weight: "));
    Serial.println(cupWeight);

    // Stop the subroutine when the person have met the required target but
    // the cup is still on the the coaster
    if(cupWeight <= targetCupWeight && cupWeight > minimumWeightOnPlatform){
      Serial.println(F("End of cold drinking sequence"));
      return;
    }

    Serial.println(F("Wait 15 minutes"));
	  // Delay the notification between the drinks
    resetMillisAndDelay(delayBetweenColdDrinks);

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
      DFPlayer.play(3);
      resetMillisAndDelay(3000);

      // Play the sound of the voice command
      // to remind the user to drink
      DFPlayer.play(2);
      resetMillisAndDelay(6000);
      DFPlayer.stop();

      Serial.println(F("Waiting for user to pick up the cup"));
      // Wait small amount of time for the user
      // to pick up the cup from the platform
      resetMillisAndDelay(5000);

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


/**
 * The person have probably taken the cup but have not returned it onto
 * the platform, notify the user to return the cup onto platform
 *
 * @param weight, latest weight reading
 */
int handleEmptyPlatform(int weight){

  // Let the user enjoy the drink and not to rush him
  resetMillisAndDelay(5000);

  // Check if the cup is on the platform
  while(weight < minimumWeightOnPlatform){

    Serial.println(F("Reminding user to put the drink back onto the coaster"));
    // Play the notification sound before the reminder
    DFPlayer.play(3);
    resetMillisAndDelay(3000);

    // Play the text to remind the user to put the cup back onto the coaster
    DFPlayer.play(1);
    resetMillisAndDelay(3000);

    // Stop the MP3 Player
    DFPlayer.stop();

    // Do not to rush the user to put the drink back onto the platform quickly
    // before reminding him again
    resetMillisAndDelay(5000);

    // Remeasure the weight
    weight = (int)round(scale.getGram());

    // User have placed the cup back onto the platform
    if(weight > minimumWeightOnPlatform){
      return weight;
    }
  }

  return weight;
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
void resetMillisAndDelay(unsigned long milisecondDelay){
  setMillis(0);
  delay(milisecondDelay);
}
