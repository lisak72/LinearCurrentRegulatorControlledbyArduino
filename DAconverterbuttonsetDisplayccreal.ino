//DA converter MCP4921                BUTTONS + DISPLAY
//pins of arduino nano
// v konstrukci> pro vstupni piny, pokud se pouzijou 3, pouzit 2,3,X, protoze na 23 jsou
//preruseni. A4, A5 jsou na twi viz nize. Asi bude vhodne je vyvest na nejaky konektor.
// DAC montovat co nejblize OZ. Vstupni piny pres odpor zemnit. Vystup DAC mozna tez.
// umistit monitorovaci ledku, mozna nejlepe k DAC (pres velky odpor), nebo extra jako stavovou
// zpatky privest monitorovaci + kalibracni pin na vstup nekam 
//v.20170320

//SPI DEFINE
// pin připojen na CS (2)
// SPI na arduinu
byte latchPin = 10;
//Pin connected to SCK clock (3)
// SCK na arduinu
byte clockPin = 13;
////Pin připojen na SDI data pin (4)
// MOSI na arduinu
byte dataPin = 11;

//TWI DEFINE - twi communication with second arduino
// twiPinTX=A4;
// twiPinCLK=A5;
// toto Nano je master, doptava se na hodnoty

//piny pro cteni nastavene urovne
// LEVEL TRIGGERS INPUT PINS
byte LLp=7;
byte MLp=8;
byte HLp=12;

//LED diag pin
byte LED=9;

//CHANELL ON/OF PINS
//swith sense resistors
// CH1 0-150mA, CH2 up to 0.5A, CH3 up to 1A
byte CH1=4;
byte CH2=5;
byte CH3=6;

//USER INPUT PINS
byte SW1=2;
byte SW2=3;

//FEEDBACK CALIBRATION INPUT
// fedbackPin=A0

volatile unsigned long lastint=0, lasttime=0, presstime=0;

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  // Display which does not send AC

void setup() {
 


  pinMode(latchPin, OUTPUT); //RCLK 
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
  

//attachInterrupt(digitalPinToInterrupt(SW2),krok,FALLING);

/*
attachInterrupt(digitalPinToInterrupt(LLp),LLup,RISING);
attachInterrupt(digitalPinToInterrupt(MLp),MLup,RISING);
attachInterrupt(digitalPinToInterrupt(HLp),HLup,RISING);
*/ 


SPI.beginTransaction(SPISettings(20000000, MSBFIRST,SPI_MODE0));
SPI.begin();
//Wire.begin();        // join i2c bus (address optional for master)


}  //end of setup part

//DISPLAY
//kod z https://www.arduinotech.cz/inpage/wifi-teplomer-385/
void displ(String st) 
{
  u8g.setFont(u8g_font_7x14);
  u8g.setPrintPos(0, 20); 
  // call procedure from base class, http://arduino.cc/en/Serial/Print
  u8g.print(st);
  //u8g.setFont(u8g_font_fub20);
  //u8g.setPrintPos(10,50); 
  //u8g.print("Ahoj!!!");
  //u8g.drawLine(0,60,128,60);

}

void displ3(String st1, String st2, String st3){
  u8g.setFont(u8g_font_7x14);
  u8g.setPrintPos(0, 20); 
  u8g.print(st1);
  u8g.setPrintPos(0, 40); 
  u8g.print(st2);
  u8g.setPrintPos(0, 60); 
  u8g.print(st3);
}

void wd(String str){
  u8g.firstPage();  
  do {
    displ(str);
  } while( u8g.nextPage() );
}

void wd3(String str1,String str2, String str3 ){
  u8g.firstPage();  
  do {
    displ3(str1, str2, str3);
  } while( u8g.nextPage() );
}

void initialSetting(){
  digitalWrite(CH1, LOW);
  digitalWrite(CH2, LOW);
  digitalWrite(CH3, LOW);
  digitalWrite(A1, LOW);
  SetI(0);
 // BlinkInternalLed(3);
}


void WriteConverter(int eb){  //using SPI
  word bitint=(word)eb;
  byte data;
  data=highByte(bitint);
  data=0b00001111 & data;
  data=0b00110000 | data;   
  SPI.transfer(data);
  SPI.transfer(lowByte(bitint));
}

/*
void WriteConverterSHFT(int eb){  //za pouziti shiftOut, musi se deaktivovat SPI
  // a je to vyrazne pomalejsi (nevim proc)
//  digitalWrite(latchPin, LOW);
  word bitint=eb;
  byte data;
  data=highByte(bitint);
  data=0b00001111 & data;
  data=0b00110000 | data;   
  shiftOut(dataPin, clockPin, MSBFIRST, data);
  shiftOut(dataPin, clockPin, MSBFIRST, (lowByte(bitint)));
//  digitalWrite(latchPin, HIGH);
}
*/

void Apply(){
  digitalWrite(latchPin, HIGH); 
  digitalWrite(latchPin, LOW);     
}


void SetI(int lvl){  //set level on DAC
  WriteConverter(lvl);
  Apply();
}

void BlinkInternalLed(int pocet){
  for(int i=0; i<pocet; i++){
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
  delay(300);
  }
}

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

void BlinkInternalLedShort(int pocet){
  for(int i=0; i<pocet; i++){
  digitalWrite(LED, HIGH);
  delay(20);
  digitalWrite(LED, LOW);
  delay(20);
  }
}

word TWIRequestValue(){
  Wire.requestFrom(8,2);    // request from slave device #8, 2 byte expected
  while (Wire.available()) { // slave may send less than requested
  byte hb,lb;
  hb = Wire.read();  // receive 1 byte per cycle
  lb = Wire.read();
  return(word(hb,lb));
}
}

void allDown(){
  digitalWrite(CH1,LOW);
  digitalWrite(CH2,LOW);
  digitalWrite(CH3,LOW);
  SetI(0);
}

void cooling(){
  allDown();
  for(unsigned int i=0;i<10;i++){
    delay(1000);  //10s
  }
}

word readValueTWI(){
  return TWIRequestValue();
}

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
  

bool writeEEw(byte pos, word val){ //pos must be even-numbered (need 2 bytes)
  if(pos%2!=0) return 0;
  EEPROM.update(pos,(byte)val); //need #include <EEPROM.h>
  EEPROM.update(pos+1,(byte)(val>>8));
  if(val!=(EEPROM.read(pos) + ((EEPROM.read(pos+1)) << 8))) return 0;
  return 1;
}

word readEEw(byte pos){
  word eew;
  eew=((EEPROM.read(pos)) + ((EEPROM.read(pos+1)) << 8));
  if(eew<4096) return eew;
}

bool storeEE(word ll, word ml, word hl, word ch){
  bool ret;
  ret=writeEEw(0,ll);
  ret=writeEEw(2,ml);
  ret=writeEEw(4,hl);
  ret=writeEEw(6,ch);
  return ret;
}

float cc(word das, word ch){  //COMPUTE CURRENT , returns mA. Parameter is set bit value and channel number
  if(ch==1) return((0.02895*das));
  if(ch==2) return((0.1091*das));
  if(ch==3) return((0.2433*das));
}

float ccreal(word ar, word ch){  //COMPUTE CURRENT IN DEPEND ON FEEDBACK PIN READ, returns mA. First param is for compatibility, second parameter is channel number.
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

/*
void readValuesTWI(){
  //THIS SECTION REQUESTED SLAVE ARDUINO FOR 3 VALUES
while(LL==0 && ML==0 && HL==0){
LL=TWIRequestValue();
ML=TWIRequestValue();
HL=TWIRequestValue();
}
}
*/



//LL,ML,HL - word values for setting current level (0-4095)
//chs - select channel (1 or 2 or 3) - main current level
volatile word stp=0;
word LL=0,ML=0,HL=0,chan=1,chs=0;
bool alreadyReaded=0, resetStarted=0;

void loop() {
  wd("init");
  initialSetting();
  delay(1000);
  
  //read EEPROM only once
if(alreadyReaded==0){
LL=readEEw(0);
ML=readEEw(2);
HL=readEEw(4);
chan=readEEw(6);
  if(chan>3 || chan<1){ writeEEw(6,1); panic();} //first run on new arduino or panic
alreadyReaded=1;
wd3("LL: "+String(LL)+"  CH:"+String(chan),"ML: "+String(ML), "HL: "+String(HL));
}
delay(5000);
wd("press for setup");
BlinkInternalLed(10);
 if((!(digitalRead(SW2)))&&(!(digitalRead(SW1)))) {   //both pressed on startup, total reset+select channel
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
                }   //end of reset+channel select
                
  initialSetting();
    //channel setting
    if(chan==1) chs=CH1;
    if(chan==2) chs=CH2;
    if(chan==3) chs=CH3;
    //for safe
    if(chan!=1 && chan!=2 && chan!=3) chs=CH1;
    digitalWrite(chs,HIGH);  //selected channel activate
  //setting levels by buttons



digitalWrite(chs,HIGH);  //selected channel activate

if((!(digitalRead(SW2))) || resetStarted){//if SW2 pressed on startup  START SETTINGS 
  BlinkInternalLed(1);
  lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 5s pokud neni upraveno lasttime
    if((increase()==1)&&(LL<4094)) LL++;
    if((increase()==-1)&&(LL>0)) LL--;
    SetI(LL);
    if((LL%10==0)||(millis()-presstime<200)) wd3("LL: "+String(LL),"I= "+String(cc(LL,chan))+" [mA]","Im= "+String(ccreal(LL,chan),0)+" [mA]");
    delay(10);
  }
   BlinkInternalLed(2);
   lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 5s pokud neni upraveno lasttime
    if((increase()==1)&&(ML<4094)) ML++;
    if((increase()==-1)&&(ML>0)) ML--;
    SetI(ML);
    if((ML%10==0)||(millis()-presstime<200)) wd3("ML: "+String(ML),"I= "+String(cc(ML,chan))+" [mA]","Im= "+String(ccreal(ML,chan),0)+" [mA]");
    delay(10);
  }
   BlinkInternalLed(3);
   lasttime=millis();
  while((millis()-lasttime)<15000){
    //nastavovani - bezi 5s pokud neni upraveno lasttime
    if((increase()==1)&&(HL<4094)) HL++;
    if((increase()==-1)&&(HL>0)) HL--;
    SetI(HL);
    if((HL%10==0)||(millis()-presstime<200)) wd3("HL: "+String(HL),"I= "+String(cc(HL,chan))+" [mA]","Im= "+String(ccreal(HL,chan),0)+" [mA]");
    delay(10);
  }
  BlinkInternalLed(5);
  
while(digitalRead(SW2)){ //and SHOW SETTINGS while dont agree
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
digitalWrite(A1, HIGH); //start Arduino2
//if(digitalRead(MLp)||digitalRead(HLp)||digitalRead(LLp)) panic();
for(;;){ //MAIN LOOP PUT HERE
if(digitalRead(MLp)) SetI(ML);
  else if(digitalRead(HLp)) SetI(HL);
        else if(digitalRead(LLp)) SetI(LL);
           else SetI(0);
  
}



} //end of main loop

/*
void krok(){
  if((millis()-lastint)>500){ 
  LL=LL+stp;
  BlinkInternalLed(1);
  lastint=millis();
  }
}
*/
