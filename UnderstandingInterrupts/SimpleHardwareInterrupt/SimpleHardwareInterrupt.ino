/* SimpleHardwareInterrupt 

*/

const int ledPin = 13;
const int inputPin = 2;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(inputPin, INPUT);
  digitalWrite(inputPin, HIGH);
  attachInterrupt(1, doSomething, FALLING);
  

}

void loop() {
  
}



void doSomething() {
  Serial.println("Doing Something");
}

