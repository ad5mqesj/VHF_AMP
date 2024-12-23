#include <LiquidCrystal.h>

//display pins
const int rs = 12, en = 11, d4 = 7, d5 = 8, d6 = 9, d7 = 10;

//digital pins
const int TRrelay = 2;
const int fans = 3;  //PWM
const int BiasOff = 4;
const int KeyIn = 5;
const int KeyOut = 6;
const int Fault = 13;

//analog pins
const int temp  = A0;
const int Idd   = A1;
const int Vdd   = A2;
const int ifwd  = A3;
const int irefl = A4;

//initialize the display library with the numbers of the interface pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
    lcd.begin(20, 4);
    pinMode (TRrelay, OUTPUT);
    pinMode (fans, OUTPUT);
    pinMode (BiasOff, OUTPUT);
    pinMode (KeyIn, INPUT);
    pinMode (KeyOut, OUTPUT);
    pinMode (Fault, OUTPUT);
    
    digitalWrite (TRrelay, 0);   //relay NO - bypass
    analogWrite (fans, 0);       //fan off
    digitalWrite (BiasOff, 0);   //bias on
    digitalWrite (KeyOut, 1);    //key out unkeyed

//    analogReadResolution(14);
    lcd.setCursor(0, 0);
    lcd.print("500W 2m Amplifier");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print(millis() / 1000);
}