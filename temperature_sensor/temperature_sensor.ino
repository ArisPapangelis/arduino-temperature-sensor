//Library used for timing function calls
#include <timer.h>

//Libraries used for interfacing with various peripherals
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>
#include <LiquidCrystal.h>

//Various constant values that are used throughout our program
#define TRIGGER_PIN 11
#define ECHO_PIN 12
#define MAX_DISTANCE 200
#define NUM_ROWS 2
#define NUM_COLS 16
#define ONE_WIRE_BUS 10
#define HIGH_TEMP 33
#define LOW_TEMP 20
#define DANGEROUS_TEMP 37
#define HIGH_TEMP_LED 24
#define LOW_TEMP_LED 22
#define DANGEROUS_TEMP_RELAY 7
#define ACTIVATION_DISTANCE 20

//Constructors for the objects that represent our peripherals
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 
LiquidCrystal lcd(9,8,5,4,3,2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Timer object, used for timing function calls
auto timer = timer_create_default();

volatile int i;                     //The index for the current temperature value (0 to 23)
volatile float temp[24];            //The array that holds the temperature measurements in the last two minutes, taken every 5 seconds
volatile float meanTemp;            //The average temperature for the previous two minutes (24 measurements)
volatile float prevTemp;            //The most recent temperature taken
volatile unsigned long screenTime;  //This variable is used to check if ten seconds have passed since the last average temperature was calculated
volatile bool flag;                 //This variable is used to clear the screen from high/low temperature warnings, when the temperature is within limits again
  
void setup() {
  Serial.begin(9600);                 //Serial monitor initialization
  lcd.begin(NUM_COLS,NUM_ROWS);       //LCD initialization
  sensors.begin();                    //Temperature sensor initialization
  i=0;          //Starting index is zero
  meanTemp=0;   //Starting average temperature is zero
  screenTime=0; //Starting value for screenTime is zero
  flag=true;    //Starting value for flag is true
  
  pinMode(HIGH_TEMP_LED,OUTPUT);          //The pin for high temperature LED is set as output
  pinMode(LOW_TEMP_LED,OUTPUT);           //The pin for low temperature LED is set as output
  pinMode(DANGEROUS_TEMP_RELAY,OUTPUT);   //The pin for dangerous temperature relay is set as output
  digitalWrite(HIGH_TEMP_LED,LOW);        //Starting value for both LEDs is low
  digitalWrite(LOW_TEMP_LED,LOW);
  getTemp();                              //A measurement is taken in setup, in order to have a starting value for the temperature
  timer.every(5000, getTemp);             //The function that saves the current temperature will be called once every five seconds
}

void loop() {
  timer.tick();   //Updating timer and checking if five seconds have passed
  
  //Code that calculates the average temperature for the previous 2 minutes, in case 24 measurements have already been taken
  if (i>23){
    showAverage();
  }
  
  //Clear lcd after average temp has been displayed for 10 seconds
  if (screenTime!=0){
    if (millis() - screenTime>=10*1000){
      lcd.clear();
      screenTime=0;
    }
  }

  //Code controlling the activation of the relay. The relay will be activated once the previous temperature exceeds the dangerous temperature threshold, or deactivated otherwise
  if (prevTemp>DANGEROUS_TEMP){
    digitalWrite(DANGEROUS_TEMP_RELAY,HIGH);
  }
  else{
    digitalWrite(DANGEROUS_TEMP_RELAY,LOW);
  }

  //Code controlling the activation of high or low temperature LEDs
  if (prevTemp>HIGH_TEMP){
    flag=true;
    digitalWrite(HIGH_TEMP_LED,HIGH);
    lcd.clear();                              //Once the temperature exceeds the high threshold, the corresponding LED pin is set to high and a warning is printed on the screen, for as long as the temperature is higher than the threshold
    lcd.setCursor(0,0);
    lcd.print("High temp");
    lcd.print(prevTemp,2);
    lcd.print(" Celsius");
  }
  else if (prevTemp<LOW_TEMP){
    flag=true;
    digitalWrite(LOW_TEMP_LED,HIGH);
    lcd.clear();                              //Once the temperature drops below the low threshold, the corresponding LED pin is set to high and a warning is printed on the screen, for as long as the temperature is lower than the threshold
    lcd.setCursor(0,0);
    lcd.print("Low temp");
    lcd.print(prevTemp,2);
    lcd.print(" Celsius");
  }
  else {
    digitalWrite(HIGH_TEMP_LED,LOW);
    digitalWrite(LOW_TEMP_LED,LOW);          //If the temperature is within normal operating limits, both LEDs are turned off
    
    if (flag==true){                         //If the variable flag is true, it means we are again within normal operating limits, so we can clear the LCD from the previous high/low temperature warning. Then, the flag is set to false once more
      lcd.clear();
      flag=false;
    }
  }

  //Code controlling the proximity sensor
  unsigned int distance = sonar.ping_cm();   //Getting the distance from the ultrasonic sensor
  Serial.print(distance);
  Serial.println("cm");
  if (distance<ACTIVATION_DISTANCE){         //If the distance is less than the activation distance we set, then we print the previous temperature in the first line of the LCD, and the average temperature of the last two minutes in the second line of the LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Prev temp: ");
    lcd.print(prevTemp,2);
    lcd.setCursor(0,1);
    lcd.print("Avg temp: ");
    lcd.print(meanTemp,2);
  }

}

//Function that saves the current temperature
bool getTemp() {
  
  sensors.requestTemperatures();          //Polling the temperature sensor for the current temperature
  Serial.print("Celsius temperature: ");
  temp[i]= sensors.getTempCByIndex(0);    //We save the temperature in Celsius in the temp array that we defined at the start of the program
  Serial.println(temp[i]);
  prevTemp=temp[i];                       //The previous temperature is refreshed, becoming the value we just saved
  i++;                                    //The index is advanced by one, in order for the next measurement to be saved in the next position of the array

  return true;                            //The function returns true, which means that we want it to execute again after five seconds
  
}

//Function that calculates and prints to the LCD the average temperature in the last two minutes
void showAverage() {
  
  float sum=0;
  for (int j=0;j<24;j++) {
    sum+=temp[j];             //Calculating the sum of the 24 temperature values taken in the last two minutes
  }
  meanTemp = sum / 24;        //Calculating the average temperature in the last two minutes, by dividing the sum by 24
  i=0;                        //The index is set to zero, for the next 24 values to be saved
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Avg temp:");     //We print the average temperature to the LCD, once it has been calculated. It will stay on the screen for ten seconds
  lcd.print(meanTemp,2);
  screenTime=millis();        //Used in the loop() function to check if ten seconds have passed since the average temperature was calculated
  
}
