PARduino
========
PARduino measures and logs photosynthetically active radiation (PAR). 
It was designed for the Barnard Ecohydology Lab at the University of Colorado, Boulder  (http://instaar.colorado.edu/research/labs-groups/ecohydrology-laboratory/).

Introduction
------------
PAR is measured during environmental monitoring and research.  We developed PARduino because it has the potential to collect environmental data in research plots at a lower cost than what we currently use for data aquisition.

Design Files
------------
PARduino is shared with the community as open source hardware (OSHW).  We hope others find it useful and will extend, modify, and otherwise improve the design.  With that in mind, this GitHub repository contains design files associated with the PARduino:
 - The Arduino code used to poll the sensor and store the measurement on a microSD flash card.
 - Designs for a printed circuit board connecting all the components.
 - A bill of materials listing parts and tools required to create a PARduino.

Making PARduino
---------------
- Procure components using the bill of materials.
- Wire the components together on a breadboard or mount them on the PCB.
- Modify the software for the gainfactor of the EME amplifier and the calibration factor of the Li-COR sensor (each Li-COR sensor has a different calibration factor).  This entails changing these values at the beginning of the PARduino source code.
- Set the time and date on the real time clock.
- Format the microSD flash card.
- Load the software onto the microcontroller using FDTI USB to Serial breakout board.

Using PARduino
--------------

[need to discuss how to initiate data collection].

Design Rationale
----------------

We selected the electronic components with an eye towards ease of assembly - everything can be soldered to the PCB using through-hole soldering techniques.  Since the device will be deployed in remote research plots that lack easy access to line power, battery life was also a consideration in the design.  This drove the decision to use the 3.3 Volt and 8 megahertz version of Sparkfun's Arduino Pro Mini.  This Arduino board uses less power than an Arduino Uno (both because of voltage and because it lacks the onboard USB to serial converter of the Uno).  It also drove the design of the microcontroller software - remaining in low power sleep mode until the device is ready to make a measurement dramatically extends field deployment times.

We use a 6 volt sealed lead acid battery because is has the amp-hours (listed as 8.2Ah) for extended field deployment, because it has a voltage that is high enough to run the EME amplifier (not to mention availability - there was a BatteriesPlus in town), and because the battery, while large by standards for cell phones and other consumer electronic devices, is still much  smaller than the 12 V sealed lead acid marine batteries that are commonly deployed at remote research plots.

License
-------
The code and other design documents for PARduino are released under the Creative Commons Attribution-ShareAlike 3.0 United States License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/us/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
