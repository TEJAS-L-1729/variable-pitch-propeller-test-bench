
#include <LiquidCrystal_I2C.h>// include the library code
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address and the number of columns and rows
 
//pins:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin
 
//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
 
const int calVal_eepromAdress = 0;
unsigned long t = 0;
 
void setup() {
    // set up the LCD's number of columns and rows:  
  Serial.begin(9600); delay(10);
  Serial.println();
  Serial.println("Starting...");
 
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 409.68; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom
 
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
}
 
void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity
  
  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;
    float i = LoadCell.getData();
    Serial.println("Weight(g): " + String(int(i)));
    delay(15);
  
}