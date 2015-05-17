/*
 ADDED 74HC595
 
 */

// SET PIN ASSIGNMENTS. These constants won't change
//First, the pins connecting to to the 74HC595 shift register:
const int latchPin = 1;     //Physical pin 6 connected to ST_CP latch pin (pin 12) of 74HC595
const int clockPin = 2;     //Physical pin 7 connected to SH_CP (pin 11) of 74HC595
const int dataPin = 4;      //Physical pin 3 connected to DS (pin 14) of 74HC595
//Then the pins 
const int voltsensePin = A3;   // physical pin 2 to an analog voltage output
const int chargePin = 0;    // physical pin 5 outputs to the charge circuit
//const int ledPin = 1;     // LED indicator
//const int dischargePin = 4; //Physical pin 3 discharge pin to relay. COMMENT OUT TEMP

// variables:
int sensorValue = 0;      // the sensor value
int sensorMin = 0;        // minimum sensor value (approx 1.8v)
int sensorMax = 250;      // maximum sensor value
unsigned long sysTime = 0;      //this is for keeping time during charge
unsigned long startTime = 0;
boolean set2zero = false;
unsigned long fuckCounter = 0;
unsigned long chargeCounter = 0;
const unsigned long maxTime = 15000; //the max number in sec*1k to wait for charge


void setup() {
  // turn on LED to signal the start of the calibration period:
  //pinMode(ledPin, OUTPUT);
  pinMode(voltsensePin, INPUT);
  pinMode(chargePin, OUTPUT);
  //pinMode(dischargePin, OUTPUT);
    //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  //BLINK ALL LEDS//
  blinkAll_2Bytes(10,100);
}
//MAIN PROGRAM//
void loop() {
  // read the voltage:
  //sensorValue = analogRead(voltsensePin);
  //Do this while the shock module is charging:
  while ((analogRead(voltsensePin)) <= sensorMax)  { //as long as voltage is less than max:
    digitalWrite(chargePin, HIGH); //send charge signal to the shock module.
    fuckCounter = getCount();
    if (fuckCounter >= maxTime) {  //...if max seconds have passed:
      set2zero = false; //reset the set to zero timer flag
      //DO SOMETHING HERE. TIME IS UP. TIME TO DETERMINE WHAT HAPPENED.
      //DID IT CHARGE?
      if ((analogRead(voltsensePin)) <= sensorMax)  { //if it it did not charge. MAYBE ERRORS FROM DIRECT READ AND SHOULD STORE VALUE IN VARIABE.
        //chargeError = true;
        blinkAll_2Bytes(3,500);
        delay(500);
        blinkAll_2Bytes(3,500);
        delay(500);
        break;
      }
      else{// IT DID CHARGE.
        blinkAll_2Bytes(5,150);
        delay(250);
        break;
      }
     break; ///leave the while loop to stop the charge.
    }
    else{  //otherwise:
      //digitalWrite(chargePin, HIGH); //send charge signal to the shock module.
      sensorValue = analogRead(voltsensePin); //get the new voltage reading.       
      //FOR TESTING OUTPUT THE VOLTAGE VALUE TO THE SHIFT REGISTER:
      // DISPLAY IN BINARY TO LEDs
      // take the latchPin low so 
      // the LEDs don't change while you're sending in bits:
      digitalWrite(latchPin, LOW);
      // shift out the bits:
      shiftOut(dataPin, clockPin, MSBFIRST, (analogRead(voltsensePin)));  
      //take the latch pin high so the LEDs will light up:
      digitalWrite(latchPin, HIGH);
      // pause before next value:
      delay(200);
    }
  }
  
  digitalWrite(chargePin, LOW);
  //blinkAll_2Bytes(5,150);
  //delay(250);
  //blinkAll_2Bytes(5,150);
  //DISPLAY THE VOLTAGE:
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, sensorValue);  
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  delay(10000);
  //DISPLAY THE VOLTAGE AGAIN:
  digitalWrite(chargePin, HIGH);
  delay(50);
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, (analogRead(voltsensePin)));  
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  digitalWrite(chargePin, LOW);
  delay(10000);
  //DISPLAY THE VOLTAGE AGAIN:
  digitalWrite(chargePin, HIGH);
  delay(50);
  digitalWrite(latchPin, LOW);
  // shift out the bits:
  shiftOut(dataPin, clockPin, MSBFIRST, (analogRead(voltsensePin)));  
  //take the latch pin high so the LEDs will light up:
  digitalWrite(latchPin, HIGH);
  digitalWrite(chargePin, LOW);
  delay(10000);
  //sensorValue analogRead(voltsensePin);
  //if ((analogRead(voltsensePin)) >= sensorMax){
  //turn off charge signal
  //digitalWrite(chargePin, LOW);
  //charge complete. flash ok is 10 blinks
  //  blinkAll_2Bytes(5,150);
      
    //discharge the capacitor
     // digitalWrite (dischargePin, HIGH);
    //  delay (500);
    //  digitalWrite (dischargePin, LOW);
    //}
  
  //else ///did not charge properly
  //{  
  //  blinkAll_2Bytes(3,250);
    //delay(500);
    //blinkAll_2Bytes(3,250);
    /*
    //discharge the capacitor
    digitalWrite (dischargePin, HIGH);
    delay (500);
    digitalWrite (dischargePin, LOW); */ 
    //did not charge. flash error.
    /*for (int count =1; count <=5; count++){ //flash the led 5x if error
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
    }
    */
  //}
}
//blinks both registers based on the number of times you want to 
//blink "n" and the pause between them "d"
//starts with a moment of darkness to make sure the first blink
//has its full visual effect.
void blinkAll_2Bytes(int n, int d) {
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
  digitalWrite(latchPin, 1);
  delay(200);
  for (int x = 0; x < n; x++) {
    digitalWrite(latchPin, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    shiftOut(dataPin, clockPin, MSBFIRST, 255);
    digitalWrite(latchPin, 1);
    delay(d);
    digitalWrite(latchPin, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    digitalWrite(latchPin, 1);
    delay(d);
  }
}

unsigned long getCount(){
  
  if (set2zero == true){
    sysTime = millis() % maxTime;
    chargeCounter = sysTime - startTime;
    return chargeCounter;
  }
  if (set2zero == false){
    startTime = (millis() % maxTime);
    chargeCounter = 0;
    set2zero = true;
    return chargeCounter;
  }
}
