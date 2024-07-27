/* Program ArduINA226 to current, voltage and power
 with Arduino Nano and INA226 module
uses Korneliusz Jarzebski Library for INA226 (modified)
save measurements on SD if present
Giovanni Carrera, 14/03/2020
 */

#include <SPI.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <SD.h>
#include <INA226.h>
// LCD pins
#define rs 7
#define en 6
#define d4 5
#define d5 4
#define d6 A2
#define d7 A3
#define SSbutton 2
#define SD_CS 10
// initialize the library by associating any needed LCD interface pin
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
INA226 ina;


char bline[17] = "                ";// blank line
const int deltat= 500;// sampling period in milliseconds
unsigned long cmilli, pmilli;
boolean SDOk = true, FileHeader = true, ACQ = false;
unsigned int ns=0;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  pinMode(SSbutton, INPUT);// Start/Stop button
  pinMode(SD_CS, OUTPUT);// SD chip select
  // Default INA226 address is 0x40
  ina.begin(0x40); 
  lcd.print("ArduINA226");
  lcd.setCursor(0, 1);// print on the second row
  lcd.print("Power Monitor");
  // Configure INA226
  ina.configure(INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
  // Calibrate INA226. Rshunt = 0.004  ohm, Max expected current = 20.48 A
  ina.calibrate(0.004, 20.48);
  ina.enableShuntOverLimitAlert();// enable Shunt Over-Voltage Alert, current over the limit
  ina.setShuntVoltageLimit(0.08); // current limit = 20 A for 0.004 ohms
  ina.setAlertLatch(true);
  if (!SD.begin(SD_CS)) {
    LCDprintLine("SD not present!", 1);
    delay(5000);
    SDOk = false;
  }
  LCDprintLine("Push to Start", 1);
  while (digitalRead(SSbutton) == HIGH) {};// wait for start
  if (SDOk) ACQ = true;
}


void loop(){
  cmilli = millis();
  if (cmilli - pmilli > deltat) {
    pmilli = cmilli;
    float volts= ina.readBusVoltage();
    float current = ina.readShuntCurrent();
    LCDprintLine("V=", 0);// print bus voltage
    lcd.print(volts,3);  
    float power = ina.readBusPower();// INA calculate power 
//    float power = volts * current;// Arduino calculate power
    lcd.print(" W=");// print power
    lcd.print(power,4);
    float Vshunt= ina.readShuntVoltage();
    String dataString = String(volts,3)+','+String(current,4)+','+String(power,4)+','+String(Vshunt,6);
    if (ACQ){
      File dataFile = SD.open("powerlog.csv", FILE_WRITE);      
      if (dataFile) {
        if (FileHeader){// print file header
          dataFile.print("Deltat [ms] = ");
          dataFile.println(deltat);
          dataFile.println("Vbus[V], Ishu [A], P [W], Vshu [V]");
          FileHeader = false;
        }
        dataFile.println(dataString);
        ns++;// number of acquired samples
        dataFile.close();
        if (digitalRead(SSbutton) == LOW && ns>=10){ //stop after at least 10 samples
          ACQ = false;// stop acquisition
          LCDprintLine(String(ns), 1);
          lcd.print(" samples");
          delay(5000);         
        }
      } else {
        LCDprintLine("Can't open file!", 1);
        ACQ = false;
        delay(5000);
      }
    }
    if (ina.isAlert()) {
      LCDprintLine("Shunt V=", 0);
      lcd.print(Vshunt,5);
      LCDprintLine("SOL ALERT", 1);
    }
    else {
      LCDprintLine("I= ", 1);
      lcd.print(current,3);
      if (ACQ){
        dataString = " N=" + String(ns);
        lcd.print(dataString);// print number of acquired sample
      }      
    }
  }
}
/************************** Functions **************************/
void LCDprintLine(String text, byte line){
   lcd.setCursor(0, line);
   lcd.print(bline);// clear the second row
   lcd.setCursor(0, line);
   lcd.print(text);// print text
}