/*Scarab v1.0 for Arduino Nano by ML. 6/25/15
 * Github URL:
 * URL of diagrams and schematics
 * URLs of references:
 * 
 */
#include <CapacitiveSensor.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <LiquidCrystal.h>
#include <PinChangeInt.h>
#include <PinChangeIntConfig.h>

#define filterSamples   15 //taking 19 samples (odd numbers only)
int sensSmoothArray1 [filterSamples];
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 18);

 //declare pin names
 /*const int chargePin = 12; //this was LED pin assignments BEFORE capsense
 const int redPin = 11;
 const int greenPin = 10;
 const int bluePin = 9;
 const int speakerPin = 6;
 const int capSensePin = 8;
 */
 const int chargePin = 14;//A0
 const int redPin = 15;//A1
 const int greenPin = 10;//D10
 const int bluePin = 9;//D9
 const int speakerPin = 6;//D6
 const int capSensePin = 8;//D8
 const int capSourcePin = 7;//D7
 const int offPin = 16;//A2
 const int hall1Pin = 2;//INT 0 (D2)
 const int hall2Pin = 17;//A3
 //const int rstPin = 16;
 const int dischargePin = 20;//A6
 
 volatile int f_wdt=1;
 //const unsigned long maxTime = 15000; //the max number in sec*1k to wait BEFORE CHARGING
 //long reading = 0;
 //long total2 = 0;
 int capStatus = 0;
 int count = 0;
 int timer = 15; //THIS IS THE ORIGINAL TIMEOUT AFTER POWER UP
 int filterCount = 0;
 int secondCount = 0;
 //int chirp = 0;
 int chirpCount = 0;
 long reading1 = 0;
 long reading2 = 0;
 //long readingComb = 0;
 int smoothedVal = 0;
 int touchThreshold = 13000;//ADJUST AS NECESSARY FOR SENSITIVTY
 int proximityDet = 120;
 bool charged = false;
 bool flippy = true;
 int reset = 0;
 int readOther = 0;
 //CONFIGURE CAPACITANCE SENSING
 CapacitiveSensor   cs_7_8 = CapacitiveSensor(7,8);// >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.
//
ISR(WDT_vect) //Interrupt Service Routine. Watchdog Interrupt Service. This is executed when watchdog timed out.
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

// WHAT TO DO IF INTERRUPTED VA HALL EFFECT SENSOR
void interrupted()
{
  detachInterrupt(digitalPinToInterrupt(hall1Pin));//DISABLE INTERRUPT SENSING
  delay(5);//DEBOUNCE
  readOther = digitalRead(hall2Pin);//TAKE A READING
  //
  if (readOther == 0)//SIGNAL CONFIRMED. SENSOR DROP TO 0 WHEN ACTIVE
  {
    //detachInterrupt(digitalPinToInterrupt(hall1Pin));//STOP INTERRUPT SENSING
    reset = 1;//FLAG FOR POWER OFF
  }
  else
  {
    attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING);
    reset = 0;
    return;
  }
  
}

void shutDown() {
  // check the value again - since it takes some time to
  // activate the interrupt routine, we get a clear signal.
  
  delay(10);
  readOther = digitalRead(hall2Pin);//TAKE A READING
  if (readOther == 0)
  {
    reset = 0;
    /* USE ONLY WHEN PROTOTYPING WITH LCD
    
     lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shutting down...");
    lcd.setCursor(0,1);
    lcd.print("discharging cap");
    digitalWrite(dischargePin, HIGH);
    //delay(2000);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shutting down...");
    lcd.setCursor(0,1);
    lcd.print("good bye!"); 
    */
    //sound shutdown tone
    //for(int i = 0; i < 3; i++)
    //{
      //tone(speakerPin, i, 125);//
      //delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 4200, 125);//c2
      delay(125);
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 3200, 125);//f
      delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 2200, 125);//e2
      delay(125);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1200, 125);//c2
      delay(125);
      tone(speakerPin, 600, 125);
      //digitalWrite(chargePin, LOW);
      setColor(0, 0, 0);
    //}    
    delay(500);
    digitalWrite(offPin, HIGH);
  
}
  return;
}

void enterSleep(void) //Enters the arduino into sleep mode.
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}


 void setColor(int red, int green, int blue)
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

 void displayValues()  //show values on LCD screen
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("A:");
      lcd.print(reading1);
      lcd.setCursor(8,0);
      //lcd.print("Dl");
      //lcd.print(readingComb);
      lcd.setCursor(0,1);
      //lcd.print("B:");
      //lcd.print(reading2);
      lcd.setCursor(9,1);
      lcd.print("Sm");
      lcd.print(smoothedVal);
      if (charged == true)
      {
      lcd.setCursor(0,1);
      lcd.print("CHARGED!");  
      }
    }

void setup() {
  
  //setup OFF pin and ensure it is not active right away
  pinMode(offPin, OUTPUT);
  digitalWrite(offPin, LOW);

  // initiate LCD(number of columns and rows:) and serial port
  /*lcd.begin(16, 2);
  Serial.begin(9600);
  
  // Print a message to the LCD.
  
   //INDICATE INITIALIZATION SUCCESS
  
  lcd.clear();
  //lcd.setCursor(0, 0);
  lcd.print("SCARAB v1.0");
  lcd.setCursor(0, 1);
  lcd.print("by Manuel Labor");
  Serial.println("SCARAB v1.0");
  Serial.println("by Manuel Labor");
  delay(1000);
  //lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  Serial.println("Initializing...");
  */
  //CONFIGURE PINS
  pinMode(chargePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(capSensePin, INPUT);
  pinMode(capSourcePin, OUTPUT);
  //pinMode(offPin, OUTPUT);
  pinMode(hall1Pin, INPUT);
  pinMode(hall2Pin, INPUT);
  //cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  cs_7_8.set_CS_AutocaL_Millis(80); 
  
  /*** SETUP THE WDT ***/
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
   /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);

  //ENABLE HALL SENSOR INTERRUPT
  attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING);
  // Global Enable INT0 interrupt
  //GICR |= ( 1 < < INT0);
  // Signal change triggers interrupt
  //MCUCR |= ( 1 << ISC00);
  //MCUCR |= ( 0 << ISC01);
  //delay(1000);
  //lcd.clear();
  //lcd.print("Finished initialization");
  //Serial.println("Finished initialization");
    //AUDIO INIT CONFIRM
  for(int i = 0; i < 2; i++){
    setColor(255, 0, 0);  // red
    tone(speakerPin, 3136, 125);
    delay(125);
    setColor(0, 255, 0);  // green
    tone(speakerPin, 3951, 125);
    delay(125);
    setColor(0, 0, 255);  // blue
    tone(speakerPin, 4186, 125);
    delay(125);
  setColor(0,0,0);
  
}
  delay(1000);
  //lcd.clear();
}

void loop() { // put your main code here, to run repeatedly:  
  //cs_7_8.reset_CS_AutoCal(); //RECALIBRATE
if (reset == 1)
{
  shutDown();
}
//cs_7_18.reset_CS_AutoCal(); //RECALIBRATE  
    //
if(f_wdt == 1){ //THINGS TO DO IF AWAKE, WHICH IT IS WHENEVER IT IS, get it? So do this:
  
  /* Don't forget to clear the flag. */
  f_wdt = 0; //clear the watchdog timer flag. 
  
  reading1 =  cs_7_8.capacitiveSensor(20); //get the reading from sensor 1
  smoothedVal = digitalSmooth(reading1, sensSmoothArray1);  // Run average through the smoothing algorithm. every sensor you use with digitalSmooth needs its own array.
  //displayValues(); //display values on the LCD screen
  
//THE FOLLOWING IS TO SECTION OUT THE READINGS.

  if(reading1 > touchThreshold){//contact or contact imminent.
    if (timer > 0){ //if within XX seconds of powerup AS DETERMINED BY INITIALIZATION OF VARIABLE (AT FIRST, THEN SEE BELOW)
    timer = 20; //reset the timer
    capStatus = 1; //signal: holding the vessel and the standby timer is running
    //break; //leave this if loop
    }
    else{
    capStatus = 2;  //unauthorized touch. signal alarm and taze cycle
    }
  }
  else if(smoothedVal > 6500){//disturbance detected, charge, sound alarm
    if (timer > 0){ //if within xx seconds of powerup
      timer = 20; //reset the timer
     capStatus = 1; //signal: holding the vessel and the standby timer is running
     //break; //leave this if loop
    }
    else{
    capStatus = 3;  //signal proximity detected
    }
  }

  else if (timer <= 0)//TIMEOUT REACHED
  {
    capStatus = 4;
  }

//NOW LET'S DO THINGS WITH THE READINGS:
  switch (capStatus){

    case 0: //NOTHING HAPPENS HERE. NOTHING SHOULD HAPPEN HERE
    break;
    case 1:{
      //holding the vessel and the standby timer is running. flash green every 10 seconds
    //count++;
    displayValues();
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
      setColor(255, 0, 0);  // RED
      tone(speakerPin, 2637, 125);//e
      delay(125);
      
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 4186, 125);//c2
      delay(125);
      
      setColor(255, 0, 0);  // RED
      tone(speakerPin, 2794, 125);//f
      delay(125);

      setColor(0, 0, 255);  // blue
      tone(speakerPin, 5274, 125);//e2
      delay(125);
      
      //tone(speakerPin, 4186, 125);//c2
      //delay(125);
      //digitalWrite(chargePin, LOW);
    }
      digitalWrite(chargePin, LOW);
      //reading1 = 0;
      //reading2 = 0;
      charged = false;
      break;      
    }
    case 3: //proximity detected. signal warning
    {
     //displayValues(); 
    
      if (flippy == true)
      {
       //sound chirp first warning
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
      tone(speakerPin, 1760, 70);//A2
      delay(200);
      flippy = false; 
      }
      else
      {
         //sound chirp second warning
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
      tone(speakerPin, 1760, 70);//A2
      delay(200);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
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
      //setColor(255, 255, 255);  // white
      digitalWrite(chargePin, HIGH);//charge on
      delay(5000);
      digitalWrite(chargePin, LOW);//charge off
      charged = true;
      setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      //setColor(0, 0, 0);
      delay(100);
      
      //setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      //setColor(0, 0, 0);
      delay(100);
      
      //setColor(255, 255, 255);  // white
      tone(speakerPin, 1109, 100);//C#
      //setColor(0, 0, 0);
      delay(100);

      //setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      setColor(0, 0, 0);
      //delay(100);
    
      }
    //flash RED every 10 seconds to indicate charge status
    else if(count > 10)
      {
        setColor(255, 0, 0);
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
      lcd.setCursor(0,0);
      lcd.print("ERROR: UNKNOWN");
      lcd.setCursor(0,1);
      lcd.print("shutting down...");
      //sound chirp
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 1380, 60);//A2
      delay(40);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 1380, 60);//A2 
      delay(60);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1380, 120);//A2 
      delay(4000);
      //reading = 0;
      digitalWrite(offPin, HIGH); 
      //digitalWrite(rstPin, HIGH); 
      delay(500); 
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

int digitalSmooth(int rawIn, int *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
  int j, k, temp, top, bottom;
  long total;
  static int i;
 // static int raw[filterSamples];
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j=0; j<filterSamples; j++){     // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;                // flag to know when we're done sorting              
  while(done != 1){        // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++){
      if (sorted[j] > sorted[j + 1]){     // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted [j+1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
    
  }

  return total / k;    // divide by number of samples
}
 int chirpCount = 0;
 long reading1 = 0;
 long reading2 = 0;
 //long readingComb = 0;
 int smoothedVal = 0;
 int touchThreshold = 13000;//ADJUST AS NECESSARY FOR SENSITIVTY
 int proximityDet = 120;
 bool charged = false;
 bool flippy = true;
 int reset = 0;
 int readOther = 0;
 //CONFIGURE CAPACITANCE SENSING
 CapacitiveSensor   cs_7_8 = CapacitiveSensor(7,8);// >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.
//
ISR(WDT_vect) //Interrupt Service Routine. Watchdog Interrupt Service. This is executed when watchdog timed out.
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

// WHAT TO DO IF INTERRUPTED VA HALL EFFECT SENSOR
void interrupted()
{
  detachInterrupt(digitalPinToInterrupt(hall1Pin));//DISABLE INTERRUPT SENSING
  delay(5);//DEBOUNCE
  readOther = digitalRead(hall2Pin);//TAKE A READING
  //
  if (readOther == 0)//SIGNAL CONFIRMED. SENSOR DROP TO 0 WHEN ACTIVE
  {
    //detachInterrupt(digitalPinToInterrupt(hall1Pin));//STOP INTERRUPT SENSING
    reset = 1;//FLAG FOR POWER OFF
  }
  else
  {
    attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING);
    reset = 0;
    return;
  }
  
}

void shutDown() {
  // check the value again - since it takes some time to
  // activate the interrupt routine, we get a clear signal.
  
  delay(10);
  readOther = digitalRead(hall2Pin);//TAKE A READING
  if (readOther == 0)
  {
    reset = 0;
    /* USE ONLY WHEN PROTOTYPING WITH LCD
    
     lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shutting down...");
    lcd.setCursor(0,1);
    lcd.print("discharging cap");
    digitalWrite(dischargePin, HIGH);
    //delay(2000);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Shutting down...");
    lcd.setCursor(0,1);
    lcd.print("good bye!"); 
    */
    //sound shutdown tone
    //for(int i = 0; i < 3; i++)
    //{
      //tone(speakerPin, i, 125);//
      //delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 4200, 125);//c2
      delay(125);
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 3200, 125);//f
      delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 2200, 125);//e2
      delay(125);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1200, 125);//c2
      delay(125);
      tone(speakerPin, 600, 125);
      //digitalWrite(chargePin, LOW);
      setColor(0, 0, 0);
    //}    
    delay(500);
    digitalWrite(offPin, HIGH);
  
}
  return;
}

void enterSleep(void) //Enters the arduino into sleep mode.
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  
  /* Now enter sleep mode. */
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */
  
  /* Re-enable the peripherals. */
  power_all_enable();
}


 void setColor(int red, int green, int blue)
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

 void displayValues()  //show values on LCD screen
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("A:");
      lcd.print(reading1);
      lcd.setCursor(8,0);
      //lcd.print("Dl");
      //lcd.print(readingComb);
      lcd.setCursor(0,1);
      //lcd.print("B:");
      //lcd.print(reading2);
      lcd.setCursor(9,1);
      lcd.print("Sm");
      lcd.print(smoothedVal);
      if (charged == true)
      {
      lcd.setCursor(0,1);
      lcd.print("CHARGED!");  
      }
    }

void setup() {
  
  //setup OFF pin and ensure it is not active right away
  pinMode(offPin, OUTPUT);
  digitalWrite(offPin, LOW);

  // initiate LCD(number of columns and rows:) and serial port
  /*lcd.begin(16, 2);
  Serial.begin(9600);
  
  // Print a message to the LCD.
  
   //INDICATE INITIALIZATION SUCCESS
  
  lcd.clear();
  //lcd.setCursor(0, 0);
  lcd.print("SCARAB v1.0");
  lcd.setCursor(0, 1);
  lcd.print("by Manuel Labor");
  Serial.println("SCARAB v1.0");
  Serial.println("by Manuel Labor");
  delay(1000);
  //lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  Serial.println("Initializing...");
  */
  //CONFIGURE PINS
  pinMode(chargePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(capSensePin, INPUT);
  pinMode(capSourcePin, OUTPUT);
  //pinMode(offPin, OUTPUT);
  pinMode(hall1Pin, INPUT);
  pinMode(hall2Pin, INPUT);
  //cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  cs_7_8.set_CS_AutocaL_Millis(80); 
  
  /*** SETUP THE WDT ***/
  /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
   /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* set new watchdog timeout prescaler value */
  WDTCSR = 1<<WDP1 | 1<<WDP2; /* 1.0 seconds */
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);

  //ENABLE HALL SENSOR INTERRUPT
  attachInterrupt(digitalPinToInterrupt(hall1Pin), interrupted, FALLING);
  // Global Enable INT0 interrupt
  //GICR |= ( 1 < < INT0);
  // Signal change triggers interrupt
  //MCUCR |= ( 1 << ISC00);
  //MCUCR |= ( 0 << ISC01);
  //delay(1000);
  //lcd.clear();
  //lcd.print("Finished initialization");
  //Serial.println("Finished initialization");
    //AUDIO INIT CONFIRM
  for(int i = 0; i < 2; i++){
    setColor(255, 0, 0);  // red
    tone(speakerPin, 3136, 125);
    delay(125);
    setColor(0, 255, 0);  // green
    tone(speakerPin, 3951, 125);
    delay(125);
    setColor(0, 0, 255);  // blue
    tone(speakerPin, 4186, 125);
    delay(125);
  setColor(0,0,0);
  
}
  delay(1000);
  //lcd.clear();
}

void loop() { // put your main code here, to run repeatedly:  
  //cs_7_8.reset_CS_AutoCal(); //RECALIBRATE
if (reset == 1)
{
  shutDown();
}
//cs_7_18.reset_CS_AutoCal(); //RECALIBRATE  
    //
if(f_wdt == 1){ //THINGS TO DO IF AWAKE, WHICH IT IS WHENEVER IT IS, get it? So do this:
  
  /* Don't forget to clear the flag. */
  f_wdt = 0; //clear the watchdog timer flag. 
  
  reading1 =  cs_7_8.capacitiveSensor(20); //get the reading from sensor 1
  smoothedVal = digitalSmooth(reading1, sensSmoothArray1);  // Run average through the smoothing algorithm. every sensor you use with digitalSmooth needs its own array.
  //displayValues(); //display values on the LCD screen
  
//THE FOLLOWING IS TO SECTION OUT THE READINGS.

  if(reading1 > touchThreshold){//contact or contact imminent.
    if (timer > 0){ //if within XX seconds of powerup AS DETERMINED BY INITIALIZATION OF VARIABLE (AT FIRST, THEN SEE BELOW)
    timer = 20; //reset the timer
    capStatus = 1; //signal: holding the vessel and the standby timer is running
    //break; //leave this if loop
    }
    else{
    capStatus = 2;  //unauthorized touch. signal alarm and taze cycle
    }
  }
  else if(smoothedVal > 6500){//disturbance detected, charge, sound alarm
    if (timer > 0){ //if within xx seconds of powerup
      timer = 20; //reset the timer
     capStatus = 1; //signal: holding the vessel and the standby timer is running
     //break; //leave this if loop
    }
    else{
    capStatus = 3;  //signal proximity detected
    }
  }

  else if (timer <= 0)//TIMEOUT REACHED
  {
    capStatus = 4;
  }

//NOW LET'S DO THINGS WITH THE READINGS:
  switch (capStatus){

    case 0: //NOTHING HAPPENS HERE. NOTHING SHOULD HAPPEN HERE
    break;
    case 1:{
      //holding the vessel and the standby timer is running. flash green every 10 seconds
    //count++;
    displayValues();
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
      setColor(255, 0, 0);  // RED
      tone(speakerPin, 2637, 125);//e
      delay(125);
      
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 4186, 125);//c2
      delay(125);
      
      setColor(255, 0, 0);  // RED
      tone(speakerPin, 2794, 125);//f
      delay(125);

      setColor(0, 0, 255);  // blue
      tone(speakerPin, 5274, 125);//e2
      delay(125);
      
      //tone(speakerPin, 4186, 125);//c2
      //delay(125);
      //digitalWrite(chargePin, LOW);
    }
      digitalWrite(chargePin, LOW);
      //reading1 = 0;
      //reading2 = 0;
      charged = false;
      break;      
    }
    case 3: //proximity detected. signal warning
    {
     //displayValues(); 
    
      if (flippy == true)
      {
       //sound chirp first warning
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
      tone(speakerPin, 1760, 70);//A2
      delay(200);
      flippy = false; 
      }
      else
      {
         //sound chirp second warning
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
      tone(speakerPin, 1760, 70);//A2
      delay(200);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 70);//A2
      delay(200);
      setColor(100, 255, 0);  // orange
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
      //setColor(255, 255, 255);  // white
      digitalWrite(chargePin, HIGH);//charge on
      delay(5000);
      digitalWrite(chargePin, LOW);//charge off
      charged = true;
      setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      //setColor(0, 0, 0);
      delay(100);
      
      //setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      //setColor(0, 0, 0);
      delay(100);
      
      //setColor(255, 255, 255);  // white
      tone(speakerPin, 1109, 100);//C#
      //setColor(0, 0, 0);
      delay(100);

      //setColor(255, 255, 255);  // white
      tone(speakerPin, 2217, 100);//C#2
      setColor(0, 0, 0);
      //delay(100);
    
      }
    //flash RED every 10 seconds to indicate charge status
    else if(count > 10)
      {
        setColor(255, 0, 0);
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
      lcd.setCursor(0,0);
      lcd.print("ERROR: UNKNOWN");
      lcd.setCursor(0,1);
      lcd.print("shutting down...");
      //sound chirp
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 1380, 60);//A2
      delay(40);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 1380, 60);//A2 
      delay(60);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1380, 120);//A2 
      delay(4000);
      //reading = 0;
      digitalWrite(offPin, HIGH); 
      //digitalWrite(rstPin, HIGH); 
      delay(500); 
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

int digitalSmooth(int rawIn, int *sensSmoothArray){     // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
  int j, k, temp, top, bottom;
  long total;
  static int i;
 // static int raw[filterSamples];
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j=0; j<filterSamples; j++){     // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;                // flag to know when we're done sorting              
  while(done != 1){        // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++){
      if (sorted[j] > sorted[j + 1]){     // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted [j+1] =  sorted[j] ;
        sorted [j] = temp;
        done = 0;
      }
    }
  }

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
    
  }

  return total / k;    // divide by number of samples
}
