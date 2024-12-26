#include <LiquidCrystal.h>

#define DIAGPRINT
#define COMMAND_BUFFER_SIZE 50

//display pins
const int rs = 12, en = 11, d4 = 7, d5 = 8, d6 = 9, d7 = 10;

//digital pins
const int pTRrelay = 2;
const int pFans = 3;  //PWM
const int pBiasOff = 4;
const int pKeyIn = 5;
const int pKeyOut = 6;
const int pFault = 13;

//analog pins
const int itemp  = A0;
const int iIdd   = A1;
const int iVdd   = A2;
const int ifwd  = A3;
const int irefl = A4;

const int SAMPLES = 10;

//global vars - saves stack
const float Kp = 1.0, Ki = 0.5, Kd = 0.01, gain = 1.0, TempSetp = 35.0;
float errSum = 0.0, lastTemp = 0.0;
unsigned long lastUpdate = 0;
float Temp, Vdd, Idd, Fwd, FwdMax, Ref, Swr;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool tempFault = false, swrFault = false, Transmit = false; 

char control_port_buffer[COMMAND_BUFFER_SIZE];
int control_port_buffer_index = 0;
unsigned long last_serial_receive_time = 0;

void setup()
{
    //initialize the display library with the numbers of the interface pins
    Serial.begin(115200);
    analogReference(INTERNAL);

    lcd.begin(20, 4);
    pinMode (pTRrelay, OUTPUT);
    pinMode (pFans, OUTPUT);
    pinMode (pBiasOff, OUTPUT);
    pinMode (pKeyIn, INPUT_PULLUP);
    pinMode (pKeyOut, OUTPUT);
    pinMode (pFault, OUTPUT);
    
    digitalWrite (pTRrelay, 0);   //relay NO - bypass
    analogWrite  (pFans, 0);      //fan off
    digitalWrite (pBiasOff, 0);   //bias on
    digitalWrite (pKeyOut, 1);    //key out unkeyed (Active LOW)
    digitalWrite (pFault, 1);     //No Fault (Active LOW)

    lcd.setCursor(0, 0);
    lcd.print("500W 2m Amplifier");
    delay(3000);
#ifdef DIAGPRINT  
    Serial.println("Exiting setup."); 
#endif
}

void loop() {
 
  getSensors();
  handleFaults();
  handleKeyIn();
  handleDiaplay();
  fanControl();
  checkSerial();
  delay(50);
}//main loop

void getSensors(){
  int rawTemp=0, rawIdd=0, rawVdd=0, rawFwd=0, rawRef=0;
  float Tave = 0.0, Vave=0.0, Iave=0.0, Fave=0.0, Rave=0.0;
  float rawVoltage;

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

  //temp fault with hysteresis
  if (Temp > 64.0)
    {
      tempFault = true;
    }
  else if (Temp < 55.0) //temp must drop by at least 9 degrees
    {
      tempFault = false;
    }
#ifdef DIAGPRINT  
  Serial.print("Temp=");
  Serial.println(Temp,3);
#endif
  Iave = Iave / (float)SAMPLES;
  rawVoltage = Iave*INTERNAL / 1023.0;
  Idd = (rawVoltage - 2.5)/12.5;    //nom 25A at 4.5v out, 0 at 2.5 v out; voltages between 0.5 and 2.5 indicate reverse polarity
#ifdef DIAGPRINT  
  Serial.print("Idd=");
  Serial.println(Idd,3);
#endif
  Vave = Vave / (float)SAMPLES;
  rawVoltage = Vave*INTERNAL / 1023.0;
  Vdd = rawVoltage*1.0E5/5100.0 + rawVoltage;    //adjust to scale 
#ifdef DIAGPRINT  
  Serial.print("Vdd=");
  Serial.println(Vdd,3);
#endif
  Fave = Fave / (float)SAMPLES;
  rawVoltage = Fave*INTERNAL / 1023.0;
  Fwd = rawVoltage;    //adjust to scale 
  if (FwdMax < Fwd)
    FwdMax = Fwd;
#ifdef DIAGPRINT  
  Serial.print("vFwd=");
  Serial.println(Fwd,3);
#endif
  Rave = Rave / (float)SAMPLES;
  rawVoltage = Rave*INTERNAL / 1023.0;
  Ref = rawVoltage;    //adjust to scale 
#ifdef DIAGPRINT  
  Serial.print("vRef=");
  Serial.println(Ref,3);
#endif
  float num = Fwd + Ref;
  float denom = Fwd - Ref;
  if (denom < 1E-3 || !Transmit)
    Swr = 0.0;
  else  
    Swr = num / denom;

  if (Transmit && Swr > 3.0)  //if swr high and transmitting fault
    {
      swrFault = true;
    }
  
#ifdef DIAGPRINT  
  Serial.print("Swr=");
  Serial.println(Swr,3);
#endif
}//getSensors()

void handleDiaplay(){
  char temps[50];
  //temp, Idd, Vdd
  lcd.setCursor(0, 0);
  lcd.print("                    "); //erase line
  lcd.setCursor(0, 0);
  lcd.print("T=");
  dtostrf(Temp, 4,1,temps);
  lcd.print(temps);
  lcd.print(" ");
  lcd.print("I=");
  dtostrf(Idd, 4,1,temps);
  lcd.print(temps);
  lcd.print(" ");
  lcd.print("V=");
  dtostrf(Vdd, 4,1,temps);
  lcd.print(temps);

  //power
  lcd.setCursor(0, 1);
  lcd.print("                    "); //erase line
  lcd.setCursor(0, 1); //go back to start
  lcd.print("F= ");
  dtostrf(FwdMax, 5,1,temps);
  lcd.print(temps);
  if (Transmit)
  {
    lcd.print("  ");
    lcd.print("R= ");
    dtostrf(Ref, 4,1,temps);
    lcd.print(temps);
  }
  //tr status and swr
  lcd.setCursor(0, 2);
  lcd.print("                    "); //erase line
  lcd.setCursor(0, 2);
  if (Transmit)
  {
    lcd.print("Transmit");
    lcd.print("    ");
    lcd.print("SWR  ");
    dtostrf(Swr, 3,1,temps);
    lcd.print(temps);
    lcd.print(":1");
  }
  else
  {
    lcd.print("Receive"); //erase line
  }


  //fault indicators
  lcd.setCursor(0, 3);
  lcd.print("                    "); //erase line
  lcd.setCursor(0, 3);
  if (tempFault)
  {
    lcd.print("OVERTEMP  ");
  }
  if (swrFault)
  {
    lcd.print("HIGH SWR");
  }
  else if (!tempFault) //since its else applied to swr fault it actually tests both faults
  {
    lcd.print("                    "); //erase line
  }
  
}//handleDiaplay()

void handleFaults(){
  if (!tempFault && !swrFault) //bail at once if no fault
    {
    setFault(false);
    return;
    }
  Transmit = false;
  //drop keyout first -hopefully input then ceases
  setKeyOut(); //drop keyout and turn off bias
  setTRrelays(); //switch relays
  setFault(true);
}//handleFaults()

void handleKeyIn(){
  int keyed = digitalRead(pKeyIn);
  if (!tempFault && !swrFault && keyed == 0) //ignore if faulted
    {
      Transmit = true;
    }
  else if (keyed == 1)
    {
      Transmit = false;
    }
  setTRrelays();  //relays first, wait settle time then turn on bias and set keyout
  setKeyOut();
}//handleKeyIn

void setTRrelays(){
  if (Transmit)
    digitalWrite (pTRrelay, 1);
  else
    digitalWrite (pTRrelay, 0);
  delay(250); //let relays settle
}//setTRrelays

void setKeyOut(){
  if (Transmit)
  {
    digitalWrite (pBiasOff, 0);   //bias on
    digitalWrite (pKeyOut, 0);  //active low
  }
  else
    {
    digitalWrite (pKeyOut, 1);  //active low
    }
  delay(50);  //wait for tranmistter to stop
}//setKeyOut

void setFault(bool fault){
  if (fault)
    {
    digitalWrite (pBiasOff, 1);   //bias off
    digitalWrite (pFault, 0);     //(Active LOW)
    }
  else
  {
    digitalWrite (pFault, 1);     //No Fault
  }
}//setFault

void fanControl(){
  if (Temp > TempSetp)
  {
    analogWrite  (pFans, 0); 
    return;           //do nothing if cool
  }
  float err = Temp - TempSetp;
  if (err < 1.0)
    {
    err = 0.0;
    return;
    }
  float pTerm = err * Kp;
  float iTerm = 0.0, dterm = 0.0;
  float partialSum = 0.0f;
  unsigned long nowTime = millis();

  if (lastUpdate != 0)
  {
      int dT = nowTime - lastUpdate;
      //compute integral
      partialSum = errSum + dT * err; 
      iTerm = Ki * partialSum;
      //and derivative
      if (dT > 0)
        dterm = Kd * (Temp - lastTemp) / dT;
  }

  lastUpdate = nowTime;
  errSum = partialSum;
  lastTemp = Temp;

  float outReal = (pTerm + iTerm + dterm)*gain;
  int out = (int)(outReal+0.5); //round up
  if (out < 0) out = 0;
  ///!!! MAX FAN should be 192 so they are not running overvoltage
  if (out > 192) out = 192;
  analogWrite  (pFans, out);
}//fanControl()

void clear_command_buffer()
{
  control_port_buffer_index = 0;
  memset(control_port_buffer, 0, sizeof(control_port_buffer));
}//clear_command_buffer

void checkSerial()
{
  int incomingByte = 0;
  int av = Serial.available();

  if (av > 0)
  {
    incomingByte = Serial.read();
    last_serial_receive_time = millis();
    if ((incomingByte != '\n') && (incomingByte != '\r') && (incomingByte != ' ')) {
       control_port_buffer[control_port_buffer_index++] = (char)incomingByte;
    }
    else{
#ifdef DIAGPRINT
        Serial.println("Calling processCommand()");
#endif
      processCommand();
      clear_command_buffer();
    }
  }//if a byte is there
  else
  {
#ifdef DIAGPRINT
//      Serial.println("no input");
#endif
  }
}//checkSerial

void processCommand(){
#ifdef DIAGPRINT
  Serial.print("process command : ");
  Serial.println((char)control_port_buffer[0]);
#endif

  switch (control_port_buffer[0]) {
    case 'T':
    case 't':
      if (!tempFault && !swrFault) //ignore if faulted
        {
#ifdef DIAGPRINT
  Serial.println("Changing State to transmit");
#endif
          Transmit = true;
          setKeyOut();
          setTRrelays();
        }
      break;

    case 'R':
    case 'r':
#ifdef DIAGPRINT
  Serial.println("Changing State to receive");
#endif
      Transmit = false;
      setKeyOut();
      setTRrelays();
      break;

    case 'C':
    case 'c':
#ifdef DIAGPRINT
  Serial.println("Clear faults");
#endif
      Transmit = false;
      setFault(false);
      break;

    default:
        break;
  }//switch command char
}//processCommand
