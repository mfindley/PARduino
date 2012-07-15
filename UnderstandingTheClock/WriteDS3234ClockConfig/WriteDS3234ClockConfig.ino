/* Example code pulled from http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde
   Used to write configuration settings on a Sparkfun DeadOn Real Time Clock breakout board using the DS3234.
   This sketch was used to eventually configure the clock for eventual use in a data logging application. 
   
   sets the time in a very clunky way, but helps understand how the device works.
   sets the configuration.
*/
#include <SPI.h>
const int  cs=8; //chip select 

void setup() {
  Serial.begin(9600);  // so I can send the register's bit pattern through the USB's serial line to the PC 
  SPI.begin();  // start command for SPI.  used in Sparkfun's example code
  SPI.setBitOrder(MSBFIRST);  // configuration command used to communicate with the clock.
  SPI.setDataMode(SPI_MODE3); // both mode 1 & 3 should work 
                              // despite sparkfun's above comment, I was only able to get it work in Mode 3, the this part of the code is slightly different.
 //set control register 
  pinMode(cs, OUTPUT);        // configure the chip select pin as an output pin 
  digitalWrite(cs, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x80);         // send the first command which contains the register address that we want to write to.  
                              // We will start with the first register in Figure 1 of the data sheet.

  SPI.transfer(B00110000);  // set part of RTC to 30 seconds. 
                            //deconstruct the bit pattern:  
                                             // bit 7: can't change.  
                                             // bits 6,5,4: 10 seconds.  I set to 011 or decimal 3.  
                                             // bits 3,2,1,0: Seconds.  I set to 0000 or decimal 0.  Together it is "30".

// supposed to automatically advance to the next register, so the next on is 0x81 - used to set minutes - and then so on, and so on ...

  SPI.transfer(B00000101);  // set part of RTC to 5 minutes. 
                            //deconstruct the bit pattern:  
                                             // bit 7: can't change.  
                                             // bits 6,5,4: 10 minutes.  I set to 000 or decimal 0.  
                                             // bits 3,2,1,0: minutes.  I set to 0101 or decimal 5.  Together it is "05".

  SPI.transfer(B00100001);  // set part of RTC to 21 hours (aka 9 pm)
                            //deconstruct the bit pattern:  
                                             // bit 7: can't change.  
                                             // bit 6: flag for 24 hour clock or 12 hour clock.  
                                             // bit 5: 20 hour digit.  set to "1" for 20. 
                                             // bit 4: 10 hour digit.  set to "0" because time isn't in the teens.
                                             // bits 3,2,1,0: hour.  I set to 0001 or decimal 1.  Together it is "21" and a 24 hour format.

  SPI.transfer(B00000110);  // set part of RTC to day of week to 6 (I will use "1" for Sunday)
                            //deconstruct the bit pattern:  
                                             // bits 7,6,5,4,3: can't change.  
                                             // bits 2,1,0: day of week.  I set to 110 or decimal 6.

  SPI.transfer(B00000110);  // set part of RTC to the sixth day of the month. 
                            //deconstruct the bit pattern:  
                                             // bits 7&6: can't change.  
                                             // bits 5,4: 10 date.  I set to 00 or decimal 0.  
                                             // bits 3,2,1,0: date.  I set to 0110 or decimal 6.  Together it is "06".
                                             
  SPI.transfer(B00000111);  // set part of RTC to the seventh month (July). 
                            //deconstruct the bit pattern:  
                                             // bits 7: "Century" - not sure how this is used.
                                             // bits 6,5:  not used.
                                             // bits 4: 10 month.  I set to 0 or decimal 0.  
                                             // bits 3,2,1,0: month.  I set to 0111 or decimal 7.  Together it is "07".

  SPI.transfer(B00010010);  // set part of RTC to the year (2012). 
                            //deconstruct the bit pattern:  
                                             // bits 7,6,5,4: 10 year.  I set to 0001 or decimal "1"
                                             // bits 3,2,1,0:  year.  I set to 0010 or decimal "2"
                                               
  digitalWrite(cs, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(100);

  digitalWrite(cs, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8B);         // send a command which contains the register address that we want to write to.  
                              // We will start with the first register in the series of alarm2 programming registers.

  SPI.transfer(B00010101);   // 15 minutes
  SPI.transfer(B10000000);   // set mask on hour
  SPI.transfer(B10000000);   // set mask on day/date
  
  digitalWrite(cs, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(100);
  
  digitalWrite(cs, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8E);         // address for the RTC's control register. 
  SPI.transfer(B00000110);   // set the interupt control and enable alarm 2.
  SPI.transfer(B00000000);   // set OSF to zero, disable the battery enabled 32 kHz signal, disable the 32 kHz signal.

  digitalWrite(cs, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(100);


}

void loop() {
}

