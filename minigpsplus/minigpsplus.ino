/******************************************************************************
 *
 * Filename       : minigpsplus.ino
 * Copyright      : (CC-BY-NC) 2020
 * Project        : MiniGPS+
 *
 * Revision History:
 * Developer            Date       Change Description
 * Hafiz C.             06/01/2021 Cleaning more lines
 *****************************************************************************/

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <NMEAGPS.h>
#include <U8x8lib.h>
#include <GPSport.h>
#include <SPI.h>
#include <SD.h>

File myFile;

Adafruit_BMP280 bmp; // I2C

static NMEAGPS  gps;
static gps_fix  fix;

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

String fileNames;
int x;
unsigned long m;
int long_log = 1000;
int btState = 0;
int btTemp[3] = {0, 0, 0};
int xxx;
uint32_t timer;
bool screencleared = false;
String txt = "";

float lat_dd;
String lat_SouthN;
float lat_d1;
int lat_d;
float lat_m1;
int lat_m;
float lat_s;

String LOG[6];

void setup() {
  Serial.begin(9600);
  gpsPort.begin(9600);

  pinMode(3, INPUT);

  Serial.println("checking SD card...");

  if (!SD.begin(48)) {
    Serial.println("init FAILED! :(");
  } else {
    Serial.println("init DONE. :)");
  }

  Serial.println(F("checking BMP280.."));

  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring! :("));
    while (1);
  } else {
	Serial.println(F("OK! :)"));
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Serial.println(F("checking OLED.."));
    if (!u8x8.begin()) {
	Serial.println(F("check wiring!"));
  } else {
	Serial.println(F("OLED found :)"));
	u8x8.setFont(u8x8_font_chroma48medium8_r);
  }

  // Start up screen on OLED
  u8x8.fillDisplay();
  delay(1000);

  u8x8.inverse();
  u8x8.draw1x2String(4, 3, "MiniGPS+");
  delay(1300);
  u8x8.noInverse();

  for (uint8_t r = 0; r < u8x8.getRows(); r++ )
  {
    u8x8.clearLine(r);
    delay(100);
    u8x8.setInverseFont(1);
  }

  u8x8.inverse();

  u8x8.drawString(0, 0, "LAT");
  u8x8.drawString(0, 1, "LNG");
  u8x8.drawString(0, 2, "ALT");
//  u8x8.drawString(0, 3, "ELT");
//  u8x8.drawString(8, 3, "ELN");

  u8x8.drawString(0, 4, "FIX");
  u8x8.drawString(5, 4, "LOG");

  u8x8.drawString(0, 6, "AVL");
  u8x8.drawString(5, 6, "TMP");
  u8x8.drawString(10, 6, "TME");

  u8x8.noInverse();

  timer = millis();
}

//----------------------------------------------------------------
//  This function gets called about once per second, during the GPS
//  quiet time.

static void doSomeWork()
{
  // timer = millis(); // reset the timer

  //----------------------------------------------------------------
  //  This section is run before a fix is made to show sat info (Available, Tracked, Time)

  // Count how satellites are being received for each GNSS
  int totalSatellites, trackedSatellites;
  totalSatellites = gps.sat_count;
  for (uint8_t i = 0; i < totalSatellites; i++) {
    if (gps.satellites[i].tracked) {
      trackedSatellites++;
    }
  }

  String temp;
  u8x8.setCursor(5, 7);
  u8x8.print("   ");
  u8x8.setCursor(5, 7);
  temp = String(bmp.readTemperature(), 0);
  u8x8.print(temp);
  u8x8.print("C");

  // LOG temperature interval 30 menit
  uint8_t minute = fix.dateTime.minutes;

  if(minute == 30) {
      LOG[5] = temp;
  }
  else if(minute == 60) {
      LOG[5] = temp;
  }
  else {
      LOG[5] = "";
  }

//  Altitude from BM280
//  u8x8.setCursor(4, 2);
//  u8x8.print("     ");
//  u8x8.setCursor(4, 2);
//  LOG[3] = String(bmp.readAltitude(1013.25), 1);
//  u8x8.print(LOG[3]);
//  u8x8.print(" m");

  // Altitude from NEOGPS
  u8x8.setCursor(4, 2);
  u8x8.print("      ");
  u8x8.setCursor(4, 2);
  LOG[4] = String(fix.altitude(), 1);
  u8x8.print(LOG[4]+" m");

  if (btState == 1) {
    u8x8.drawString(5, 5, "ON ");
    u8x8.setCursor(8, 5);
    u8x8.print(fileNames);
  } else {
    u8x8.drawString(5, 5, "OFF");
    u8x8.setCursor(8, 5);
    u8x8.print("      ");
  }


  enum {BufSizeTracked = 3}; //Space for 2 characters + NULL
  char trackedchar[BufSizeTracked];
  snprintf (trackedchar, BufSizeTracked, "%d", trackedSatellites);
  //  u8x8.drawString(0, 7, "  ");
  //  u8x8.drawString(0, 7, trackedchar);

  enum {BufSizeTotal = 3};
  char availchar[BufSizeTotal];
  snprintf (availchar, BufSizeTotal, "%d", totalSatellites);
  u8x8.drawString(0, 7, "  ");
  u8x8.drawString(0, 7, availchar);

  //----------------------------------------------------------------

  if (fix.valid.time) {
    enum {BufSizeTime = 3};
    int hour = fix.dateTime.hours + 7;
    if (hour >= 24) {
      hour = hour - 24;
    }
    int minute = fix.dateTime.minutes;
    int second = fix.dateTime.seconds;

    char hourchar[BufSizeTime];
    char minutechar[BufSizeTime];
    char secondchar[BufSizeTime];
    snprintf (hourchar, BufSizeTime, "%d", hour);
    snprintf (minutechar, BufSizeTime, "%d", minute);
    snprintf (secondchar, BufSizeTime, "%d", second);
    if ( hour < 10 )
    {
      snprintf (hourchar, BufSizeTime, "%02d", hour);
    }
    if ( minute < 10 )
    {
      snprintf (minutechar, BufSizeTime, "%02d", minute);
    }    if ( second < 10 )
    {
      snprintf (secondchar, BufSizeTime, "%02d", second);
    }

    LOG[1] = "T";
    LOG[1] += hourchar;
    LOG[1] += ":";
    LOG[1] += minutechar;
    LOG[1] += ":";
    LOG[1] += secondchar;

    u8x8.drawString(10, 7, hourchar);
    u8x8.drawString(12, 7, ":");
    u8x8.drawString(13, 7, minutechar);
  }

  //----------------------------------------------------------------

  if (fix.valid.date) {
    enum {BufSizeTime = 3};
    enum {BufSizeYear= 5};
    int year = fix.dateTime.full_year();
    int month = fix.dateTime.month;
    int date = fix.dateTime.date;

    char yearchar[4];
    char monthchar[2];
    char datechar[2];
    snprintf (yearchar, BufSizeYear, "%d", year);
    snprintf (monthchar, BufSizeTime, "%d", month);
    snprintf (datechar, BufSizeTime, "%d", date);

    if ( month < 10 )
    {
      snprintf (monthchar, BufSizeTime, "%02d", month);
    }
    if ( date < 10 )
    {
      snprintf (datechar, BufSizeTime, "%02d", date);
    }

    LOG[0] = "";
    LOG[0] += yearchar;
    LOG[0] += "-";
    LOG[0] += monthchar;
    LOG[0] += "-";
    LOG[0] += datechar;
  }

  //----------------------------------------------------------------

  // Once the location is found the top part of the screen is cleared and the fix data is shown
  if (fix.valid.location) {

    if (!screencleared) // do once
    {
      int r;
      for ( int r = 0; r < 5; r++ )
      {
        //        u8x8.clearLine(r);
      }
      screencleared = true;
    }

    enum {BufSize = 3}; // Space for 2 digits
    char satchar[BufSize];
    snprintf (satchar, BufSize, "%d", fix.satellites);
    u8x8.drawString(0, 5, "  ");
    u8x8.drawString(0, 5, satchar);

    char latchar[10]; // Buffer big enough for 9-character float
    dtostrf(fix.latitude(), 3, 7, latchar); // Leave room for large numbers
    //u8x8.drawString(4, 0, latchar);
    LOG[2] = latchar;

    lat_dd = LOG[2].toFloat();
    if (lat_dd < 0)
      lat_SouthN = "S";
    else
      lat_SouthN = "N";

    lat_d1 = abs(lat_dd);
    lat_d = lat_d1;
    lat_m1 = (lat_d1 - lat_d) * 60;
    lat_m = lat_m1;
    lat_s = (lat_m1 - lat_m) * 60;

    u8x8.setCursor(4, 0);
    u8x8.print(lat_d);         u8x8.print(" ");
    u8x8.print(lat_m);         u8x8.print("'");
    u8x8.print(lat_s, 0);      u8x8.print("\"");
    u8x8.print(lat_SouthN);

  // Print error LAT
  // String error_lat = String(fix.valid.lat_err);
  // 8x8.setCursor(4, 3);        u8x8.print(error_lat);

    char longchar[10];
    dtostrf(fix.longitude(), 3, 7, longchar);
    //    u8x8.drawString(4, 1, longchar);
    LOG[3] = longchar;

    lat_dd = LOG[3].toFloat();
    if (lat_dd < 0)
      lat_SouthN = "W";
    else
      lat_SouthN = "E";

    lat_d1 = abs(lat_dd);
    lat_d = lat_d1;
    lat_m1 = (lat_d1 - lat_d) * 60;
    lat_m = lat_m1;
    lat_s = (lat_m1 - lat_m) * 60;


    u8x8.setCursor(4, 1);
    u8x8.print(lat_d);         u8x8.print(" ");
    u8x8.print(lat_m);         u8x8.print("'");
    u8x8.print(lat_s, 0);      u8x8.print("\"");
    u8x8.print(lat_SouthN);

  // Print error LON
  // String error_lon = String(fix.valid.lon_err);
  // u8x8.setCursor(12, 3);        u8x8.print(error_lon);

  }
}

//  This is the main GPS parsing loop.
static void GPSloop()
{
  while (gps.available( gpsPort )) {
    fix = gps.read();
    doSomeWork();
  }
}

void loop()
{
  GPSloop();

  //  // until we get a fix, print a dot every 5 seconds
  //  if (millis() - timer > 5000 && !screencleared) {
  //    timer = millis(); // reset the timer
  //    //u8x8.print(".");
  //  }


  btTemp[0] = digitalRead(3);

  if (btTemp[0] != btTemp[1]) {
    delay(100);

    if (digitalRead(3) == btTemp[0]) {
      btTemp[1] = btTemp[0];
    }


    if (btTemp[1] == 0 && btTemp[2] == 0) {
      Serial.println("Start LOG");
      btTemp[2] = 1;
      x = 0;
      cek_filename();
      log_header(fileNames);
      btState = 1;
      m = millis();

      u8x8.drawString(5, 5, "ON ");
      u8x8.setCursor(8, 5);
      u8x8.print(fileNames);
    }
    else if (btTemp[1] == 0 && btTemp[2] == 1) {
      Serial.println("Stop LOG");
      btTemp[2] = 0;
      btState = 0;

      u8x8.drawString(5, 5, "OFF");
      u8x8.setCursor(8, 5);
      u8x8.print("      ");
    }
  }

  if (millis() - m >= long_log) {
    if (btState == 1) {
      log_main(fileNames);
    }
    m = millis();
  }

}


void log_header(String fileName) {
  File dataFile = SD.open(fileName, FILE_WRITE);

  if (dataFile) {
    dataFile.println("DateTime, Latitude, Longitude, Altitude, Temperature");
    dataFile.close();
  }
  else {
    Serial.println("error opening datalog.txt");
  }
}

void log_main(String fileName) {
  File dataFile = SD.open(fileName, FILE_WRITE);
  x++;

  if (dataFile) {
    dataFile.print(LOG[0]);
    /* dataFile.print(", "); */
    dataFile.print(LOG[1]);
    dataFile.print(", ");
    dataFile.print(LOG[2]);
    dataFile.print(", ");
    dataFile.print(LOG[3]);
    dataFile.print(", ");
    dataFile.print(LOG[4]);
    dataFile.print(", ");
    dataFile.println(LOG[5]);
    dataFile.close();
  }
  else {
    Serial.println("error opening datalog.txt");
  }
}

void cek_filename() {
  String cek;

  for (int i = 0; i < 1000; i++) {
    cek = "gps";

    if (i < 10) {
      cek += "00";
      cek += String(i);
    }

    if (i >= 10 && i < 100) {
      cek += "0";
      cek += String(i);
    }

    if (i >= 100 && i < 1000) {
      cek += String(i);
    }

    if (SD.exists(cek)) {
      //Serial.println("example.txt exists.");
    } else {
      Serial.print("File Name : ");
      Serial.println(cek);
      fileNames = cek;
      break;
    }
  }

}
