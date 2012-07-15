/* Example code pulled from http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde
   Used to read the time and date on a Sparkfun DeadOn Real Time Clock breakout board using the DS3234. (disabled the code that was used to set the time).
   This sketch was used to read the clock after setting it with binary bit patterns (very clunkly, but helped understand the device). 
*/
#include <SPI.h>
const int  cs=8; //chip select 

void setup() {
  Serial.begin(9600);
  RTC_init();
  //day(1-31), month(1-12), year(0-99), hour(0-23), minute(0-59), second(0-59)
//  SetTimeDate(4,7,12,13,56,00); 
}

void loop() {
  Serial.println(ReadTimeDate());  // the ReadTimeDate function puts together a string with the time date, but the date convention is not US custom.
                                   // Rather, it is day/month/year.  ... also doesn't put a zero place holder in seconds or minutes less than 10.
  delay(1000);
}
//=====================================
int RTC_init(){ 
	  pinMode(cs,OUTPUT); // chip select
	  // start the SPI library:
	  SPI.begin();
	  SPI.setBitOrder(MSBFIRST); 
	  SPI.setDataMode(SPI_MODE3); // both mode 1 & 3 should work 
                                      // despite sparkfun's above comment, I was only able to get it work in Mode 3, the this part of the code is slightly different.
	  //set control register 
//	  digitalWrite(cs, LOW);  
//	  SPI.transfer(0x8E);
//	  SPI.transfer(0x60); //60= disable Osciallator and Battery SQ wave @1hz, temp compensation, Alarms disabled
//	  digitalWrite(cs, HIGH);
	  delay(10);
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
        if(TimeDate[1] < 10)
  	  temp.concat("0") ;
	temp.concat(TimeDate[1]);
	temp.concat(":") ;
        if(TimeDate[0] < 10)
  	  temp.concat("0") ;
	temp.concat(TimeDate[0]);
  return(temp);
}
