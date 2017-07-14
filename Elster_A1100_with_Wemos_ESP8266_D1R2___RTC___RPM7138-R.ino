/**
  You can see details of this project at https://wordpress.com/read/feeds/68572272/posts/1524193538
  This project uses a Wemos ESP8266 D1R2 from ebay http://www.ebay.com.au/itm/WeMos-D1-R2-Latest-ESP-12E-WiFi-ESP8266-Board-Arduino-IDE-Uno-SYDNEY/272385909659?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2060353.m2749.l2649
  This project is based on https://pedrosnotes.wordpress.com/ which uses an Arduino UNO.
  A photo of my setup is here: https://drive.google.com/open?id=0B73QqrlATOVmOV9iQktmT0NuSVU
  This project uses a Wemos ESP8266 D1R2 which is Arduino compatible and has WIFI which is used to
  upload the Elster A1100 data to www.pvoutput.org. You can see mine here https://pvoutput.org/intraday.jsp?id=50165&sid=51923
  Also uses an RTC and a light sensitive switch device.
  http://www.ebay.com.au/itm/Photosensitive-Detector-Light-Photo-Sensitive-Switch-Sensor-Module-Arduino-Part/161869818445?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2060353.m1438.l2649
  http://www.ebay.com.au/itm/ZS042-DS3231-AT24C32-IIC-module-precision-Real-time-clock-quare-memory-Arduino/221549752451?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2060353.m2749.l2649
  This code has been tested with a Wemos ESP8266 D1R2 wiring a comercial infrared sesor RPM7138-R from Jaycar Cat.No. ZD1952.
  It will print to to the serial port just when it detects a change
  in Total Imports, Total exports or a change in direction (0=Importing , 1=Exporting)
  The code will not work unless 12v is connected to the power jack (9v - 24V) as the serial microUSB cable can
  only supply 3.3v to the Wemos ESP8266 D1R2 and the RPM7138-R is a 5v device.
  I have tried some IR sensors so far the only one working at the moment is the RPM7138-R which is a 5v device.

  Based on Dave's code to read an elter a100c for more info on that vist:
  http://www.rotwang.co.uk/projects/meter.html
  Thanks Dave and Pedro.
*/
// IMPOTANT: The RPM7138-R is a sensitive device so be carefull of static, heat and scratching the lense.

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

const char ssid[] = "DODOD7D8";
const char pass[] = "VT6543320";

//Meter Pulse
long pulseCountimp = 0, pulseCountexp = 0;
long lastPulseCountimp = 0, lastPulseCountexp = 0;
unsigned long pulseTime, lastTime, savedTime;
double power, TotalkWhexp, TotalkWhimp, lastpower;
int ppwh = 1; //1000 pulses/kwh = 1 pulse per wh
int pulse = 0;
int done = 0;
int firstread = 0;

//RTC integers
int currenthour = 99; // create variable for hours. Dummy value assigned by default.
int currentminute = 99; // create variable for minutes. Dummy value assigned by default.
int currentsecond = 99;  // create variable for seconds. Dummy value assigned by default.
int currenttime = 9999; // create variable for time. Dummy value assigned by default.
int currentday = 99; // create variable for month. Dummy value assigned by default.
int currentmonth = 99; // create variable for month. Dummy value assigned by default.
int currentyear = 9999; // create variable for year. Dummy value assigned by default.
const int errorled = 13; // LED used for status information. I will illuminate this if an error occurs, suggesting to the operator to check the hardware/software.


String result;
String Directory = "GET /service/r2/addstatus.jsp";
String MySID = "51923";// your system ID for www.pvoutput.org must be placed here.
String MyKey = "1ca25ec13241e202c4bc939fc4a1323b050ba8bd"; // your SSID key for pvoutput must be placed here.
String months, days, hours, mins, EG, PG, EU, PU, Temp, volts, IMP;

const unsigned int meter_pulses = 1000; // set for your meter eg 1000/kWh, 800/kWh for Sprint meters

// Infrared Sensor RPM7138-R integers

#define BIT_PERIOD 860      //860 us
#define BUFF_SIZE 64
volatile long data[BUFF_SIZE];
volatile uint8_t in;
volatile uint8_t out;
volatile unsigned long last_us;
unsigned int statusFlag = 0;
float imports;
float exports;
float lastimports;
float lastexports;
float incexports;
float incimports;
float eodimports;
float sodimports;
float todayimports;
float eodexports;
float sodexports;
float todayexports;
float lastsFlag;
float last_data;
uint8_t sFlag;
float imps;
float exps;
uint16_t idx = 0;
uint8_t byt_msg = 0;
uint8_t bit_left = 0;
uint8_t bit_shft = 0;
uint8_t pSum = 0;
uint16_t BCC = 0;
uint8_t eom = 1;
int over = 0;

int past1 = 0;
int past2 = 0;
int counter = 0;
int counterprt = 0;
int countermem = 0;
int lastcountermem = 0;
int countervalue = 10000000;  // Time to get out of decodebuff subroutine
uint8_t dw = 0;   //diagnostics
uint8_t dbug = 1;  //debugging


// Create an instance of the server and specify the port to listen on

WiFiServer server(80);

char servername[] = "pvoutput.org"; // remote server we will connect to

WiFiClient client;

//--------------------------------------------------------------------------------------------------------------------
void deleteHttpHeader()//pvoutput
{
  if (result.endsWith("Content-Type: text/plain")) result = "";
}
//--------------------------------------------------------------------------------------------------------------------
void sendGET() // send data to www.pvoutput.org
{
  time_t prevDisplay = 0; // when the digital clock was displayed
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      if (dbug) digitalClockDisplay();
    }
  }
  if (currentmonth < 10) months = "0" + String(currentmonth);
  else months = String(currentmonth);
  if (currentday < 10) days = "0" + String(currentday);
  else days = String(currentday);
  if (currenthour < 10) hours = "0" + String(currenthour);
  else hours = String(currenthour);
  if (currentminute < 10) mins = "0" + String(currentminute);
  else mins = String(currentminute);
  if (dbug) {
    Serial.println("DATE: " + String(currentyear) + "/" + months + "/" + days);
    Serial.println("TIME: " + hours + ":" + mins);
  }
  if (client.connect(servername, 80)) {  //Send Elster data to www.pvoutput.org
    if (dbug) {
      Serial.println("connected");
      //      Serial.println(Directory + "?key=" + MyKey + "&sid=" + MySID + "&d=" + currentyear + months + days + "&t=" + hours + ":" + mins + "&v1=" + EG + "&v2=" + PG + "&v3=" + EU + "&v4=" + PU + "&v5=" + Temp + "&v6=" + volts + "&v7=" + IMP);
      Serial.println(Directory + "?key=" + MyKey + "&sid=" + MySID + "&d=" + currentyear + months + days + "&t=" + hours + ":" + mins + "&v1=" + EG + "&v2=" + PG + "&v3=" + EU + "&v4=" + PU + "&v6=" + statusFlag + "&v7=" + exports + "&v8=" + imports + "&v9=" + statusFlag + "&v10=" + todayexports + "&v11=" + todayimports + "&v12=" + countermem);
    }
    //      client.println(Directory + "?key=" + MyKey + "&sid=" + MySID + "&d=" + currentyear + months + days + "&t=" + hours + ":" + mins + "&v1=" + EG + "&v2=" + PG + "&v3=" + EU + "&v4=" + PU + "&v5=" + Temp + "&v6=" + volts + "&v7=" + IMP);
    client.println(Directory + "?key=" + MyKey + "&sid=" + MySID + "&d=" + currentyear + months + days + "&t=" + hours + ":" + mins + "&v1=" + EG + "&v2=" + PG + "&v3=" + EU + "&v4=" + PU + "&v6=" + statusFlag + "&v7=" + exports + "&v8=" + imports + "&v9=" + statusFlag + "&v10=" + todayexports + "&v11=" + todayimports + "&v12=" + countermem);
    client.println(); //end of get request
    countermem = 0;
  }
  else {
    if (dbug) {
      Serial.println("connection failed"); //error message if no client connect
      Serial.println();
    }
  }
  delay(1000);
  while (client.connected() && !client.available()) delay(1); //waits for data
  while (client.connected() || client.available()) { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer from pvoutput
    result = result + c;
    deleteHttpHeader();
  }
  if (dbug) {
    Serial.print("Received from server: ");
    Serial.println(result);
  }
  client.stop(); //stop client
  over = 1;
}
//--------------------------------------------------------------------------------------------------------------------
void meterupdate() {
  cli();   //disable interrupts  // Reset KWH at end of day.
  readRTC();
  if ((done == 0) && (currenthour == 23) && (currentminute > 50)) {
    pulseCountexp = 1;
    pulseCountimp = 1;
    done = 1;
    firstread = 0;
  }
  if ((done == 1) && (currenthour > 0)) done = 0;
  if (pulse == 1)   {
    if (statusFlag == 0) TotalkWhimp = (1.0 * pulseCountimp / (ppwh * 1000)); //multiply by 1000 to convert pulses per wh to kwh
    if (statusFlag == 1) TotalkWhexp = (1.0 * pulseCountexp / (ppwh * 1000));
    power = (3600000000.0 / (pulseTime - lastTime)) / ppwh;
    if (firstread == 1)  {    //Do not average first power reading as lastpower = 0.
      power = ((power + lastpower) / 2);     //Average the power
    }
    lastpower = power;
    pulse = 0;
    firstread = 1;
  }

  if (statusFlag == 1) PG = power;  //Power Generated
  if (statusFlag == 1) PU = "0";

  if (statusFlag == 0) {
    PU = power; //Power Used
    PG = "0";
  }

  EG = (TotalkWhexp * 1000); //Energy Generated

  EU = (TotalkWhimp * 1000);  //Energy Used

  sei();    //enable interrupts

  if (dbug) {
    if ((pulseCountexp > lastPulseCountexp) || (pulseCountimp > lastPulseCountimp)) {
      Serial.print("Import Pulses: ");
      Serial.println(pulseCountimp);
      Serial.print("Export Pulses: ");
      Serial.println(pulseCountexp);
      Serial.print("Time between IRS: ");
      Serial.println(pulseTime - lastTime);
      if (statusFlag == 0) Serial.print("Power Imported: ");
      if (statusFlag == 1) Serial.print("Power Exported: ");
      Serial.print(power, 0);
      Serial.print(" watts");
      Serial.println(" ");
      Serial.print("Energy Imported Today (pulses): ");
      Serial.print(TotalkWhimp, 3);
      Serial.println(" kwh ");
      Serial.print("Energy Exported Today (pulses): ");
      Serial.print(TotalkWhexp, 3);
      Serial.println(" kwh ");
      Serial.print("Flag: ");
      if (statusFlag == 1) Serial.println("Exporting");
      if (statusFlag == 0) Serial.println("Importing");
      Serial.print("Time: ");
      Serial.print(currenthour);
      Serial.print(":");
      Serial.print(currentminute);
      Serial.print(":");
      Serial.println(currentsecond);
      lastPulseCountimp = pulseCountimp;
      lastPulseCountexp = pulseCountexp;
    }
  }
}
//--------------------------------------------------------------------------------------------------------------------
void heartbeat() {
  int over = 0;
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage LOW
}
//--------------------------------------------------------------------------------------------------------------------
void setup() {
  attachInterrupt(digitalPinToInterrupt(12), onPulse, FALLING);  // Pulsed output KWH interrupt attached to D6 = GPIO 12
  attachInterrupt(digitalPinToInterrupt(13), onPulseRPM7138, RISING); // D7 = GPIO 13  RPM7138-R
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize HEARTBEAT
  if (dbug) Serial.begin(115200);
  delay(250);
  if (dbug) {
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (dbug) Serial.print(".");
  }
  if (dbug) {
    Serial.println("END SETUP");
    Serial.println(" ");
  }
  result = "";
  last_us = micros();
  delay(1000);
}//end of setup
//--------------------------------------------------------------------------------------------------------------------
void loop() {
  int rd = decode_buff();     // Infrared sensor RPM7138-R loop
  counter = counter + 1;
  if ((!rd) && (counter < countervalue)) return; //IFrD counter
  countermem = counter;
  countermem = ((countermem + lastcountermem) / 2); // average the counter
  lastcountermem = countermem;
  counterprt = counter;
  counter = 0;
  if (rd == 3) {
    rd = 4;
    if ((statusFlag > 1) || (statusFlag < 0)) { //filter
      statusFlag = lastsFlag;
      imports = lastimports;
      exports = lastexports;
    }

    if (lastimports == 0) lastimports = imports;

    if (imports > (lastimports + 40)) {  //filter
      imports = lastimports;  
      exports = lastexports;
      statusFlag = lastsFlag;
    }

    if (imports < 0)  {         //filter
      imports = lastimports;
      exports = lastexports;
      statusFlag = lastsFlag;
    }
    if (lastexports == 0) lastexports = exports;

    if (exports > (lastexports + 40)) {  //filter
      exports = lastexports;
      imports = lastimports;
      statusFlag = lastsFlag;
    }
    if (exports < 0) {              //filter
      exports = lastexports;
      imports = lastimports;
      statusFlag = lastsFlag;
    }
    if ((currenthour == 0) && (currentminute >= 0) && (currentminute <= 3 ) && (past1 == 0)) {
      if (sodexports != exports) sodexports = exports;
      past1 = 1;     
    }
    if ((currenthour == 0) && (currentminute > 3) && (past1 == 1)) past1 = 0;

    if ((currenthour == 0) && (currentminute >= 0) && (currentminute <=3 ) && (past2 == 0)) {
      if (sodimports != imports) sodimports = imports;
      past2 = 1;
    }
    if ((currenthour == 0) && (currentminute > 3) && (past2 == 1)) past2 = 0;


    if ((sodexports == 0) && (exports != lastexports)) {
      incexports = exports - lastexports;
      todayexports = todayexports + incexports;
    }
    if ((sodimports == 0) && (imports != lastimports)) {
      incimports = imports - lastimports;
      todayimports = todayimports + incimports;
    }
    
    if ((sodexports != 0) && (exports != sodexports)) todayexports = exports - sodexports;
    if ((sodimports != 0) && (imports != sodimports)) todayimports = imports - sodimports;

    lastimports = imports;
    lastexports = exports;
    lastsFlag = statusFlag;

    if (dbug) {
      Serial.println("");
      Serial.print("Total Import IFrD: ");
      Serial.print(imports);
      Serial.println(" kWh");
      Serial.print("Total Export IFrD: ");
      Serial.print(exports);
      Serial.println(" kWh");
      Serial.print("Flag: ");
      if (statusFlag == 1) Serial.println("Exporting");
      if (statusFlag == 0) Serial.println("Importing");
      Serial.print("Imports Today IFrD: ");
      Serial.print(todayimports, 1);
      Serial.println(" kWh");
      Serial.print("Exports Today IFrD: ");
      Serial.print(todayexports, 1);
      Serial.println(" kWh");
      Serial.println(counterprt);
      Serial.println(" ");
    }
  }
  heartbeat();
  meterupdate();
  readRTC();
  if (currentminute == 0 && over == 0) sendGET();
  if (currentminute > 0 && currentminute < 5 && over == 1) over = 0;
  if (currentminute == 5 && over == 0) sendGET();
  if (currentminute > 5 && currentminute < 10 && over == 1) over = 0;
  if (currentminute == 10 && over == 0) sendGET();
  if (currentminute > 10 && currentminute < 15 && over == 1) over = 0;
  if (currentminute == 15 && over == 0) sendGET();
  if (currentminute > 15 && currentminute < 20 && over == 1) over = 0;
  if (currentminute == 20 && over == 0) sendGET();
  if (currentminute > 20 && currentminute < 25 && over == 1) over = 0;
  if (currentminute == 25 && over == 0) sendGET();
  if (currentminute > 25 && currentminute < 30 && over == 1) over = 0;
  if (currentminute == 30 && over == 0) sendGET();
  if (currentminute > 30 && currentminute < 35 && over == 1) over = 0;
  if (currentminute == 35 && over == 0) sendGET();
  if (currentminute > 35 && currentminute < 40 && over == 1) over = 0;
  if (currentminute == 40 && over == 0) sendGET();
  if (currentminute > 40 && currentminute < 45 && over == 1) over = 0;
  if (currentminute == 45 && over == 0) sendGET();
  if (currentminute > 45 && currentminute < 50 && over == 1) over = 0;
  if (currentminute == 50 && over == 0) sendGET();
  if (currentminute > 50 && currentminute < 55 && over == 1) over = 0;
  if (currentminute == 55 && over == 0) sendGET();
  if (currentminute > 55 && currentminute <= 59 && over == 1) over = 0;

  if (dbug) Serial.println(" "), digitalClockDisplay();
}
//--------------------------------------------------------------------------------------------------------------------
static int decode_buff(void) {    // Infrared sensor RPM7138-R
  if (dbug) {
    if (counter == (countervalue - 1)) Serial.println("Timed out");  //IFrD counter
  }
  if (in == out) {
    if (dw) Serial.print(".");
    //   if (dw) Serial.print(counter);
    return 0;
  }
  int next = out + 1;
  if (next >= BUFF_SIZE) next = 0;
  int p = (((data[out]) + (BIT_PERIOD / 2)) / BIT_PERIOD);
  //  if (dbug) { Serial.print(data[out]); Serial.print(" "); if (p>500) Serial.println("<-"); }
  if (p > 500) {
    idx = BCC = eom = imps = exps = sFlag = 0;
    out = next;
    if (dw) Serial.print("o");
    //    if (dw) Serial.print(counter);
    return 0;
  }
  bit_left = (4 - (pSum % 5));
  bit_shft = (bit_left < p) ? bit_left : p;
  pSum = (pSum == 10) ? p : ((pSum + p > 10) ? 10 : pSum + p);
  if (eom == 2 && pSum >= 7) {
    pSum = pSum == 7 ? 11 : 10;
    eom = 0;
  }
  if (bit_shft > 0) {
    byt_msg >>= bit_shft;
    if (p == 2) byt_msg += 0x40 << (p - bit_shft);
    if (p == 3) byt_msg += 0x60 << (p - bit_shft);
    if (p == 4) byt_msg += 0x70 << (p - bit_shft);
    if (p >= 5) byt_msg += 0xF0;
  }
  if (pSum >= 10) {
    idx++;
    if (idx != 328) BCC = (BCC + byt_msg) & 255;
    //       if (dbug) Serial.print("["); Serial.print(idx); Serial.print(":"); Serial.print(byt_msg, HEX); Serial.println("]");
    if (idx >= 95 && idx <= 101)
      imps += ((float)byt_msg - 48) * pow(10 , (101 - idx));
    if (idx == 103)
      imps += ((float)byt_msg - 48) / 10;
    if (idx >= 114 && idx <= 120)
      exps += ((float)byt_msg - 48) * pow(10 , (120 - idx));
    if (idx == 122)
      exps += ((float)byt_msg - 48) / 10;
    if (idx == 210)
      sFlag = (byt_msg - 48) >> 3; //1=Exporting ; 0=Importing
    if (byt_msg == 3) eom = 2;
    if (idx == 328) {
      if ((byt_msg >> (pSum == 10 ? (((~BCC) & 0b1000000) ? 0 : 1) : 2)) == ((~BCC) & 0x7F)) {
        //   if (last_data != (imps + exps + sFlag)) {
        imports = imps;
        exports = exps;
        statusFlag = sFlag;
        last_data = imps + exps + sFlag;
        out = next;
        return 3;
        //   }
      }
    }
  }
  out = next;
  if (dw) Serial.print("-");
  //  if (dw) Serial.print(counter);
  return 0;
}
//--------------------------------------------------------------------------------------------------------------------
void onPulse()  // Meter - The interrupt routine
{
  savedTime = lastTime;
  lastTime = pulseTime;  //used to measure time between pulses.
  pulseTime = micros();
  if ((pulseTime - lastTime) > 180000) {  //180mS filter for max power of 20kw reading = 3600000000/20000 = 180000uS = 180mS
    pulse = 1;
    if (statusFlag == 1) pulseCountexp++;  // Count of the number of pulses since start of day
    if (statusFlag == 0) pulseCountimp++;  // Count of the number of pulses since start of day
  }
  else {
    pulseTime = lastTime;
    lastTime = savedTime;
  }
}
//--------------------------------------------------------------------------------------------------------------------
void onPulseRPM7138() {     // Infrared sensor RPM7138-R
  unsigned long us = micros();
  unsigned long diff = us - last_us;
  if (diff > 20 ) {
    last_us = us;
    int next = in + 1;
    if (next >= BUFF_SIZE) next = 0;
    data[in] = diff;
    in = next;
  }
}
//--------------------------------------------------------------------------------------------------------------------
void digitalClockDisplay()//timeclock
{
  // digital clock display of the time
  if (dbug) {
    // date and time
    Serial.print("Year: ");
    Serial.println(currentyear);
    Serial.print("Month: ");
    Serial.println(currentmonth);
    Serial.print("Day: ");
    Serial.println(currentday);
    Serial.print("Time: ");
    print2digits(currenthour);
    Serial.print(":");
    print2digits(currentminute);
    Serial.print(":");
    print2digits(currentsecond);
    Serial.println(" ");
  }
}
//--------------------------------------------------------------------------------------------------------------------
void printDigits(int digits)//timeclock
{
  // utility for digital clock display: prints preceding colon and leading 0
  if (dbug) Serial.print(":");
  if (digits < 10) {
    if (dbug) Serial.print('0');
  }
  if (dbug) Serial.print(digits);
}
//--------------------------------------------------------------------------------------------------------------------
void readRTC()
{

  tmElements_t tm;


  if (RTC.read(tm))
  {

    currenthour = (tm.Hour); // set global variable to current time (hours)
    currentminute = (tm.Minute); // set global variable to current time (minutes)
    currentsecond = (tm.Second); // set global variable to current time (minutes)
    currenttime = ((currenthour * 100) + currentminute);
    currentday = (tm.Day); // set global variable to current (date as number)
    currentmonth = (tm.Month); // set global variable to current  (month as a number)
    currentyear =  (tmYearToCalendar(tm.Year));

    //    digitalWrite(errorled, LOW);

  }
  else
  {
    if (RTC.chipPresent())
    {
      if (dbug) {
        Serial.println("The DS1307 is stopped.  Please run the SetTime");
        Serial.println("example to initialize the time and begin running.");
        Serial.println();
      }
    }
    else
    {
      if (dbug) {
        Serial.println("DS1307 read error!  Please check the circuitry.");
        Serial.println();
      }
    }
    digitalWrite(errorled, HIGH); // light up the error LED to attract attention.
    delay(9000);
  }
}
//--------------------------------------------------------------------------------------------------------------------
void print2digits(int number) {  // used for RTC module to append '0' to single digit data, eg hours, days, months. Remove if not using RTC module.
  if (number >= 0 && number < 10) {
  }
  if (dbug) Serial.print(number);
}
