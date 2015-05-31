/*
 * ScarabLock.c
 *
 * Created: 5/29/2015 7:49:50 PM
 *  Author: ML
 */ 
//INCLUDED HEADERS
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdlib.h>
// Following lines are essential to enable sleep mode
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

//#define _BV(BIT)   (1<<BIT)
volatile boolean f_wdt = 1;



// SET PIN ASSIGNMENTS. These constants won't change
//First, the pins connecting to to the 74HC595 shift register:
const int latchPin = 1;     //Physical pin 6 connected to ST_CP latch pin (pin 12) of 74HC595
const int clockPin = 2;     //Physical pin 7 connected to SH_CP (pin 11) of 74HC595
const int dataPin = 4;      //Physical pin 3 connected to DS (pin 14) of 74HC595
//Then the pins 
const int voltsensePin = A3;   // physical pin 2 to an analog voltage output
const int dualPin = 0;     // speaker and Q0-Q1 to hard pin 5 (P0)

// variables:
int sensorValue = 0;      // the sensor value
int sensorMin = 0;        // minimum sensor value (approx 1.8v)
int sensorMax = 250;      // maximum sensor value
unsigned long sysTime = 0;      //this is for keeping time during charge
unsigned long startTime = 0;
boolean set2zero = false;
boolean chargeStatus = false;
unsigned long fuckCounter = 0;
unsigned long chargeCounter = 0;
const unsigned long maxTime = 15000; //the max number in sec*1k to wait for charge
int watchdog_counter = 0;
byte chargeMode = 4; //00000100
//byte powerDown = 128;//10000000
byte discharge = 16;//00001000
//byte onVibSensor = 12;//00001100
byte Check = 1; //00000001
byte inputCode = 0; //00000000


void setup() {
	//REGARDING SLEEP MODE
	// CPU Sleep Modes
	// SM2 SM1 SM0 Sleep Mode
	// 0    0  0 Idle
	// 0    0  1 ADC Noise Reduction
	// 0    1  0 Power-down
	// 0    1  1 Power-save
	// 1    0  0 Reserved
	// 1    0  1 Reserved
	// 1    1  0 Standby(1)
	cbi( SMCR,SE );      // sleep enable, power down mode
	cbi( SMCR,SM0 );     // power down mode
	sbi( SMCR,SM1 );     // power down mode
	cbi( SMCR,SM2 );     // power down mode
	
	setup_watchdog(7);
	
	pinMode(dualPin, OUTPUT); //this is output for the speaker, then switch back to input
	pinMode(voltsensePin, INPUT); // analog input
	//pinMode(chargePin, OUTPUT);// charge signal output
	//set pins to output to control the shift register
	pinMode(latchPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	pinMode(dataPin, OUTPUT);
    //THEN PLAY POWER ON SONG - SHAVE AND HAIRCUT
	playShaveCut();
	pinMode(dualPin, INPUT); // switch pin from speaker out to interrupt input from 74hc595
	delay(200);
  
	//initialize interrupt
	GIMSK = 0b00100000;    // turns on pin change interrupts
	PCMSK = 0b0000001;    // turn on interrupts on pin PB0, hard pin 5
	sei(); // enables interrupt
}//SETUP COMPLETE
//****************************************************************
//****************************************************************
//****************************************************************

//MAIN PROGRAM//
void loop() {//MAIN PROG LOOP
  // wait 30 seconds of uninterrupted sleep
  
    
  while (inputCode == 0){ //START LOOP1. As long as there is no input detected from either the reed switch or the vib sensor...
	//ADCSRA &= ~(1<<ADEN); //Disable ADC, saves ~230uA
	//setup_watchdog(2); //Setup watchdog to go off after 1/3 sec
	//sleep_mode(); //Go to sleep! Wake up 1sec later and check system
	if (chargeStatus == true){
		//BLINK THE LED TO INDICATED SYSTEM ARMED
		digitalWrite(latchPin, LOW);
		shiftOut(dataPin, clockPin, MSBFIRST, 16); //TURN ON THE LED.
		digitalWrite(latchPin, HIGH);
		delay(500);
		digitalWrite(latchPin, LOW);
		shiftOut(dataPin, clockPin, MSBFIRST, 0); //TURN OFF THE LED.
		digitalWrite(latchPin, HIGH);
		delay(500);
	}
	if(watchdog_counter > 30) {//START LOOP2. after 30 seconds of uninterrupted sleep, TIME TO CHARGE THE CAPACITOR
		watchdog_counter = 0; //reset timer
		//start charging!
		//Do this while the shock module is charging:
		//send charge signal to 74hc595, Q4
		//ADCSRA |= (1<<ADEN); //Enable ADC to read voltage
		digitalWrite(latchPin, LOW);
		// shift out the bits:
		shiftOut(dataPin, clockPin, MSBFIRST, chargeMode); //charge signal out, and so...
		while ((analogRead(voltsensePin)) <= sensorMax)  { //START LOOP3. as long as voltage is less than max:
			
			playCharging(); //PLAY THE CHARGING NOTE
			
			digitalWrite(latchPin, HIGH);//take the latch pin high and keep it that way so the charge signal remains HIGH:
			fuckCounter = getCount(); //GET VALUE OF CHARGECOUNTER, LOAD TO FUCKCOUNTER
			if (fuckCounter >= maxTime) {  //START LOOP4...if max seconds have passed (TIMEOUT ERROR):
				set2zero = false; //reset the set to zero timer flag
				//DO SOMETHING HERE. TIME IS UP. TIME TO DETERMINE WHAT HAPPENED.
				//DID IT CHARGE?
				if ((analogRead(voltsensePin)) <= sensorMax)  { // START LOOP5. IF IT DID NOT CHARGE TO THE MAX VOLTAGE...
					//the set charge was not reached.
					//stop charging:
					shiftOut(dataPin, clockPin, MSBFIRST, 0); //charge signal out, and so...
					playChargeError();
					delay (500);
					playChargeError();
					delay(500);
					playChargeError();
					delay (500);
					
					//discharge the remaining voltage
					digitalWrite(latchPin, LOW); //
					// shift out the bits:
					shiftOut(dataPin, clockPin, MSBFIRST, discharge); //sends a 1 to Q5 - discharge signal
					while ((analogRead(voltsensePin)) > 0)  { //START LOOP6. DISCHARGE as long as voltage is more than zero:
						//take the latch pin high TO DISCHARGE THE CIRCUIT.
						digitalWrite(latchPin, HIGH); //send it
						chargeStatus = false;
					} //END LOOP6
					delay(500);
					//POWER DOWN
					powerDown();
					//the whole thing shuts down here.
					//break;
				}//END LOOP5.
				else{// START LOOP7. CHARGE SUCCESS!
					playChargeSuccess();
					delay(250);
					chargeStatus = true;
					break; //leave LOOP4.
				}//END LOOP7
					break; ///leave the LOOP3
			}//END LOOP4. CHARGE TIMEOUT HANDLING
		
		}//END LOOP3. as long as voltage is less than max
		//OK SO NOW THE CAPACITOR IS CHARGED.
		//PLAY CHARGE SUCCESS NOTE.
		chargeStatus = true;
		cli();//disable interrupts
		pinMode(dualPin,OUTPUT); //set the dualPin to output
		playChargeSuccess();
		delay(100);
		sei(); //enable interrupts
		//BLINK THE LED TO INDICATED SYSTEM ARMED
		digitalWrite(latchPin, LOW);
		shiftOut(dataPin, clockPin, MSBFIRST, 16); //TURN ON THE LED.
		digitalWrite(latchPin, HIGH);
		delay(500);
		digitalWrite(latchPin, LOW);
		shiftOut(dataPin, clockPin, MSBFIRST, 0); //TURN OFF THE LED.
		digitalWrite(latchPin, HIGH);
		delay(500);
		return;
	}//END OF LOOP2-- WHAT TO DO AFTER X SECONDS UNINTERRUPTED SLEEP.
	else{//LETS ACTIVATE Q0-Q1 TO SENSE INPUTS
		//enable Q0-Q1 of 74HC595 to check inputs
		digitalWrite(latchPin, LOW);
		// shift out the bits:
		shiftOut(dataPin, clockPin, MSBFIRST, 3);// sends 00000011 to the 74HC595.
		digitalWrite(latchPin, HIGH);
		delay(666); //delay to give time for sensing
	}
  }	//END OF LOOP 1 LEAVING THIS LOOP MEANS THERE WAS INPUT DETECTED.
		//ok so now it's out of the while loop, which means the charge voltage was reached before the time-out AND THERE MIGHT BE AN INPUT DETECTED.
		//stop charging.
	if (inputCode == 1) { //MEANS VIBRATION WAS DETECTED
		//RESET TIMER
		watchdog_counter = 0;
		//SOUND ALARM
		playAlarm();
		delay(250);  

	}//END OF VIBRATION DETECTED LOOP
	if (inputCode == 2) { //MEANS REED SWITCH WAS DETECTED
		//RESET TIMER
		watchdog_counter = 0;
		//DOWN DOWN SONG
		playPowerDown();
		delay(250); 
		//POWER DOWN
		powerDown();
		//the whole thing shuts down here.

	}//END OF REED SWITCH DETECTED LOOP
}// END OF MAIN PROGRAM LOOP



//***********************************************************
//***********************************************************
//FUNCTIONS//
//***********************************************************
//***********************************************************

//***********************************************************
// SEND OFF SIGNAL TO POLOLU THROUGH SHIFT REGISTER
void powerDown()	{ 
	//CONSIDER ADDING EEPROM VARIABLE FOR POWER OFF FLAG
	digitalWrite(latchPin, LOW); // LATCH OFF
	shiftOut(dataPin, clockPin, MSBFIRST, 128); //sends a 1 to Q7 (pololu off)
	digitalWrite(latchPin, HIGH);//take the latch pin high TO SEND THE NEW SIGNAL
}

//***********************************************************
//USED FOR COUNTING AFTER POWER ON TO TIME-OUT THE CHARGE SIGNAL
unsigned long getCount(){ //USED FOR COUNTING AFTER POWER ON TO TIME-OUT THE CHARGE SIGNAL
  
  if (set2zero == true){ //DEFAULT FALSE SO IT WILL VISIT HERE THE SECOND AND SUBSEQUENT TIMES
    sysTime = millis() % maxTime; // SYSTIME IS DEFAULT 0. GET THE MODULUS OF MILLIS/MAXTIME AND LOAD VALUE TO SYSTIME.
    chargeCounter = sysTime - startTime; //sysTime - starttime (default 0), load result to counter
    return chargeCounter; //RETURN VALUE
  }
  if (set2zero == false){ //FIRST RUN. DEFAULT IS 0.
    startTime = (millis() % maxTime); //LOAD VARIABLE STARTTIME WITH THE MODULUS OF MILLIS/MAXTIME.
    chargeCounter = 0;// SET COUNTER TO ZERO
    set2zero = true; // SET TO TRU SO NEXT RUNS ARE EXECUTED IN THE PREVIOUS LOOP
    return chargeCounter; // RETURN VALUE
  }
}

//***********************************************************
//GENERATE TONE (PARAMETERS (NOTE, OCTAVE, DURATION)
void TinyTone(unsigned char divisor, unsigned char octave, unsigned long duration)
{
	TCCR1 = 0x90 | (8-octave); // for 1MHz clock
	// TCCR1 = 0x90 | (11-octave); // for 8MHz clock
	OCR1C = divisor-1;         // set the OCR
	delay(duration);
	TCCR1 = 0x90;              // stop the counter
}

//***********************************************************
// Play Shave and a Haircut
void playShaveCut(void)
{
	TinyTone(Note_C, 5, 400);
	delay(125);
	TinyTone(Note_G, 4, 400);
	delay(25);
	TinyTone(Note_G, 4, 250);
	delay(25);
	TinyTone(Note_A, 4, 400);
	delay(50);
	TinyTone(Note_G, 4, 300);
	delay(250);
	TinyTone(Note_B, 4, 400);
	delay(25);
	TinyTone(Note_C, 5, 500);
}

//***********************************************************
// Play POWER DOWN SOUND
void playPowerDown(void)
{
	for (int yopp = 0, yopp <4, yopp++){
		TinyTone(Note_C, 5, 250);
		delay(250);
		TinyTone(Note_G, 4, 250);
		delay(250);
		TinyTone(Note_A, 4, 250);
		delay(250);
		TinyTone(Note_B, 4, 250);
		delay(250);
	}
	TinyTone(Note_C, 5, 500);
		delay(250);
}

//***********************************************************
// Play CHARGING SOUND
void playCharging(void)
{
	TinyTone(Note_E, 6, 200);
	delay(25);
	TinyTone(Note_C, 5, 200);
	delay(25);
}

//***********************************************************
// Play CHARGE ERROR SOUND
void playChargeError(void)
{
	TinyTone(Note_F, 5, 400);
	delay(250);
	TinyTone(Note_F, 4, 400);
	delay(25);
}

//***********************************************************
// Play CHARGE SUCCESFUL SOUND
void playChargeSuccess(void)
{
	for (int zip = 0, zip <6, zip++){
	TinyTone(Note_C, 5, 100);
	delay(10);
	TinyTone(Note_E, 5, 100);
	delay(10);
	TinyTone(Note_E, 5, 100);
	delay(10);
	}
}

//***********************************************************
// Play ALARM SOUND
void playAlarm(void)
{
	for int (zop = 0, zop >6, zop++){
		for (int zip = 4, zip <14, zip++){
			TinyTone(Note_G, zip, 100);
			delay(10);
		}
		delay(200);
	}
}

//***********************************************************
// SET SYSTEM INTO THE SLEEP STATE
// SYSTEM WAKES UP WHEN WATCHDOG IS TIMED OUT
void system_sleep() {

	cbi(ADCSRA,ADEN);                    // switch Analog to Digital converter OFF

	set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
	sleep_enable();

	sleep_mode();                        // System sleeps here

	sleep_disable();                     // System continues execution here when watchdog timed out
	sbi(ADCSRA,ADEN);                    // switch Analog to Digital converter ON

}

//****************************************************************
// SETUP WATCHDOG TIMER (0-9)
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
  byte bb;
  int ww;
  if (ii > 9 ) ii=9; //ENSURES NO VALUE GREATER THAN 9
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;
  //Serial.println(ww);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}

//***********************************************************
//INTERRUPT SERVICE ROUTINE
ISR(PCINT0_vect)  
{
	    for(j=0; j<50; j++)
	    delayMicroseconds(500);  //delay FOR 25000 uS to debounce
		
		Check=1; // starts check byte at 00000001
		for(j=0; j<2; j++){//going to check the first two pins of the 74hc595
			digitalWrite(latchPin, 0);
			shiftOut(dataPin, clockPin, MSBFIRST, Check);
			//shiftOut(dataPin, clockPin, MSBFIRST, 0);
			digitalWrite(latchPin, 1);
			delayMicroseconds(500);
			if(digitalRead(dualPin)==HIGH)
			bitWrite(inputCode, j, 1);
			//bitWrite(Output, j, 1);
			else
			bitWrite(inputCode, j, 0);
			Check = Check<<1; // shift the check bit from 0000001 to 00000010
			}
	//now we have the byte inputCode with value either 0000001=vibration detected,
	//or 00000010=reed switch detected.
}

//****************************************************************
// WATCHDOG INTERRUPT SERVICE / IS EXECUTED WHEN  WATCHDOG TIMED OUT
ISR(WDT_vect) {
	watchdog_counter++;
}
