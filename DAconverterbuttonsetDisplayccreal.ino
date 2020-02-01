//DA MCP4921               
//A4, A5 pouzity pro twi 
//v.20170325

//SPI - ovladani MCP4921
// pin připojen na CS (2)
byte latchPin = 10;
// SCK 
byte clockPin = 13;
//Pin připojen na SDI data pin (4)
// MOSI na arduinu
byte dataPin = 11;

//TWI twi komunikace s displejem
// twiPinTX=A4;
// twiPinCLK=A5;

//piny pro cteni nastavene urovne
byte LLp=7;
byte MLp=8;
byte HLp=12;

//diagnosticka LED pin
byte LED=9;

//piny spinajici jednotlive kanaly
// CH1 0-125mA, CH2 up to 0.5A, CH3 up to 1A
byte CH1=4;
byte CH2=5;
byte CH3=6;

//tlacitka
byte SW1=2; //leve tlacitko
byte SW2=3; //prave tlacitko

//pin A0 - mereni ubytku na rezistorech
// fedbackPin=A0

volatile unsigned long lastint=0, lasttime=0, presstime=0;

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include "U8glib.h"

//definice pro displej
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);

void setup() {

  pinMode(latchPin, OUTPUT); 
  pinMode(LLp, INPUT);
  pinMode(MLp, INPUT);
  pinMode(HLp, INPUT);
  pinMode(A0, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(CH3, OUTPUT);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(A1, OUTPUT);

SPI.beginTransaction(SPISettings(20000000, MSBFIRST,SPI_MODE0));
SPI.begin();
}  //konec setupu

//odeslani jednoho radku na displej
void displ(String st) 
{
  u8g.setFont(u8g_font_7x14);
  u8g.setPrintPos(0, 20); 
  // http://arduino.cc/en/Serial/Print
  u8g.print(st);
}

//odeslani 3 radku na displej
void displ3(String st1, String st2, String st3){
  u8g.setFont(u8g_font_7x14);
  u8g.setPrintPos(0, 20); 
  u8g.print(st1);
  u8g.setPrintPos(0, 40); 
  u8g.print(st2);
  u8g.setPrintPos(0, 60); 
  u8g.print(st3);
}

//zobrazovani 1 radku na displeji
void wd(String str){
  u8g.firstPage();  
  do {
    displ(str);
  } while( u8g.nextPage() );
}

//zobrazovani 3 radku na displeji
void wd3(String str1,String str2, String str3 ){
  u8g.firstPage();  
  do {
    displ3(str1, str2, str3);
  } while( u8g.nextPage() );
}

//nastaveni hardwaru po spusteni
void initialSetting(){
  digitalWrite(CH1, LOW);
  digitalWrite(CH2, LOW);
  digitalWrite(CH3, LOW);
  digitalWrite(A1, LOW);
  SetI(0);
}

//zapis hodnoty na DA prevodnik
void WriteConverter(int eb){  // SPI
  word bitint=(word)eb;
  byte data;
  data=highByte(bitint);
  data=0b00001111 & data;
  data=0b00110000 | data;   
  SPI.transfer(data);
  SPI.transfer(lowByte(bitint));
}

//aktivace hodnoty na DA prevodniku
void Apply(){
  digitalWrite(latchPin, HIGH); 
  digitalWrite(latchPin, LOW);     
}

//odeslani hodnoty na DA a aktivace
void SetI(int lvl){  //nastavi hodnotu DAC
  WriteConverter(lvl);
  Apply();
}

//blikani diagnosticke ledky
void BlinkInternalLed(int pocet){
  for(int i=0; i<pocet; i++){
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
  delay(300);
  }
}

//neresitelna zavazna chyba
void panic(){ //panic
  wd("PANIC");
  for(;;){
  digitalWrite(LED, HIGH);
  delay(30);
  digitalWrite(LED, LOW);
  delay(30);
  }
}

void ledon(){
  digitalWrite(LED, HIGH);
}

void ledoff(){
  digitalWrite(LED, LOW);
}

//rychle blikani diagnostickou ledkou
void BlinkInternalLedShort(int pocet){
  for(int i=0; i<pocet; i++){
  digitalWrite(LED, HIGH);
  delay(20);
  digitalWrite(LED, LOW);
  delay(20);
  }
}

//vypnuti vystupu
void allDown(){
  digitalWrite(CH1,LOW);
  digitalWrite(CH2,LOW);
  digitalWrite(CH3,LOW);
  SetI(0);
}

//nastavovani hodnot pomoci tlacitek
short increase(){ //return 0 (nothing or both pressed) or 1 (SW2 pressed) or -1 (SW1 pressed)
   if((!(digitalRead(SW2)))&&(!(digitalRead(SW1)))) {lasttime=millis(); return 0;} //error both pressed
    if(!(digitalRead(SW2))) {
      lasttime=millis();
      return 1;
      }
   if(!(digitalRead(SW1))) {
    lasttime=millis();
    return -1;
    }
  presstime=millis();  
  return 0;
}
  
//zapis do EEPROM, vraci 1 kdyz ok
bool writeEEw(byte pos, word val){ //pos must be even-numbered (need 2 bytes)
  if(pos%2!=0) return 0;
  EEPROM.update(pos,(byte)val); 
  EEPROM.update(pos+1,(byte)(val>>8));
  if(val!=(EEPROM.read(pos) + ((EEPROM.read(pos+1)) << 8))) return 0;
  return 1;
}

//cteni z EEPROM
word readEEw(byte pos){
  word eew;
  eew=((EEPROM.read(pos)) + ((EEPROM.read(pos+1)) << 8));
  if(eew<4096) return eew;
}

//zapis tri hodnot a cisla kanalu do EEPROM
bool storeEE(word ll, word ml, word hl, word ch){
  bool ret;
  ret=writeEEw(0,ll);
  ret=writeEEw(2,ml);
  ret=writeEEw(4,hl);
  ret=writeEEw(6,ch);
  return ret;
}

//vypocet proudu podle hodnot stanovenych merenim
float cc(word das, word ch){  
  if(ch==1) return((0.02895*das));
  if(ch==2) return((0.1091*das));
  if(ch==3) return((0.2433*das));
}

//vypocet proudu podle hodnoty napeti odecteneho na rezistorech
//prvni parametr je jen kvuli kompatibilite v programu, druhy cislo kanalu
float ccreal(word ar, word ch){  
  unsigned int rd=0;
  for(byte i=0; i<10; i++){
    rd=rd+analogRead(A0);
  }
  rd=rd/10;
  float voltage=(((float)rd/1023)*4.6);
  
  if(ch==1) return((voltage/40)*1000);  //  I=U/R
  if(ch==2) return((voltage/10)*1000);
  if(ch==3) return((voltage/5)*1000);
  }

word LL=0,ML=0,HL=0,chan=1,chs=0;
bool alreadyReaded=0, resetStarted=0;

void loop() {
  wd("init");
  initialSetting();
  delay(1000);
  
  //cteni EEPROM pouze jednou (asi neni nutne)
if(alreadyReaded==0){
LL=readEEw(0);
ML=readEEw(2);
HL=readEEw(4);
chan=readEEw(6);
  if(chan>3 || chan<1){ writeEEw(6,1); panic();} //prvni pruchod na novem Arduinu nebo chyba
alreadyReaded=1;
wd3("LL: "+String(LL)+"  CH:"+String(chan),"ML: "+String(ML), "HL: "+String(HL));
}
delay(5000);
wd("press for setup");
BlinkInternalLed(10);
//obe tlacitka stisknuta - vyvola reset a nastavovani cisla kanalu
 if((!(digitalRead(SW2)))&&(!(digitalRead(SW1)))) { 
                SetI(25);  //nothing dangerous
                chan=1;
                BlinkInternalLedShort(10);
                lasttime=millis();
                while((millis()-lasttime)<15000){
                //nastavovani - bezi 5s pokud neni upraveno lasttime
                if((increase()==1)&&(chan<3)) chan++;
                if((increase()==-1)&&(chan>1)) chan--;                
                          if(chan==1) digitalWrite(CH1,HIGH);
                            else digitalWrite(CH1,LOW);
                          if(chan==2) digitalWrite(CH2,HIGH);
                            else digitalWrite(CH2,LOW);
                          if(chan==3) digitalWrite(CH3,HIGH);
                            else digitalWrite(CH3,LOW);
                BlinkInternalLedShort(chan);
                wd("channel: "+String(chan));
                delay(1000);
                            } 
                LL=0; ML=0; HL=0;
                resetStarted=1;
                }   //konec nastavování kanalu
                
  initialSetting();
    //nastaveni kanalu
    if(chan==1) chs=CH1;
    if(chan==2) chs=CH2;
    if(chan==3) chs=CH3;
    //for safe
    if(chan!=1 && chan!=2 && chan!=3) chs=CH1;
    digitalWrite(chs,HIGH);  //aktivace vybraneho kanalu
  //nastavovani urovni tlacitky



digitalWrite(chs,HIGH);  //aktivace vybraneho kanalu

if((!(digitalRead(SW2))) || resetStarted){//kdyz stisknuto SW2 pri startu 
  BlinkInternalLed(1);
  lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 15s pokud neni upraveno lasttime
    if((increase()==1)&&(LL<4094)) LL++;
    if((increase()==-1)&&(LL>0)) LL--;
    SetI(LL);
    if((LL%10==0)||(millis()-presstime<200)) wd3("LL: "+String(LL),"I= "+String(cc(LL,chan))+" [mA]","Im= "+String(ccreal(LL,chan),0)+" [mA]");
    delay(10);
  }
   BlinkInternalLed(2);
   lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 15s pokud neni upraveno lasttime
    if((increase()==1)&&(ML<4094)) ML++;
    if((increase()==-1)&&(ML>0)) ML--;
    SetI(ML);
    if((ML%10==0)||(millis()-presstime<200)) wd3("ML: "+String(ML),"I= "+String(cc(ML,chan))+" [mA]","Im= "+String(ccreal(ML,chan),0)+" [mA]");
    delay(10);
  }
   BlinkInternalLed(3);
   lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 15s pokud neni upraveno lasttime
    if((increase()==1)&&(HL<4094)) HL++;
    if((increase()==-1)&&(HL>0)) HL--;
    SetI(HL);
    if((HL%10==0)||(millis()-presstime<200)) wd3("HL: "+String(HL),"I= "+String(cc(HL,chan))+" [mA]","Im= "+String(ccreal(HL,chan),0)+" [mA]");
    delay(10);
  }
  BlinkInternalLed(5);
  
while(digitalRead(SW2)){ //prepina nastavene hodnoty, dokud neni stisknuto tl 2
BlinkInternalLed(2);
SetI(LL);  
delay(500);
SetI(ML);
delay(500);
SetI(HL);
delay(500);
wd("press for store");
}

if(!storeEE(LL,ML,HL,chan)) panic();
wd("EEPROM OK");
digitalWrite(LED,HIGH);
delay(2000);
digitalWrite(LED,LOW);
}     //END SETTINGS
SetI(0);
wd("MAIN LOOP");
digitalWrite(A1, HIGH); //signal pro start Synchronizacniho mikroprocesoru (Gabor)
for(;;){ //hlavni cyklus
if(digitalRead(MLp)) SetI(ML);
  else if(digitalRead(HLp)) SetI(HL);
        else if(digitalRead(LLp)) SetI(LL);
           else SetI(0);
}

} 


