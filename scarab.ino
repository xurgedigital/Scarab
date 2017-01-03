/*Scarab v1.0 for ATMega328p by ML. 6/25/15
* Github URL: https://github.com/morenointl/Scarab/blob/master/scarab.ino
* URL of diagrams and schematics
* URLs of references:
*/

//LIBRARIES USED
#include <EEPROMAnything.h>
#include <CapacitiveSensor.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <EEPROM.h>
//#include <LiquidCrystal.h>
#include <LiquidCrystal595.h>    // include the library
#include <PinChangeInt.h>
#include <PinChangeIntConfig.h>
//

//CREATE AN ARRAY OF 15 INTEGERS FOR USE IN SMOOTHING OUT THE SIGNAL FROM CAPSENSE
#define filterSamples 15 //taking 15 samples (odd numbers only)
int sensSmoothArray1 [filterSamples];
//

//CREATE VARIABLES FOR USE IN CODE AND SET THEIR INITIAL VALUES
//
//FIRST THE PIN ASSIGNMENTS (ATMEGA328P)
const int dischargePin = 19;//A5
const int hall2Pin = 17;//A3
const int offPin = 16;//A2 
const int chargePin = 12;//A0
const int redPin = 11;//A1
const int greenPin = 10;//D10
const int bluePin = 9;//D9
const int capSensePin = 8;//D8
const int capSourcePin = 7;//D7
const int speakerPin = 6;//D6
const int hall1Pin = 2;//INT 0 (D2)
//SET PINS FOR SERIAL LCD
LiquidCrystal595 lcd(0,1,4);     // datapin, latchpin, clockpin
//
//
//NEXT THE VARIABLES FOR USE IN CODE
volatile int f_wdt=1; //WATCHDOG TIMER FLAG
int capStatus = 0; //
int count = 0; //
int timer = 15; //THIS IS THE ORIGINAL TIMEOUT AFTER POWER UP
int filterCount = 0; //
int secondCount = 0; //THE ELAPSED TIME IN APPROXIMATE SECONDS
int chirpCount = 0; //NOT USED YET. DELETE?
long reading1 = 0; //THIS IS WHERE THE RAW CAPSENSE VALUE IS STORED. NOT SMOOTHED
long reading2 = 0; // NOT USED/ DELETE?
int smoothedVal = 0;
int addr_touch 0;
int addr_perimeter = 64;
//struct config_t
//{
unsigned int touchThreshold;
unsigned int perimeter;
//}configuration;
// = 11000;//ADJUST AS NECESSARY FOR SENSITIVTY (was 30000)
unsigned char byteArray[4];
int proximityDet = 120; //NOT USED. DELETE?
bool charged = false; //IS THE DEVICE CHARGED? YES OR NO
bool flippy = true; //USED TO DETERMINE FIRST OR SECOND PROX WARNING CHIRP
int reset = 0; //USED TO SIGNAL POWER OFF WHEN 1
int readOther = 0; //USED TO STORE VALUE OF HALL2 SENSOR
int addr =1; //address foe EEPROM
int toggle = 0;
//
//END OF INITIALIZING VARIABLES.
//
//
//CONFIGURE CAPACITANCE SENSING
CapacitiveSensor cs_7_8 = CapacitiveSensor(7,8);// >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.

//ISR
ISR(WDT_vect) //Interrupt Service Routine. Watchdog Interrupt Service. This is executed when watchdog times out.
{
if(f_wdt == 0)
{
f_wdt=1;
}
else
{
Serial.println("WDT Overrun!!!");//MAKE THIS AN AUDIO ERROR MSG
}
}

// WHAT TO DO IF INTERRUPTED VA HALL1 EFFECT SENSOR. 
//THIS MEANS THAT A MAGNETIC FIELD WAS DETECTED BY THE HALL1 
//AND THE VOLTAGE DROPPED TO 0 THUS ACTIVATING THE INTERRUPT WHEN FALLING.
void interrupted()
{
detachInterrupt(digitalPinToInterrupt(hall1Pin));//DISABLE INTERRUPT SENSING
delay(5);//DEBOUNCE
readOther = digitalRead(hall2Pin);//TAKE A READING FROM THE HALL2 SENSOR. IT IS POSITIONED REVERSED OF HALL1.
//
if (readOther == 0)//SIGNAL CONFIRMED. (SENSOR DROPS TO 0 WHEN ACTIVE) THIS MEANS BOTH THE HALL1 AND HALL2 SENSORS ARE ACTIVE. THE KEY IS IN.
{
//detachInterrupt(digitalPinToInterrupt(hall1Pin));//STOP INTERRUPT SENSING
reset = 1;//FLAG FOR POWER OFF
}
else //NO HALL2 ACTIVE. MIGHT BE BAD POSITIONING OF KEY OR ATTEMPT TO INFILTRATE. EITHER WAY RESUME NORMAL OPERATION.
{
attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING); //RESUME INTERRUPT SENSING
reset = 0; //FLAGGED TO REMAIN ON.
return;
}

}

void setup() 
{
//CONFIGURE PINS
//config OFF pin as output and ensure it is not active right away
pinMode(offPin, OUTPUT);
//and the rest of the pins
digitalWrite(offPin, LOW);
pinMode(chargePin, OUTPUT);
pinMode(dischargePin, OUTPUT);
pinMode(redPin, OUTPUT);
pinMode(greenPin, OUTPUT);
pinMode(bluePin, OUTPUT);
pinMode(speakerPin, OUTPUT);
pinMode(capSensePin, INPUT);
pinMode(capSourcePin, OUTPUT);
pinMode(offPin, OUTPUT);
pinMode(hall1Pin, INPUT);
pinMode(hall2Pin, INPUT);
//
//CONFIG CAPSENSE
//cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF); // turn off autocalibrate on channel 1 - just as an example
cs_7_8.set_CS_AutocaL_Millis(80); //SET TIMING FOR CAPSENSE
//GET CAPSENSE THRESHOLD FROM EEPROM
cli();//turn off interrupts
if (EEPROM.read (0) != 0xff){ // if value is oxff (256) means first time calibration so don't use the 256 value
	touchThreshold = 11000; //default value
	perimeter = 6500; // default value
}
else{
	EEPROM_readAnything(addr_touch,touchThreshold); //get value from EEPROM and set threshold value
	EEPROM_readAnything(addr_perimeter,perimeter);//get value from EEPROM and set perimeter value
}
//touchThreshold = EEPROM.read(addr);
sei();//turn on interrupts
/*** SETUP THE WDT ***/
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
   // In order to change WDE or the prescaler, we need to
   //set WDCE (This will allow updates for 4 clock cycles).
   //
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
  /* Enable the WD interrupt (note no reset). */
WDTCSR |= _BV(WDIE);
//
//ENABLE HALL SENSOR INTERRUPT
attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING);
//
//AUDIO INIT CONFIRM
//for(int i = 0; i < 2; i++){
digitalWrite(redPin, HIGH); // red
tone(speakerPin, 3136, 500);
delay(500);
digitalWrite(redPin, LOW);
setColor(0, 0, 0); // OFF
setColor(0, 255, 0); // green
tone(speakerPin, 3951, 500);
delay(500);
setColor(0, 0, 0); // OFF
setColor(0, 0, 255); // blue
tone(speakerPin, 4186, 500);
delay(500);
setColor(0,0,0);

//CONFIGURE LCD
lcd.begin(16,2);             // 16 characters, 2 rows

    lcd.clear();
    lcd.setCursor(1,0); //COLUMN 1, ROW 0 TO CENTER JUSTIFY
    lcd.print("Scarab Defense");
    lcd.setCursor(1,1); //COLUMN 0, ROW 1
    lcd.print("INIT COMPLETE");
//delay(250);
//lcd.clear();
}

void loop() { // put your main code here, to run repeatedly:
//cs_7_8.reset_CS_AutoCal(); //RECALIBRATE -- NOT USED
if (reset == 1)
{
shutDown();
}
//cs_7_18.reset_CS_AutoCal(); //RECALIBRATE
//
if(f_wdt == 1){ //THINGS TO DO IF AWAKE, WHICH IT IS WHENEVER IT IS, get it? So do this:

/* Don't forget to clear the flag. */
f_wdt = 0; //clear the watchdog timer flag.

reading1 = cs_7_8.capacitiveSensor(20); //get the reading from sensor 1
smoothedVal = digitalSmooth(reading1, sensSmoothArray1); // Run average through the smoothing algorithm. every sensor you use with digitalSmooth needs its own array.

//THE FOLLOWING IS TO SECTION OUT THE READINGS.
//TO ENTER CALIBRATION MODE, TOUCH VESSEL AND 
if(reading1 > touchThreshold){//contact or contact imminent.
if (timer > 0){ //if within XX seconds of powerup AS DETERMINED BY INITIALIZATION OF VARIABLE (AT FIRST, THEN SEE BELOW)
//TURN OFF INTERRUPT TO USE THE HALL EFFECT SENSORS to 
detachInterrupt(digitalPinToInterrupt(hall1Pin));//DISABLE INTERRUPT SENSING
//ENTERING CALIBRATION MODE
int leftHall = digitalRead(hall1Pin);
int rightHall = digitalRead(hall2Pin);
if (leftHall==0 && rightHall==0){	//little man is on the vessel
	
	chirpFunc(); //ACK enter CAL mode
    lcd.clear();
    lcd.setCursor(0,0); //COLUMN 4, ROW 0 CENTER JUSTIFY
    lcd.print("ENTERNG CAL MODE");
	lcd.setCursor(0,1);
	lcd.print("Remove Key   ");
	for(int cntdn=5; cntdn!==0; cntdn--){ //COUNT DOWN TO GIVE TIME TO EXIT
		lcd.setCursor(13,1);
		lcd.print(cntdn);
		setColor(255, 0, 0); // RED
		tone(speakerPin, 2637, 100);
		setColor(0,0,0);
		delay(1000);
	}
	tone(speakerPin, 2637, 200);//e
	tone(speakerPin, 3238, 300);//e //THIS NOTIFIES ENTERED CALIBRATION MODE
	
	//CHECK HALL SENSORS
	leftHall = digitalRead(hall1Pin);
	rightHall = digitalRead(hall2Pin);
	//CLEAR LCD SCREEN
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("CALIBRATION MODE");
	lcd.setCursor(9,1);
	lcd.print("Touch then NEXT>");
    //CALIBRATION MODE:
	//LITTLE MAN ON VESSEL EXITS OUT OF CAL MODE. ACTIVATE RIGHT HE SENSOR TO SELECT.
	while (leftHall==1 || rightHall==1){ //AS LONG AS LITTLE MAN IS NOT ON THE VESSEL...
	
	toggle=0;
	
	while (digitalRead(hall2Pin)==0){ //WHEN RIGHT SENSOR ACTIVATED TO CONTINUE CALIBRATION...
	toggle = 1; //toggle 1 means to enter calibration mode
	}
	
	if (toggle == 1){
		leftHall = digitalRead(hall1Pin); //CHECK IF LITTLE MAN IS ON VESSEL
		if (leftHall == 0){
			tone(speakerPin, 3238, 200);
			tone(speakerPin, 2637, 300); //THIS NOTIFIES EXIT CALIBRATION MODE
			break; //IF SO, GET OUT OF THIS LOOP AND EXIT.
		}
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print(WAIT...);

		for(int u; u<16; u++){
			reading1 = cs_7_8.capacitiveSensor(20); //get the reading from sensor 1
			smoothedVal = digitalSmooth(reading1, sensSmoothArray1);
		}
	
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print("TOUCH:  ");
		lcd.setCursor(8,0);
		lcd.print(reading1);
		
		
		while (digitalRead(hall1Pin)!=0){ //waiting for input
		
		if (digitalRead(hall2Pin)==0){
		break;
		}
		
		lcd.setCursor(0,1);
		lcd.print("<<CANCEL  NEXT>>");		
					
		}
	
		
	
	}
		leftHall = digitalRead(hall1Pin);
		rightHall = digitalRead(hall2Pin);
	
	}
	
	
}

//TURN ON INTERRUPT FOR NORMAL OPERATION
//
//
timer = 20; //reset the timer
capStatus = 1; //signal: holding the vessel and the standby timer is running
//break; //leave this if loop
}
else{
capStatus = 2; //unauthorized touch. signal alarm and taze cycle
}
}
else if(smoothedVal > perimeter){//disturbance detected, charge, sound alarm
if (timer > 0){ //if within xx seconds of powerup
timer = 20; //reset the timer
capStatus = 1; //signal: holding the vessel and the standby timer is running
//break; //leave this if loop
}
else{
capStatus = 3; //signal proximity detected
}
}

else if (timer <= 0) //TIMEOUT REACHED 
{
capStatus = 4;
}

//NOW LET'S DO THINGS WITH THE CAPSTATUS:
switch (capStatus){

case 0: //NOTHING HAPPENS HERE. NOTHING SHOULD HAPPEN HERE
break;
case 1:{
//holding the vessel and the standby timer is running. flash green every 10 seconds
//count++;
//displayValues();
if(count > 10)
{
setColor(0, 255, 0);
delay(400);
setColor(0, 0, 0);
count = 0;
}
else
{
count ++;
}

break;
}


case 2: //unauthorized touch. signal alarm and taze cycle
{
digitalWrite(chargePin, HIGH);

//sound alarm
for(int i = 0; i < 3; i++)
{
setColor(255, 0, 0); // RED
tone(speakerPin, 2637, 125);//e
delay(125);

setColor(0, 0, 255); // blue
tone(speakerPin, 4186, 125);//c2
delay(125);

setColor(255, 0, 0); // RED
tone(speakerPin, 2794, 125);//f
delay(125);

setColor(0, 0, 255); // blue
tone(speakerPin, 5274, 125);//e2
delay(125);
setColor(0, 0, 0); // OFF
//tone(speakerPin, 4186, 125);//c2
//delay(125);
//digitalWrite(chargePin, LOW);
}
delay(1000);
digitalWrite(chargePin, LOW);
//reading1 = 0;
//reading2 = 0;
charged = false;
reset = 0;
break;
}

case 3: //proximity detected. signal warning
{
//displayValues();

if (flippy == true)
{
//sound chirp first warning
setColor(255, 0, 0); // red
tone(speakerPin, 3520, 70);//A2
delay(200);
setColor(100, 255, 0); // orange
tone(speakerPin, 1760, 70);//A2
delay(200);
flippy = false;
}
else
{
//sound chirp second warning
setColor(255, 0, 0); // red
tone(speakerPin, 3520, 70);//A2
delay(200);
setColor(100, 255, 0); // orange
tone(speakerPin, 1760, 70);//A2
delay(200);
setColor(255, 0, 0); // red
tone(speakerPin, 3520, 70);//A2
delay(200);
setColor(100, 255, 0); // orange
tone(speakerPin, 1760, 70);//A2
delay(200);
flippy = true;
}

break;
}

case 4: //TIMEOUT REACHED
{
//displayValues();
if (charged == false)
{
//setColor(255, 255, 255); // white
digitalWrite(chargePin, HIGH);//charge on
//delay(5000);
//digitalWrite(chargePin, LOW);//charge off
charged = true;
setColor(255, 255, 255); // white
tone(speakerPin, 2217, 100);//C#2
//setColor(0, 0, 0);
delay(100);

//setColor(255, 255, 255); // white
tone(speakerPin, 2217, 100);//C#2
//setColor(0, 0, 0);
delay(100);

//setColor(255, 255, 255); // white
tone(speakerPin, 1109, 100);//C#
//setColor(0, 0, 0);
delay(100);

//setColor(255, 255, 255); // white
tone(speakerPin, 2217, 100);//C#2
setColor(0, 0, 0);
digitalWrite(chargePin, LOW);//charge off
delay(500);
digitalWrite(chargePin, HIGH);//charge on
delay(900); 
digitalWrite(chargePin, LOW);//charge off
delay(500);
digitalWrite(chargePin, HIGH);//charge on
delay(900); 
digitalWrite(chargePin, LOW);//charge off
delay(500);
digitalWrite(chargePin, HIGH);//charge on
delay(900); 
digitalWrite(chargePin, LOW);//charge off
}
//flash RED every 10 seconds to indicate charge status
else if(count > 10)
{
setColor(255, 0, 0);
digitalWrite(chargePin, HIGH);//charge on
delay(400);
setColor(0, 0, 0);
count = 0;
}
count ++;
timer = 1;
break;
}

default:
{
//possible error. restart.
setColor(0, 0, 255); // blue
tone(speakerPin, 1380, 60);//A2
delay(40);
setColor(0, 255, 0); // green
tone(speakerPin, 1380, 60);//A2
delay(60);
setColor(255, 0, 0); // red
tone(speakerPin, 1380, 120);//A2
delay(4000);
//reading = 0;
digitalWrite(offPin, HIGH);
//digitalWrite(rstPin, HIGH);
delay(500);
shutDown();
break;
}
//reading=0;
}//END OF SWITCH STATEMEMNT

capStatus = 0;
//so by this point nothing out of the ordinary has happened.
timer--; //count tdown timer
/* Don't forget to clear the flag. */
f_wdt = 0;
/* Re-enter sleep mode. */
enterSleep();
}// END OF "IF AWAKE" LOOP

else //IF NOT AWAKE --DO NOT USE
{
/* Do nothing. */
}
}

//SUBFUNCTIONS USED IN THIS PROGRAM:

void shutDown() //SHUTS DOWN DEVICE
{
// check the value again - since it takes some time to
// activate the interrupt routine, we get a clear signal. (REALLY NECESSARY??)

//delay(125);
//DISCHARGE DEVICE
digitalWrite(dischargePin, HIGH);
delay(100);
digitalWrite(dischargePin, LOW);
delay(100);
digitalWrite(dischargePin, HIGH);
delay(100);
digitalWrite(dischargePin, LOW);
delay(100);
//
///LED AND AUDIO POWER OFF INDICATION
digitalWrite(redPin, HIGH);
//setColor(255, 0, 0); // red
tone(speakerPin, 4200, 500);//c2
delay(500);
digitalWrite(redPin, LOW);
setColor(0, 0, 0); // OFF
setColor(0, 255, 0); // green
tone(speakerPin, 3200, 500);//f
delay(500);
setColor(0, 0, 0); // OFF
setColor(0, 0, 255); // blue
tone(speakerPin, 2200, 500);//e2
delay(500);
setColor(0, 0, 0); // OFF
//delay(125);
digitalWrite(offPin, HIGH);

return;
}
//
void enterSleep(void) //Enters the arduino into sleep mode.
{
set_sleep_mode(SLEEP_MODE_PWR_DOWN); /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
sleep_enable();

/* Now enter sleep mode. */
sleep_mode();

/* The program will continue from here after the WDT timeout*/
sleep_disable(); /* First thing to do is disable sleep. */

/* Re-enable the peripherals. */
power_all_enable();
}
//
void setColor(int red, int green, int blue)//FOR RGB LED
{
#ifdef COMMON_ANODE
red = 255 - red;
green = 255 - green;
blue = 255 - blue;
#endif
analogWrite(redPin, red);
analogWrite(greenPin, green);
analogWrite(bluePin, blue);
}
//


int digitalSmooth(int rawIn, int *sensSmoothArray){ // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
int j, k, temp, top, bottom;
long total;
static int i;
// static int raw[filterSamples];
static int sorted[filterSamples];
boolean done;

i = (i + 1) % filterSamples; // increment counter and roll over if necc. - % (modulo operator) rolls over variable
sensSmoothArray[i] = rawIn; // input new data into the oldest slot

// Serial.print("raw = ");

for (j=0; j<filterSamples; j++){
  sorted[j] = sensSmoothArray[j];
}

done = 0; // flag to know when we're done sorting
while(done != 1){ // simple swap sort, sorts numbers from lowest to highest
done = 1;
for (j = 0; j < (filterSamples - 1); j++){
if (sorted[j] > sorted[j + 1]){ // numbers are out of order - swap
temp = sorted[j + 1];
sorted [j+1] = sorted[j] ;
sorted [j] = temp;
done = 0;
}
}
}

// throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
bottom = max(((filterSamples * 15) / 100), 1);
top = min((((filterSamples * 85) / 100) + 1 ), (filterSamples - 1)); // the + 1 is to make up for asymmetry caused by integer rounding
k = 0;
total = 0;
for ( j = bottom; j< top; j++){
total += sorted[j]; // total remaining indices
k++;

}

return total / k; // divide by number of samples
}
int chirpFunc(){

setColor(0, 255, 0); // green
tone(speakerPin, 3183, 60);//A2
delay(40);
tone(speakerPin, 3183, 60);//A2
delay(40);
tone(speakerPin, 4136, 60);//A2
setColor(0, 0, 0); //off
return;

} 
