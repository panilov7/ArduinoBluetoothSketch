//Arduino 1.0+ Only

#include "Wire.h"
#include <LiquidCrystal.h>
#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527
byte bPin = 10;
byte connPin = 7;
byte lightSense = A0;
byte proxPin = A1;
byte valvePin = 13;
boolean displayOn = true;

byte second = 0;
byte minute = 0;
byte hour = 0;
byte weekDay = 0;
byte monthDay = 0;
byte month = 0;
int year = 0;
int initVal = 0;

byte backlight = 178;

boolean isConnected = false;
boolean isFilling = false;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

byte deg[8] = { 
  B01100, B10010, B10010, B01100, B00000, B00000, B00000 };
byte arrow[8] = { 
  B00100, B00100, B00100, B00100, B10101, B01110, B00100 };

void setup(){
  Wire.begin();
  Serial.begin(9600);
  pinMode(bPin, OUTPUT);
  pinMode(connPin, INPUT);
  pinMode(proxPin, INPUT);
  pinMode(valvePin, OUTPUT);
  digitalWrite(valvePin, LOW);

  lcd.begin(16, 2);

  lcd.createChar(6, deg);
  lcd.createChar(7, arrow);

  setLCDBacklight();

  loading();

  analogRead(proxPin);
  delay(50);

  initVal = analogRead(proxPin);

  lcd.clear();
  //setDateAndTime(); //MUST CONFIGURE IN FUNCTION
}

void loading(){ 

  byte p20[8] = { 
    B10000, B10000, B10000, B10000, B10000, B10000, B10000           };
  byte p40[8] = { 
    B11000, B11000, B11000, B11000, B11000, B11000, B11000           };
  byte p60[8] = { 
    B11100, B11100, B11100, B11100, B11100, B11100, B11100           };
  byte p80[8] = { 
    B11110, B11110, B11110, B11110, B11110, B11110, B11110           };
  byte p100[8] = { 
    B11111, B11111, B11111, B11111, B11111, B11111, B11111           };

  lcd.createChar(0, p20);
  lcd.createChar(1, p40);
  lcd.createChar(2, p60);
  lcd.createChar(3, p80);
  lcd.createChar(4, p100);

  lcd.setCursor(0, 0);
  lcd.print("    Welcome!");

  for(int i = 0; i < 16; i++){
    for(int j = 0; j < 5; j++){
      lcd.setCursor(i, 1);
      lcd.write(j);
      delay(30);
    }
  }
}

boolean b = true;
String data = "";
char ch;
boolean autoBacklight = true;

void loop(){

  isConnected = digitalRead(connPin);

  if (isConnected && b){
    Serial.print("Connected:b");
    if (autoBacklight){
      Serial.println("a");
    }
    else {
      Serial.println(backlight);
    }
    b = false;
  } 
  else if(!isConnected){
    b = true;
  }

  while(Serial.available() > 0){
    ch = Serial.read();
    data.concat(ch);
    delay(2);
  }

  if(displayOn && !isFilling){
    printDateAndTimeToLCD();
  }
  //printDateAndTimeToSerial();

  if (data.startsWith("b")){
    String s = data.substring(1);

    if(s == "a"){
      autoBacklight = true;
      data = "";
    }
    else{
      autoBacklight = false;
      backlight = s.toInt();  
      lcd.display();
      displayOn = true;
      setLCDBacklight();
      data = "";
    }
  }

  autoLCDBacklight();  
  proximityStuff();

}

long previousPROXms = 0;
long updatePROX = 100; // in ms

void proximityStuff(){

  unsigned long currentMillis = millis();

  if(currentMillis - previousPROXms > updatePROX) {
    previousPROXms = currentMillis; 

    analogRead(proxPin);
    delay(50);

    fillCup(analogRead(proxPin));

  }
}

int senseVal = 0;

long previousBLms = 0;
long updateBLCD = 3000; // in ms

long previousOnLCDms = 0;
long updateOnLCD = 30000; // in ms

int previousSenseVal = 255;

void fillCup(int val){
  if(val >= initVal + 5){
    digitalWrite(valvePin, HIGH);
    previousOnLCDms = millis();
    isFilling = true;
    if(!displayOn){
      turnLcdOn(millis());
      lcdFilling();
    }
    else{
      lcdFilling(); 
    }
    delay(200);
  }
  else {
    digitalWrite(valvePin, LOW);
    isFilling = false;
  }
}

void lcdFilling(){
  lcd.clear();
  lcd.setCursor(7, 1);
  lcd.write(7);
}

void turnLcdOn(unsigned long currentMillis){
  Serial.println("Display On");   
  displayOn = true;
  previousOnLCDms = currentMillis;
  lcd.display();
  previousSenseVal = 255;
}

void turnLcdOff(){
  previousSenseVal = senseVal;  
  if(backlight != 0){  
    Serial.println("Display Off");  
    displayOn = false; 
    backlight = 0;
    setLCDBacklight();
    lcd.noDisplay();
  }
}
void autoLCDBacklight(){
  if (autoBacklight){

    unsigned long currentMillis = millis();

    analogRead(lightSense);
    delay(50);

    senseVal = constrain(map(analogRead(lightSense), 0, 1000, 25, 255), 0, 255);

    if(senseVal - 10 > previousSenseVal){
      turnLcdOn(currentMillis);
    }
    else if(currentMillis - previousOnLCDms >= updateOnLCD){
      turnLcdOff();
    }
    else if(currentMillis - previousBLms > updateBLCD) {
      previousBLms = currentMillis; 
      if (senseVal - backlight > 10){
        while (senseVal > backlight){
          backlight++;
          setLCDBacklight();
          delay(7);
        }
        previousOnLCDms = currentMillis;
      } 
      else if (backlight - senseVal > 10){
        while (backlight > senseVal){
          backlight--;
          setLCDBacklight();
          delay(3);
        }
        previousOnLCDms = currentMillis;
      }
    }
  }
}

void setLCDBacklight(){
  analogWrite(bPin, 255 - backlight);
}

long previousLCDms = 0;
long updateLCD = 1000; // in ms

void printDateAndTimeToLCD(){

  unsigned long currentMillis = millis();

  if(currentMillis - previousLCDms > updateLCD) {
    // save the last time you blinked the LED 
    previousLCDms = currentMillis; 

    getDataFromRTC();

    lcd.setCursor(0, 0);

    lcd.print(weekDayToString(weekDay));
    lcd.print(" ");

    if(monthDay < 10){
      lcd.print("0");
      lcd.print(monthDay);
    }
    else{
      lcd.print(monthDay);
    }

    lcd.print(".");

    if(month < 10){
      lcd.print("0");
      lcd.print(month);
    }
    else{
      lcd.print(month);
    }

    lcd.print(".");

    lcd.print(year + 2000);
    lcd.setCursor(0, 1);

    if(hour < 10){
      lcd.print("0");
      lcd.print(hour);
    }
    else{
      lcd.print(hour);
    }

    lcd.print(":");

    if(minute < 10){
      lcd.print("0");
      lcd.print(minute);
    }
    else{
      lcd.print(minute);
    }

    lcd.print(":");

    if(second < 10){
      lcd.print("0");
      lcd.print(second);
    }
    else{
      lcd.print(second);
    }
  }
}

long previousSERIALms = 0;
long updateSerial = 1000; // in ms

void printDateAndTimeToSerial(){

  unsigned long currentMillis = millis();

  if(currentMillis - previousSERIALms > updateSerial) {
    // save the last time you blinked the LED 
    previousSERIALms = currentMillis; 

    getDataFromRTC();

    Serial.print(weekDayToString(weekDay));
    Serial.print(" ");
    Serial.print(monthDay);
    Serial.print(".");
    Serial.print(month);
    Serial.print(".");
    Serial.print(year);
    Serial.print(" ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(second);

  }
}

void getDataFromRTC(){
  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
  weekDay = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  monthDay = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read()); 
}

String weekDayToString(int day){
  String s = "";

  switch(day){
  case 1:
    s = "Sun";
    break;
  case 2:
    s = "Mon";
    break;
  case 3:
    s = "Tue";
    break;
  case 4:
    s = "Wed";
    break;
  case 5:
    s = "Thu";
    break;
  case 6:
    s = "Fri";
    break;
  case 7:
    s = "Sat";
    break;
  }
  return s;
}

byte decToBcd(byte val){
  // Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  {
  // Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}


void setDateAndTime(){

  byte second =      05; //0-59
  byte minute =      28; //0-59
  byte hour =        9; //0-23
  byte weekDay =     1; //1-7
  byte monthDay =    27; //1-31
  byte month =       10; //1-12
  byte year  =       13; //0-99

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //stop Oscillator

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekDay));
  Wire.write(decToBcd(monthDay));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));

  Wire.write(zero); //start 

  Wire.endTransmission();

}



















































