#include <LiquidCrystal.h>

#define DIAGPRINT

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
const int itemp  = A0;
const int iIdd   = A1;
const int iVdd   = A2;
const int ifwd  = A3;
const int irefl = A4;

const int SAMPLES = 10;

void setup()
{
    //initialize the display library with the numbers of the interface pins
    LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
    Serial.begin(9600);
    analogReference(INTERNAL);

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

    lcd.setCursor(0, 0);
    lcd.print("500W 2m Amplifier");
#ifdef DIAGPRINT  
    Serial.println("Exiting setup."); 
#endif
}

void loop() {
  int rawTemp=0, rawIdd=0, rawVdd=0, rawFwd=0, rawRef=0;
  float Tave = 0.0, Vave=0.0, Iave=0.0, Fave=0.0, Rave=0.0;
  float rawVoltage;
  float Temp, Vdd, Idd, Fwd, Ref, Swr;
  //sample all analog input SAMPLES times and average
  for (int i = 0; i < SAMPLES; i++)
  {
    rawTemp = analogRead(itemp);
    Tave += rawTemp;
    rawIdd = analogRead(iIdd);
    Iave  += rawIdd;
    rawVdd = analogRead(iVdd);
    Vave + rawVdd;
    rawFwd = analogRead(ifwd);
    Fave += rawFwd;
    rawRef = analogRead(irefl);
    Rave += rawRef;
  }
  //compute averages and scale to Engineering units
  Tave = Tave / (float)SAMPLES;
  rawVoltage = Tave*INTERNAL / 1023.0;
  Temp = rawVoltage * 100.0;
#ifdef DIAGPRINT  
  Serial.println(Temp,3);
#endif

  Iave = Iave / (float)SAMPLES;
  rawVoltage = Iave*INTERNAL / 1023.0;
  Idd = (rawVoltage - 2.5)/12.5;    //nom 25A at 4.5v out, 0 at 2.5 v out; voltages between 0.5 and 2.5 indicate reverse polarity
#ifdef DIAGPRINT  
  Serial.println(Idd,3);
#endif

  Vave = Vave / (float)SAMPLES;
  rawVoltage = Vave*INTERNAL / 1023.0;
  Vdd = rawVoltage*1.0E5/5100.0 + rawVoltage;    //adjust to scale 
#ifdef DIAGPRINT  
  Serial.println(Vdd,3);
#endif

  Fave = Fave / (float)SAMPLES;
  rawVoltage = Fave*INTERNAL / 1023.0;
  Fwd = rawVoltage;    //adjust to scale 
#ifdef DIAGPRINT  
  Serial.println(Fwd,3);
#endif

  Rave = Rave / (float)SAMPLES;
  rawVoltage = Rave*INTERNAL / 1023.0;
  Ref = rawVoltage;    //adjust to scale 
#ifdef DIAGPRINT  
  Serial.println(Ref,3);
#endif

    float num = Fwd + Ref;
    float denom = Fwd - Ref;
    if (denom < 1E-3)
      Swr = 0.0;
    else  
      Swr = num / denom;
#ifdef DIAGPRINT  
  Serial.println(Swr,3);
#endif

}