/*  Geiger Counter project
    (c) A.G.Doswell May 2020

    Hardware details can be found at andydoz.blogspot.com

    Compile with -02 switch for maximum performance.
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
unsigned int spk = 4; // Speaker drive transistor connected to pin 4
unsigned int led = 5; // Battery Low/Counter LED connected to pin 5
unsigned int meter = 9;// Analogue meter drive transistor connected to pin 9
unsigned int battery = A0; // Pin used to monitor LiPo voltage
unsigned int tube = 3; // Pin where CTC5 output connects from Q5
unsigned int peakMeterVal; // Peak read value for analogue meter
unsigned int peakCPM; // Peak value
unsigned int loopcounter;
int writeMeterVal;
unsigned long lastPulse; //Timing of last pulse arrived
long countsPerMin; // counts per minute
volatile boolean pulse; // Flag for when a pulse has occured
unsigned long LTAmins; //total number of minutes
unsigned long totalCPM; // total counts per minute
unsigned int LTA; //Long term average
unsigned long actualCPMtime;// timer updated once every second
unsigned int actualCPM = 0;// Actual counts per minute for display
volatile unsigned int actualCPMCount;// actual counts per minute
unsigned long decayMillis;// used to decay the meter slowly
unsigned long LTAcumulative;// cumulative number of counts since system start
unsigned long CPHcumulative;// cumulative number of counts per hour
unsigned int CPH;// counts per hour for display perposes.
unsigned long LTAMins;// number of minutes since system start
int bat;// battery level
int CPHmins; //time since last CPH update in mins
LiquidCrystal_I2C  lcd(0x3F, 16, 2);

byte Increasing[] = { // graphics for trendws
  B01111,
  B00011,
  B00101,
  B00101,
  B01000,
  B01000,
  B10000,
  B10000
};
byte Decreasing[] = {
  B10000,
  B10000,
  B01000,
  B01000,
  B00101,
  B00101,
  B00011,
  B01111
};

byte batt0[] = { //graphics for battery level
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111
};

byte batt1[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111
};

byte batt2[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111
};

byte batt3[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};
byte batt4[] = {
  B01110,
  B11011,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte batt5[] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};


void setup() {
  pinMode (spk, OUTPUT); // Set up pins
  pinMode (led, OUTPUT);
  pinMode (meter, OUTPUT);
  pinMode (battery, INPUT);
  pinMode (tube, INPUT);
  pinMode (7, INPUT);// set this to high impedance to be compatible with existing PCB.
  lcd.init();// start I2C LCD
  lcd.begin(16, 2); // set up the LCD as 16 x 2 display
  lcd.setBacklight(HIGH);
  lcd.clear();
  analogWrite (meter, 255);
  digitalWrite (spk, HIGH);
  digitalWrite (led, HIGH);
  lcd.println(" Geiger Counter "); //splash screen
  lcd.setCursor(0, 1);
  lcd.println("  (C) Doz 2020  ");
  delay (1000);
  digitalWrite (spk, LOW);
  digitalWrite (led, LOW);
  lcd.clear();
  lcd.print("andydoz.blogspot");
  lcd.setCursor(0, 1);
  lcd.print("     .com");
  delay (1000);
  lcd.clear();
  analogWrite (meter, 0);
  lcd.print(" Battery level");
  bat = BatteryCheck ();
  lcd.setCursor(0, 1);
  lcd.print("     ");
  lcd.print(bat);
  lcd.print("%");
  int level = map(bat, 0, 100, 0, 255);
  analogWrite(meter, level);
  delay (2000);
  lcd.clear ();
  lastPulse = millis();
  actualCPMtime = millis();
  decayMillis = millis();
  attachInterrupt(digitalPinToInterrupt(tube), pulseDetect, FALLING);// Interrupt to detect a pulse
}

void loop() {
  if (pulse) { //if the tube gets a pulse, update the results
    processResults();
    pulse = false;
    tick();
  }
  else {
    updateLCD();
  }
  analogWrite (meter, (peakCPM / 5));
  if (((millis() - decayMillis) >= 5) && (peakCPM >= 10)) {
    peakCPM -= 10;
    decayMillis = millis();
    BatteryCheck();
  }
}

void tick() {
  digitalWrite (spk, HIGH); // make the speaker click, and flash the LED.
  digitalWrite (led, HIGH);
  delay (1);
  digitalWrite (spk, LOW);
  digitalWrite (led, LOW);
}

float BatteryCheck() { //get the battery voltage

  int sensorValue = analogRead(battery);
  float voltage = sensorValue * (5.0 / 1023.0);
  int batLevel = (voltage - 3.2) * 100;
  int batChar = (batLevel / 20) + 1;
  if (batLevel <= 5) {
    lcd.setBacklight(LOW); // switch of backlight ot save power
    digitalWrite (led, HIGH);// and switch on the LED to alert us
  }
  switch (batChar) {
    case 1:
      lcd.createChar(1, batt1);
      break;
    case 2:
      lcd.createChar(1, batt2);
      break;
    case 3:
      lcd.createChar(1, batt3);
      break;
    case 4:
      lcd.createChar(1, batt4);
      break;
    case 5:
      lcd.createChar(1, batt5);
      break;
  }
  return batLevel;
}

void pulseDetect() { //ISR for geiger tube
  pulse = true;
  actualCPMCount ++;
}

void processResults () {

  countsPerMin = 60000 / (millis() - lastPulse) ;// calculate instantanious CPM
  lastPulse = millis();
  if (countsPerMin > peakCPM) {
    peakCPM = countsPerMin;
  }
  if (peakCPM > 1275) { // limit peak value so as not to over-range meter
    peakCPM = 1275;
  }
  analogWrite (meter, (peakCPM / 5)); // write peak value to meter
  totalCPM += countsPerMin; // update totalCPM

  if (millis() - actualCPMtime >= 60000) { // After one minute, calculate ACPM, for display, and update CPH
    actualCPMtime = millis();
    actualCPM = actualCPMCount;
    actualCPMCount = 0;
    LTAmins ++;
    CPHmins++;
    LTAcumulative += actualCPM;
    LTA = (LTAcumulative / LTAmins);
    CPHcumulative += actualCPM;
    if (CPHmins == 60) { // after 1 hour, update CPH for display
      CPH = CPHcumulative;
      CPHmins = 0;
      CPHcumulative = 0;
    }
  }

  if (!LTAmins) { // Prevents spurious data until the first minute has elapsed.
    actualCPM = 0;
    LTA = 0;
  }
}

void updateLCD () { // Check the battery, update the LCD display with all current results=
  lcd.setCursor(1, 0); //Display resuluts to LCD
  lcd.print("CPM:");
  lcd.print(countsPerMin);
  lcd.print(" ACPM:");
  lcd.print(actualCPM);
  lcd.print("  ");
  lcd.setCursor(1, 1);
  lcd.print("LTA:");
  lcd.print(LTA);
  lcd.print(" CPH:");
  lcd.print(CPH);
  if (actualCPM > LTA) {
    lcd.createChar(0, Increasing);
    lcd.setCursor (0, 1);
    lcd.write(0);
  }
  if (actualCPM < LTA) {
    lcd.createChar(0, Decreasing);
    lcd.setCursor (0, 1);
    lcd.write(0);
  }
  if (actualCPM == LTA) {
    lcd.setCursor (0, 1);
    lcd.print("-");
  }
  lcd.setCursor(0, 0);
  lcd.write(1);
}
