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

#define filterSamples   9 //taking 19 samples (odd numbers only)
int sensSmoothArray1 [filterSamples];
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

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
 const int greenPin = 10;
 const int bluePin = 9;
 const int speakerPin = 6;
 const int capSensePin = 8;
 const int capSourcePin = 7;
 const int offPin = 16;//A2
 const int hall1Pin = 18;//A4
 const int hall2Pin = 17;//A3
 const int rstPin = 19;
 
 volatile int f_wdt=1;
 //const unsigned long maxTime = 15000; //the max number in sec*1k to wait BEFORE CHARGING
 //long reading = 0;
 //long total2 = 0;
 int capStatus = 0;
 int count = 0;
 int timer = 15; //THIS 
 int filterCount = 0;
 int secondCount = 0;
 //int chirp = 0;
 int chirpCount = 0;
 long reading1 = 0;
 long reading2 = 0;
 long readingComb = 0;
 int smoothedVal = 0;
 int touchThreshold = 400;
 int proximityDet = 220;
 bool charged = false;
 
 CapacitiveSensor   cs_7_8 = CapacitiveSensor(7,8);          // >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.
 CapacitiveSensor   cs_7_18 = CapacitiveSensor(7,18);          // >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.

/***************************************************
 *  Name:        ISR(WDT_vect)
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 *
 ***************************************************/
ISR(WDT_vect) //Interrupt Service Routine
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
  else
  {
    Serial.println("WDT Overrun!!!");
  }
}


/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
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
      lcd.print("Av");
      lcd.print(readingComb);
      lcd.setCursor(0,1);
      lcd.print("B:");
      lcd.print(reading2);
      lcd.setCursor(8,1);
      lcd.print("Sm");
      lcd.print(smoothedVal);
    }
/*int smooth(int data, float filterVal, float smoothedVal){


  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }
\\
  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
}*/
 

void setup() {
  // put your setup code here, to run once:
  // set up the LCD's number of columns and rows:
  digitalWrite(rstPin, HIGH);
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Initializing...");
  Serial.begin(9600);
  Serial.println("Initializing...");
  //CONFIGURE PINS
  pinMode(chargePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(capSensePin, INPUT);
  pinMode(capSourcePin, OUTPUT);
  pinMode(offPin, OUTPUT);
  pinMode(hall1Pin, INPUT);
  pinMode(hall2Pin, INPUT);
  //pinMode(rstPin, OUTPUT);
  

  //CONFIGURE CAPSENSE PARAMETERS
  /*The baseline is value is re-calibrated at intervals 
   * determined by CS_Autocal_Millis. 
   * The default value is 200000 milliseconds (20 seconds). 
   * This re-calibration may be turned off 
   * by setting CS_Autocal_Millis to a high value 
  with the set_CS_AutocaL_Millis() method. */
  
  //cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  //cs_7_8.set_CS_Timeout_Millis(40);
  //cs_7_18.set_CS_Timeout_Millis(40);
  cs_7_8.set_CS_AutocaL_Millis(80); 
  cs_7_18.set_CS_AutocaL_Millis(80);
  

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

  //INDICATE INITIALIZATION SUCCESS
  lcd.clear();
  //lcd.setCursor(0, 0);
  lcd.print("SCARAB v1.0");
  lcd.setCursor(0, 1);
  lcd.print("by Manuel Labor");
  Serial.println("SCARAB v1.0: INITIALIZED");
  Serial.println("by Manuel Labor");
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
    
  }
  delay(500);
  lcd.clear();
  //lcd.setCursor(0, 0);
  lcd.print("INITIALIZED");  
  delay(750);
  setColor(0,0,0);
  lcd.clear();
  
}

void loop() { // put your main code here, to run repeatedly:  
  //cs_7_8.reset_CS_AutoCal(); //RECALIBRATE
  //cs_7_18.reset_CS_AutoCal(); //RECALIBRATE  
    //
if(f_wdt == 1){ //THINGS TO DO IF AWAKE, WHICH IT IS WHENEVER IT IS, get it? So do this:
  
  /* Don't forget to clear the flag. */
  f_wdt = 0; //clear the watchdog timer flag. 
  
  //int capStatus = 0;
  //long start = millis(); //used to display diag info
  reading1 =  cs_7_8.capacitiveSensor(20); //get the reading from sensor 1
  reading2 =  cs_7_18.capacitiveSensor(20); //get the reading from sensor 2
  readingComb = ((reading1 + reading2) / 2); //get the average of both readings
  //float smoothedVal =  smooth(reading, .25, smoothedVal);   // second parameter determines smoothness  - 0 is off,  .9999 is max smooth 
  smoothedVal = digitalSmooth(readingComb, sensSmoothArray1);  // Run average through the smoothing algorithm. every sensor you use with digitalSmooth needs its own array.

  /* for diagnostics
  Serial.print(reading1);
  Serial.print("\tSmooth: ");
  Serial.println(smoothedVal);
  */
  displayValues(); //display values on the LCD screen

  
//THE FOLLOWING IS TO SECTION OUT THE READINGS.

  if(readingComb > touchThreshold){//contact or contact imminent.
    if (timer > 0){ //if within XX seconds of powerup AS DETERMINED BY INITIALIZATION OF VARIABLE (AT FIRST, THEN SEE BELOW)
    timer = 30; //reset the timer
    capStatus = 1; //signal: holding the vessel and the standby timer is running
    //break; //leave this if loop
    }
    else{
    capStatus = 2;  //unauthorized touch. signal alarm and taze cycle
    }
  }
  else if((readingComb >=proximityDet) && (readingComb <=touchThreshold)){//disturbance detected, charge, sound alarm
    if (timer > 0){ //if within 30 seconds of powerup
      timer = 30; //reset the timer
     capStatus = 1; //signal all is normal
     //break; //leave this if loop
    }
    else{
    capStatus = 3;  //signal warning and charge
    }
  }
    //reading = 0;
  
  else if((readingComb - smoothedVal) <0){ //capacitance decreasing. walking away.
    if (timer > 0){ //if within 30 seconds of powerup
      //timer--; //count down the timer
      capStatus = 1; //signal all is normal
      //break; //leave this if loop
    }
    else{
      capStatus = 4;
    }  
    //reading = 0;
  }
  
   

//NOW LET'S DO THINGS WITH THE READINGS:
  /*********************************************
  * STATUSES:
  * 1 = all is normal
  * 2 = unauthorized touch. signal alarm and taze cycle
  * 3 = proximity detected. signal warning and charge
  * 4 = going away. signal move away
  **********************************************/
  switch (capStatus){

    case 0: //NOTHING HAPPENS HERE. NOTHING SHOULD HAPPEN HERE
    break;
    case 1:{
      //holding the vessel and the standby timer is running. flash green every 10 seconds
    //count++;
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
           
      
      
      //digitalWrite(chargePin, LOW);
      
      break;
        //
       
    }
    case 2: //unauthorized touch. signal alarm and taze cycle
    {
      //charge
      digitalWrite(chargePin, HIGH);
      
      //sound alarm
    for(int i = 0; i < 4; i++)
    {
      setColor(255, 0, 0);  // red
      tone(speakerPin, 2637, 125);//e
      delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 4186, 125);//c2
      delay(125);
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 2794, 125);//f
      delay(125);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 5274, 125);//e2
      delay(125);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 4186, 125);//c2
      delay(125);
      digitalWrite(chargePin, LOW);
      lcd.clear();
    }
      digitalWrite(chargePin, LOW);
      //reading1 = 0;
      //reading2 = 0;
      break;      
    }
    case 3: //proximity detected. signal warning and charge
    {
      
    //charge
      digitalWrite(chargePin, HIGH);
      
      //sound chirp
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 60);//A2
      delay(40);
      setColor(100, 0, 255);  // orange
      tone(speakerPin, 1760, 60);//A2
      delay(200);
      
    /*
    //sound warning
      setColor(255, 0, 0);  // red
      tone(speakerPin, 2217, 100);//C#2
      delay(80);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 2217, 100);//C#2
      delay(80);
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 1109, 100);//C#
      delay(80);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 2217, 100);//C#2
      delay(100);
    setColor(0, 0, 0);
      */
    
      digitalWrite(chargePin, LOW);
      
      //reading = 0;
      //reading1 = 0;
      //reading2 = 0;
      break;
    }
    case 4:
    {
      //going away. signal move away
      
     
      //sound chirp
      setColor(0, 255, 0);  // green
      tone(speakerPin, 1760, 60);//A2
      delay(200);
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 3520, 60);//A2
      delay(40);
      setColor(0, 0, 0);  //
      //digitalWrite(chargePin, LOW);
      
      //chirp = 1;
      //reading = 0;
      //reading1 = 0;
      //reading2 = 0;
      break;
    }
    case 5:
    {
      //all is normal but timer is out. CHARGE and flash red every 10 seconds.
    count++;
    if(count > 10)
      {
        digitalWrite(chargePin, HIGH);//charge  
    //flash RED
        setColor(255, 0, 0);
        delay(500);
        setColor(0, 0, 0);
        
    count = 0; //reset count
        
      }
      else
      {
        count ++;
      }
      
      digitalWrite(chargePin, LOW);
      
      //reading1 = 0;
      //reading2 = 0;
      break;
    }
    /*case 6:
    {
      //possible error. restart.
      lcd.setCursor(0,0);
      lcd.print("ERROR: OUTSIDE PARAMETERS");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print("  S: ");
      lcd.print(smoothedVal);
      //sound chirp
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 1380, 60);//A2
      delay(40);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 1380, 60);//A2 
      delay(60);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1380, 120);//A2 
      delay(500);
      //reading = 0;
      digitalWrite(rstPin, LOW);
      delay(5); 
      break;
      
    }*/
    default:
    {
      //possible error. restart.
      lcd.setCursor(0,0);
      lcd.print("ERROR: UNKNOWN");
      lcd.setCursor(0,1);
      lcd.print("restarting...");
      //sound chirp
      setColor(0, 0, 255);  // blue
      tone(speakerPin, 1380, 60);//A2
      delay(40);
      setColor(0, 255, 0);  // green
      tone(speakerPin, 1380, 60);//A2 
      delay(60);
      setColor(255, 0, 0);  // red
      tone(speakerPin, 1380, 120);//A2 
      delay(500);
      //reading = 0;
      digitalWrite(rstPin, LOW); 
      //digitalWrite(rstPin, HIGH); 
      delay(5); 
      break;
    }
     //reading=0; 
  }//END OF SWITCH STATEMEMNT
  
  capStatus = 0;
  timer--; //count tdown timer
  //so by this point nothing out of the ordinary has happened.
  if (timer <= 0){//signal there is NO CONTACT and timer is out. so, CHARGE UP! because the vessel is sitting idle.
    if (charged == false){ 
      digitalWrite(chargePin, HIGH);//charge on
      delay(1000);
      digitalWrite(chargePin, LOW);//charge off
      charged = true;
    }
    //flash RED every 10 seconds to indicate charge status
    if(count > 10)
      {
                
        setColor(255, 0, 0);
        delay(400);
        setColor(0, 0, 0);
        count = 0;
        
      }
      else
      {
        count ++;
      }
    
    timer = 0;        //NECESSARY??            
  }

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

/*
  for (j = 0; j < (filterSamples); j++){    // print the array to debug
    Serial.print(sorted[j]); 
    Serial.print("   "); 
  }
  Serial.println();
*/

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((filterSamples * 15)  / 100), 1); 
  top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j< top; j++){
    total += sorted[j];  // total remaining indices
    k++; 
    // Serial.print(sorted[j]); 
    // Serial.print("   "); 
  }

//  Serial.println();
//  Serial.print("average = ");
//  Serial.println(total/k);
  return total / k;    // divide by number of samples
}

