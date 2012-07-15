/* Playing with hardware alarms on the DS3234.

   set up Alarm2 to fire every minute.  monitor the flags in the register and the logic on the interrupt line every 5 seconds.
   ... observe how the interrupt line falls from hi to low on the one minute mark.
   then reset the register.
   
   ... using a Sparkfun DeadOn RTC which is really just a DS3234 chip with a couple of other circuit elements on a break-out board
   ... also using an Arduino Uno    

   Matt Findley.  7 July 2012.
*/

#include <SPI.h>  // load the SPI library

const int cs=8; // use pin 8 as the chip select pin.
const int RTC_InterruptPin = 6;  // use pin 6 to detect interrupt signals on the DS3234's INT/SQW line.

byte DS3234Alarm2MinutesRegisterByte, DS3234Alarm2HoursRegisterByte;
byte DS3234Alarm2DayDateRegisterByte, DS3234ControlRegisterByte;
byte DS3234ControlStatusRegisterByte;

void setup() {
  Serial.begin(9600);  // open up a serial connection over the USB
  // configure the SPI connection.
  pinMode(cs, OUTPUT);
  pinMode(RTC_InterruptPin, INPUT);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE3); // Mode 3 should work 

  // read and output a few registers on the RTC just to 
  // make sure everything is configured the way we want.

  digitalWrite(cs, LOW);  
  SPI.transfer(0x0B); // address the Alarm2 Minutes Read Register. 
  DS3234Alarm2MinutesRegisterByte = SPI.transfer(0x00);
  DS3234Alarm2HoursRegisterByte = SPI.transfer(0x00);  
  DS3234Alarm2DayDateRegisterByte = SPI.transfer(0x00);  
  DS3234ControlRegisterByte = SPI.transfer(0x00);  
  DS3234ControlStatusRegisterByte = SPI.transfer(0x00); 
  digitalWrite(cs, HIGH);  
  delay(10); 

  Serial.println("register settings before setup:");
  Serial.print("alarm2 minutes: ");
  Serial.println(DS3234Alarm2MinutesRegisterByte, BIN);

  Serial.print("alarm2 hours: ");
  Serial.println(DS3234Alarm2HoursRegisterByte, BIN);

  Serial.print("alarm2 day/date: ");
  Serial.println(DS3234Alarm2DayDateRegisterByte, BIN);

  Serial.print("control register: ");
  Serial.println(DS3234ControlRegisterByte, BIN);

  Serial.print("control/status register: ");
  Serial.println(DS3234ControlStatusRegisterByte, BIN);

  // tweak with the Alarm2 config to alarm every two minutes

  digitalWrite(cs, LOW);  
  SPI.transfer(0x8B); // address the Alarm 2 Minutes Write Register. 
  SPI.transfer(B10000000); //minutes mask set at 1 and everything else set to zero.
  digitalWrite(cs, HIGH);  
  delay(10); 

  // clear the alarm flag

  digitalWrite(cs, LOW);
  SPI.transfer(0x8F);  // address the Control/Status Write Register.
  SPI.transfer(B11111101 & DS3234ControlStatusRegisterByte); //clear bit 2 using a bitwise AND operation.  retain all other bits in their original form.
  digitalWrite(cs, HIGH);  
  delay(10); 

  // now, check register settings all over again (should probably be a function to avoid duplicate lines of code).

  digitalWrite(cs, LOW);  
  SPI.transfer(0x0B); // address the Alarm 2 Minutes Read Register. 
  DS3234Alarm2MinutesRegisterByte = SPI.transfer(0x00);
  DS3234Alarm2HoursRegisterByte = SPI.transfer(0x00);  
  DS3234Alarm2DayDateRegisterByte = SPI.transfer(0x00);  
  DS3234ControlRegisterByte = SPI.transfer(0x00);  
  DS3234ControlStatusRegisterByte = SPI.transfer(0x00); 
  digitalWrite(cs, HIGH);  
  delay(10); 

  Serial.println("register settings after setup:");
  Serial.print("alarm2 minutes: ");
  Serial.println(DS3234Alarm2MinutesRegisterByte, BIN);

  Serial.print("alarm2 hours: ");
  Serial.println(DS3234Alarm2HoursRegisterByte, BIN);

  Serial.print("alarm2 day/date: ");
  Serial.println(DS3234Alarm2DayDateRegisterByte, BIN);

  Serial.print("control register: ");
  Serial.println(DS3234ControlRegisterByte, BIN);

  Serial.print("control/status register: ");
  Serial.println(DS3234ControlStatusRegisterByte, BIN);

}

void loop() {

  Serial.println();  // space between text blocks
  Serial.println(ReadTimeDate());
  Serial.println("reading alarm flag ... ");
  // read alarm2 flag from the DS3234 register. 
  digitalWrite(cs, LOW);
  SPI.transfer(0x0F);  // address the Control/Status Read Register.
  DS3234ControlStatusRegisterByte = SPI.transfer(0x00); //assign the register settings to the variable.
  digitalWrite(cs, HIGH);  
  delay(10); 
  // extract just bit 1 from the byte.
  Serial.print("Alarm2 Flag: ");
  Serial.println(bitRead(DS3234ControlStatusRegisterByte, 1)); // read and print just bit 1 (A2F) from the byte

  // read INT/SQW pin.
  Serial.println("reading interrupt pin ... ");
  int InterruptPinValue = digitalRead(RTC_InterruptPin);  // read input value on the RTC's interrupt line.
  if(InterruptPinValue == HIGH)
    {
      Serial.println("Interrupt pin high!");
    }
  else
    {
      Serial.println("Interrupt pin low!");
    }
  // clear flag as needed.
  if(bitRead(DS3234ControlStatusRegisterByte, 1) == 1) {
    Serial.println(ReadTimeDate());
    // clear the alarm flag
    digitalWrite(cs, LOW);
    SPI.transfer(0x8F);  // address the Control/Status Write Register.
    SPI.transfer(B11111101 & DS3234ControlStatusRegisterByte); //clear bit 2 using a bitwise AND operation.  retain all other bits in their original form.
    digitalWrite(cs, HIGH);  
    delay(10); 
  }


  delay(5000); // hold 5000 ms
 
}

//=====================================
String ReadTimeDate(){
	String temp;
	int TimeDate [7]; //second,minute,hour,null,day,month,year		
	for(int i=0; i<=6;i++){
		if(i==3)
			i++;
		digitalWrite(cs, LOW);
		SPI.transfer(i+0x00); 
		unsigned int n = SPI.transfer(0x00);        
		digitalWrite(cs, HIGH);
		int a=n & B00001111;    
		if(i==2){	
			int b=(n & B00110000)>>4; //24 hour mode
			if(b==B00000010)
				b=20;        
			else if(b==B00000001)
				b=10;
			TimeDate[i]=a+b;
		}
		else if(i==4){
			int b=(n & B00110000)>>4;
			TimeDate[i]=a+b*10;
		}
		else if(i==5){
			int b=(n & B00010000)>>4;
			TimeDate[i]=a+b*10;
		}
		else if(i==6){
			int b=(n & B11110000)>>4;
			TimeDate[i]=a+b*10;
		}
		else{	
			int b=(n & B01110000)>>4;
			TimeDate[i]=a+b*10;	
			}
	}
	temp.concat(TimeDate[4]);
	temp.concat("/") ;
	temp.concat(TimeDate[5]);
	temp.concat("/") ;
	temp.concat(TimeDate[6]);
	temp.concat("     ") ;
	temp.concat(TimeDate[2]);
	temp.concat(":") ;
	temp.concat(TimeDate[1]);
	temp.concat(":") ;
	temp.concat(TimeDate[0]);
  return(temp);
}
