# DrinkMe
Drink Me - The talking coaster to support dementia patients. The idea to this started in March 2017 when at a university based hackathon Hacking Health. The idea was pitched by
Hillary Tetlow.

## Idea
One of the issues for people with dementia is the limited short term memory capacity. The memory problem can lead to forgetting to drink leading to reduced water intake and dehydration. The dehydration leads to further problems often leading to hospitalisation.

## Proposed Solution
The solution to this problem would be a talking coaster. The coaster would speak to the person reminding them to drink.

![Circuit](https://github.com/learn2develop/DrinkMe/blob/master/Hardware/Circuit.png "Circuit")

## Sound Files
+ 001.mp3 - Spoken text "Please put the drink back onto the coaster"
+ 002.mp3 - Spoken text "It is a long time since you had a drink, please take a drink"
+ 003.mp3 - Beep sound to alarm the person from http://soundbible.com/1252-Bleep.html

## Libraries Used
+ https://github.com/aguegu/ardulibs/tree/master/hx711
+ https://github.com/milesburton/Arduino-Temperature-Control-Library
+ https://github.com/DFRobot/DFRobotDFPlayerMini/archive/1.0.1.zip

## Hardware List
+ Arduino Nano
+ Load Cell 2kg
+ Dallas DS18B20 Temperature Sensor
+ DFPlayer MP3 Player
+ Smartphone loudspeaker 1W
+ SD Card

## Features
+ Measure weight
+ Measure temperature
+ Battery operated (Target of minimum of 12 hours)
+ Play MP3 reasonably loud
+ Distinguish between cold and warm drinks
