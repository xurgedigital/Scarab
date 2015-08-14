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

#define filterSamples   5
int sensSmoothArray1 [filterSamples];
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
 //declare pin names
 /*const int chargePin = 12;
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
 //int capStatus = 0;
 int count = 0;
 int secondCount = 0;
 //int chirp = 0;
 int chirpCount = 0;
 long reading = 0;

 CapacitiveSensor   cs_7_8 = CapacitiveSensor(7,8);          // >=10 megohm resistor between pins 7 & 8, pin 8 is sensor pin.

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
ISR(WDT_vect)
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



/*int smooth(int data, float filterVal, float smoothedVal){


  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

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
  //cs_7_8.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
  cs_7_8.set_CS_Timeout_Millis(100);
  

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
  lcd.setCursor(0, 0);
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

void loop() {
  // put your main code here, to run repeatedly:
  
  
    //
if(f_wdt == 1){ //if awake:
  
  /* Don't forget to clear the flag. */
  f_wdt = 0;
  //long reading = 0;
  int capStatus = 0;
  //long start = millis();
  reading =  cs_7_8.capacitiveSensor(30);
  //float smoothedVal =  smooth(reading, .25, smoothedVal);   // second parameter determines smoothness  - 0 is off,  .9999 is max smooth 
  int smoothedVal = digitalSmooth(reading, sensSmoothArray1);  // every sensor you use with digitalSmooth needs its own array

  Serial.print(reading);
  Serial.print("\tSmooth: ");
  Serial.println(smoothedVal);
  /*if(reading == -2){
    reading = 0;
  }*/

  if((reading == -2) || (reading >444)){//touch detected, sound alarm, taze cycle
    capStatus = 1;  
    //reading = 0;
  }
  else if(reading >= 244){//disturbance detected, charge, sound alarm
    capStatus = 2;  
    //reading = 0;
  }
  else if((reading > 30) && (smoothedVal < 100)){//imminent touch detected, charge, sound warning
    capStatus = 3;  
    //reading = 0;
  }
  else if((reading > 12) && (smoothedVal < 31)){//proximity detected, charge, chirp, blink temp
    capStatus = 4;  
    //reading = 0;
  }
  else if (reading < 11){//no proximity detected
    capStatus = 5;
    //reading = 0;
  }
  //else { //possible error. restart.
  //  capStatus = 6;
    //reading = 0;
  //}

  
  switch (capStatus){

    case 0:
    break;
    case 1:{
      ////touch detected, sound alarm, taze cycle
      
      //charge
      digitalWrite(chargePin, HIGH);
      //diagnosis
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("TOUCH DETECTED");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print(" S: ");
      lcd.print(smoothedVal);
      //sound alarm
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
      
      reading = 0;
      break;
        //
       
    }
    case 2:
    {
      //manipulation detected, charge, sound alarm
      //charge
      digitalWrite(chargePin, HIGH);
      //diagnosis
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("DISTURBANCE DETECTED");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print(" S: ");
      lcd.print(smoothedVal);
      //sound alarm
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
      //reading = 0;
      break;      
    }
    case 3:
    {
      //imminent touch detected, charge, sound warning
      //charge
      digitalWrite(chargePin, HIGH);
      lcd.clear();
      //diagnosis
      lcd.setCursor(0,0);
      lcd.print("TOUCH IMMINENT");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print(" S: ");
      lcd.print(smoothedVal);
      //sound alarm
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
      //delay(100);
      digitalWrite(chargePin, LOW);
      
      //reading = 0;
      break;
    }
    case 4:
    {
      //proximity detected, charge, chirp, blink temp
      //charge
      digitalWrite(chargePin, HIGH);
      lcd.clear();
      //diagnosis
      lcd.setCursor(0,0);
      lcd.print("PROXIMITY DETECTED");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print(" S: ");
      lcd.print(smoothedVal);
      //sound chirp
      setColor(255, 0, 0);  // red
      tone(speakerPin, 3520, 60);//A2
      delay(40);
      setColor(100, 0, 255);  // orange
      tone(speakerPin, 1760, 60);//A2
      delay(200);
      setColor(0, 0, 0);  //
      delay(100);
      setColor(255, 0, 0);
      delay(500);
      setColor(0, 0, 0);  //
      delay(100);
      setColor(255, 0, 0);
      delay(500);
      setColor(0, 0, 0);
      digitalWrite(chargePin, LOW);
      
      //chirp = 1;
      //reading = 0;
      break;
    }
    case 5:
    {
      //no proximity detected
      //count++
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("SYSTEM ACTIVE");
      lcd.setCursor(0,1);
      //lcd.print("scan: ");
      lcd.setCursor(0,1);
      lcd.print(reading);
      lcd.print(" S: ");
      lcd.print(smoothedVal);
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
      
      //lcd.clear();
      //reading = 0;
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
  }
  capStatus = 0;
//while the vessel detects proximity, go to sleep and check every .xxx seconds
    //while((cs_7_8.capacitiveSensor(30) > 600) || (cs_7_8.capacitiveSensor(30) < 0)){
    //while((reading > 300) || (reading < 0)){
    /* Don't forget to clear the flag. */
    //f_wdt = 0;//NECESSARY?
     
    
    /*total2 = cs_7_8.capacitiveSensor(30);
    if(total2 == -2){//TOUCH??
    total2 = 1;
    lcd.setCursor(0,1);
    lcd.print("TOUCH?");
    }
    lcd.setCursor(0,0);
    lcd.print(total2);
    Serial.print("PROXIMITY DETECTED: ");
    Serial.print(total2);
    Serial.print("   \t");
    Serial.print("AVERAGE: ");
    Serial.println((total2 + cs_7_8.capacitiveSensor(30)) / 2);

    setColor(0, 0, 255); //blue
    delay(50);
    setColor(0, 0, 0);
    lcd.clear();
    /* Don't forget to clear the flag. */
    //f_wdt = 0;
    /* Re-enter sleep mode. */
    //enterSleep();
    
   //} //END OF WHILE LOOP*/
   //BELOW IS WHAT TO DO WHEN PROX NOT DETECTED
    //total2 =  cs_7_8.capacitiveSensor(30);
    //lcd.clear();
    //lcd.print(cs_7_8.capacitiveSensor(30));
    //lcd.setCursor(0,1);
    //lcd.print(millis() - start);
    //Serial.print(millis() - start);        // check on performance in milliseconds
    //Serial.print("\tRAW: ");
    //Serial.print(reading);
    //Serial.print("   \tAVG: ");                    // tab character for debug window spacing
    //Serial.println( (reading / total2) / 2 );                  // print sensor output 1
    //delay(10);
    /* Don't forget to clear the flag. */
    f_wdt = 0;
  //here the vessel is NOT being held, start counting, then arm system.
  //flash red
    //setColor(255, 0, 0);
    //delay(50);
    //setColor(0, 0, 0);
    //NEED TO START COUNTING SOMEWHERE HERE
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
