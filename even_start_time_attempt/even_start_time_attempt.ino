/* read minutes and figure out if it is an even 15 minute interval, then start.

... couldn't get it to break out of the while loop.

  ... for eventual incorporation into AvgPARCalc2.

  Matt Findley  7 July 2012

*/
#include <SPI.h>             // need this libary to communicate with the DS 3234 Real Time Clock (RTC). 

const int CHIP_SELECT_PIN = 8;   // digital pin 8 is used in the SPI communication protocol to let the RTC know who the boss is and when it needs to be communicating.

void setup(){
  Serial.begin(9600);  // begin a serial output stream for sending data back to the computer.
  pinMode(CHIP_SELECT_PIN,OUTPUT); // chip select
  // start the SPI library:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE3); // mode 3 works 

  Serial.println("let's see if it time to start yet");
  
  int minutes = ReadMinutes();  
  while((minutes % 2) > 0) {
    int minutes = ReadMinutes();
    Serial.println(minutes);
    delay(1000);
  }
}

void loop(){
Serial.println("Off to the races!!!");

}

int ReadMinutes() {
  int minutes;
  digitalWrite(CHIP_SELECT_PIN, LOW);
  SPI.transfer(0x01);  // address the register for reading minutes on the RTC
  unsigned int n = SPI.transfer(0x00);  // assign register value to n
  digitalWrite(CHIP_SELECT_PIN, HIGH);
  int a=n & B00001111;  // assign 'a' as the lower four bits of the byte.  This is the first digit of the minutes (0 thru 9).
  int b=(n & B01110000)>>4; // assign 'b' as the next higher three bits of the byte.  This is the second digit of the minutes (0 thru 5).
  minutes = (b*10) + a;  //combine a and b together to get the integer value of minutes.	
  return(minutes);
}

