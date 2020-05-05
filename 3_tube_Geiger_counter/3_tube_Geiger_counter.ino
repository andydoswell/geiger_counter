/* Triple tube Geiger Counter project
    (c) A.G.Doswell April 2020

    DOB-50 Tube for Beta/Gamma
    DOB-80 Tube for Beta/Gamma & X-rays
    CTC5 tube for Hard Beta & Gamma

    Hardware details can be found at andydoz.blogspot.com

    Compile with -02 switch for maximum performance.

*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
unsigned int spk = 4; // Speaker drive transistor connected to pin 4
unsigned int led = 5; // Battery Low/Counter LED connected to pin 5
unsigned int meter = 9;// Analogue meter drive transistor connected to pin 9
unsigned int battery = A0; // Pin used to monitor LiPo voltage
unsigned int DOB50 = 1; // Pin where DOB-50 output connects from Q3
unsigned int CTC5 = 0; // Pin where CTC5 output connects from Q5
unsigned int DOB80 = 7; // Pin where DOB80 connects from Q4
unsigned int peakMeterVal; // Peak read value for analogue meter
unsigned int DOB50peak;
unsigned int DOB80peak;
unsigned int CTC5peak;
int writeMeterVal;
unsigned loopcounter;
volatile boolean DOB50flag; // flags to show which tube has triggered
boolean DOB80flag;
boolean CTC5flag;
boolean DOB80HIGH; // flags used for "software" falling edge interrupt
boolean CTC5HIGH;
boolean DOB50HIGH;
unsigned long lastDOB50; //Timing of last pulses arrived from each tube
unsigned long lastDOB80;
unsigned long lastCTC5;
long DOB50CPM; // Counts per minute for each tube.
long DOB80CPM;
long CTC5CPM;
LiquidCrystal_I2C  lcd(0x3F, 16, 2);


void setup() {
  pinMode (spk, OUTPUT); // Set up pins
  pinMode (led, OUTPUT);
  pinMode (meter, OUTPUT);
  pinMode (battery, INPUT);
  pinMode (DOB50, INPUT);
  pinMode (CTC5, INPUT);
  pinMode (DOB80, INPUT);
  pinMode (0, INPUT);
  lcd.init();// start I2C LCD
  lcd.begin(16, 2); // set up the LCD as 16 x 2 display
  lcd.setBacklight(HIGH);
  lcd.clear();
  analogWrite (meter, 255);
  lcd.println(" 3 Tube Geiger  "); //splash screen
  lcd.setCursor(0, 1);
  lcd.println("    Counter     ");
  delay (1000);
  lcd.clear();
  lcd.print("andydoz.blogspot");
  lcd.setCursor(0, 1);
  lcd.print(".com     (c)2020");
  delay (1000);
  lcd.clear();
  analogWrite (meter, 0);
  lastDOB50 = millis(); //Update all timers for tubes
  lastDOB80 = millis();
  lastCTC5 = millis();

}

void loop() {
  if (!digitalRead (DOB80)) { // if the DOB80 tube gets a pulse
    DOB80HIGH = true;
  }
  if (DOB80HIGH && digitalRead(DOB80)) { //set the flag when it falls.
    DOB80flag = true;
    DOB80HIGH = false;
  }

  if (!digitalRead (CTC5)) { // if the CTC5 tube gets a pulse
    CTC5HIGH = true;
    //CTC5flag = true;
  }
  if (CTC5HIGH && digitalRead(CTC5)) {// set the flag when it falls.
    CTC5flag = true;
    CTC5HIGH = false;
  }

  if (!digitalRead (DOB50)) { // if the DOB80 tube gets a pulse
    DOB50HIGH = true;
  }
  if (DOB50HIGH && digitalRead(DOB50)) { //set the flag when it falls.
    DOB50flag = true;
    DOB50HIGH = false;
  }

  if (DOB50flag || DOB80flag || CTC5flag) { //if any tube gets a pulse, update the results
    processResults();
  }
  else { //only update the display when there's no pulses, and the loop counter's been round a while.
    if (loopcounter >= 10000) {
      updateLCD();
      loopcounter = 0;
    }
  }
  loopcounter++;
}

void tick() {
  digitalWrite (spk, HIGH); // make the speaker click, and flash the LED.
  digitalWrite (led, HIGH);
  digitalWrite (led, LOW);
  digitalWrite (spk, LOW);
}

float BatteryCheck() { //get the battery voltage

  int sensorValue = analogRead(battery);
  float voltage = sensorValue * (5.0 / 1023.0);
  return voltage;
}

void DOB50detect() { //ISR for DOB50 tube
  DOB50flag = true;
}

void processResults () {
  if (DOB50flag) {
    DOB50CPM = (60000 / (millis() - lastDOB50)) ; // Calculate counts per minute.
    tick();
    lastDOB50 = millis();
    DOB50flag = false;
    if (DOB50CPM > DOB50peak) {
      DOB50peak = DOB50CPM;
    }
  }
  if (DOB80flag) {
    DOB80CPM = (60000 / (millis() - lastDOB80)) ;
    tick();
    lastDOB80 = millis();
    DOB80flag = false;
    if (DOB80CPM > DOB80peak) {
      DOB80peak = DOB80CPM;
    }
  }
  if (CTC5flag) {
    CTC5CPM = (60000 / (millis() - lastCTC5))  ;
    tick();
    lastCTC5 = millis();
    CTC5flag = false;
    if (CTC5CPM > CTC5peak) {
      CTC5peak = CTC5CPM;
    }
  }
}

void updateLCD () { // Check the battery, update the LCD display with all current results
  float  batVoltage = BatteryCheck();
  batVoltage = 100 * (batVoltage - 3.2 );
  if (batVoltage <= 10) { // if the battery is less than 10% remaining, switch the LED on permanently
    digitalWrite (led, HIGH);
  }
  lcd.setCursor(0, 0); //Display resuluts to LCD
  lcd.print("Bat:");
  lcd.print(batVoltage, 0);
  lcd.print("% ");
  lcd.print("Hi:");
  lcd.print(DOB50peak);
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print("Mid:");
  lcd.print(CTC5peak);
  lcd.print(" Lo:");
  lcd.print(DOB80peak);
  lcd.print(" ");
  int meterVal = (DOB80CPM + DOB50CPM + CTC5CPM) / 3;
  if (meterVal > peakMeterVal) { // update peak value on analogue meter
    peakMeterVal = meterVal;
  }
  int writeMeterVal = map(peakMeterVal, 0, 100, 0, 255);// write the value to the analogue meter.
  analogWrite (meter, writeMeterVal);
  if (peakMeterVal) {
    peakMeterVal --;
  }
  if (DOB50peak) {
    DOB50peak--;
  }
  if (DOB80peak) {
    DOB80peak--;
  }
  if (CTC5peak) {
    CTC5peak--;
  }

  writeMeterVal = map(peakMeterVal, 0, 100, 0, 255);// write the value to the analogue meter.
  analogWrite (meter, writeMeterVal);
}
