/* Example code pulled from http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde
   Used to set the time and date on a Sparkfun DeadOn Real Time Clock breakout board using the DS3234.
   This sketch was used to set the clock to mountain standard time (MST) for eventual use in a data logging application. 
   The only change between this sketch and Sparkfun's example was to:
     - change the values in the SetTimeDate function call.
     - add some comments to the code.
     - change SPI mode from SPI mode 1 to SPI mode 3.  ... think all this has to do with polarity and phase of the clock signal. 
       Differences between modes are fairly esoteric to me at this point.
     
    modified 4 July 2012 (America - Fuck Yeah!) by Matt Findley
    
    modified again 7 July 2012 to tweak with the RTC's control and alarm registers.  ... also inserts leading zeros, as needed, into the time/date string for minutes and seconds. 
*/

#include <SPI.h>

const int  cs=8; //chip select 

void setup() {
  Serial.begin(9600);
  RTC_init();
  //day(1-31), month(1-12), year(0-99), hour(0-23), minute(0-59), second(0-59)
  SetTimeDate(7,7,12,6,59,00); 
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

	  //set alarm and control registers 
	  digitalWrite(cs, LOW);  
          SPI.transfer(0x8B);         // send a command which contains the register address that we want to write to.  
                                      // We will start with the first register in the series of alarm2 programming registers.
          SPI.transfer(B00010101);    // 15 minutes
          SPI.transfer(B10000000);    // set mask on hour
          SPI.transfer(B10000000);    // set mask on day/date
  
          digitalWrite(cs, HIGH);     // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
          delay(10);
  
          digitalWrite(cs, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
          SPI.transfer(0x8E);         // address for the RTC's control register. 
          SPI.transfer(B00000110);    // set the interupt control and enable alarm 2.
          SPI.transfer(B00000000);    // set OSF to zero, disable the battery enabled 32 kHz signal, disable the 32 kHz signal.
          digitalWrite(cs, HIGH);     // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
          delay(10);
}
//=====================================
int SetTimeDate(int d, int mo, int y, int h, int mi, int s){ 
	int TimeDate [7]={s,mi,h,0,d,mo,y};
	for(int i=0; i<=6;i++){
		if(i==3)
			i++;
		int b= TimeDate[i]/10;
		int a= TimeDate[i]-b*10;
		if(i==2){
			if (b==2)
				b=B00000010;
			else if (b==1)
				b=B00000001;
		}	
		TimeDate[i]= a+(b<<4);
		  
		digitalWrite(cs, LOW);
		SPI.transfer(i+0x80); 
		SPI.transfer(TimeDate[i]);        
		digitalWrite(cs, HIGH);
  }
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
