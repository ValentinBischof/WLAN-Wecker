#include <Wire.h>
#include <EEPROM.h>
#include <I2C_eeprom.h>
#include <SPI.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>

#include "DFRobot_ST7687S_Latch.h"
#include "DFRobot_Display_Clock.h"

#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#include <TimeLib.h>


typedef struct 
{
  uint8_t settingsByte1; // 1 Byte >-> 1 Byte
  uint8_t settingsByte2; // 1 Byte >-> 2 Bytes

  char displayName[16]; // 16 Byte >-> 18 Bytes

  uint8_t timeByte1;    // 1 Byte >-> 19 Bytes
  uint8_t timeByte2;    // 1 Byte >-> 20 Bytes
  uint8_t idByte;       // 1 Byte >-> 21 Bytes

  // clockBytes = $"{settingsByte1},{settingsByte2},{displayName},{hourByte},{minuteByte},{idByte}";
  // Zusammensetzung -> 
  
} AlarmEncoded; // 21 Bytes per Alarm;

typedef struct
{
  bool isEnabled;
  bool makeSound;
  bool doVibrate;
  bool repeat;

  bool monday;
  bool tuesday;
  bool wednesday;
  bool thursday;
  bool friday;
  bool saturday;
  bool sunday;

  char displayName[16];

  int hour;
  int minute;

  int idByte;
} AlarmDecoded;

typedef struct
{
  char ssid[32];
  char pw[32];
} WiFiCredentials;


bool connectedToWiFi = false;
char ssid[32];
char pw[32];

AlarmEncoded alarms[255]; // Limit an unterschiedlichen Alarmen (aufgrund uint8_t)
WiFiCredentials wiFiCredentials;

int outOfContext = 255; // Haben bestimmte Bytes diesen Wert, ist dieser Alarm nicht gültig, fehlerhaft oder nicht beschrieben.
bool alarmActive = false;
bool alarmActiveVibration;
char alarmActiveText[16];

const int arduinoSlave = 8;
const char* dns_name = "wifi-clock-";

IPAddress local_IP(192, 168, 1, 184);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

uint8_t pin_cs = 17, pin_rs = 4, pin_wr = 15, pin_lck = 5;
DFRobot_ST7687S_Latch tft(pin_cs, pin_rs, pin_wr, pin_lck);

int segmentSize = 6; // 360 / 60 == 6 px 

int touchToWakeTime = 30;
float touchToWakeForce = 1.3;

int currentDisplayState = true;
int startCountdown = false;
int waitTimeUntilSeconds, waitTimeUntilMinutes, waitTimeUntilHours;


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

int vibrationPin = 16;


WebServer webServer(80);
WiFiClient wiFiClient;

I2C_eeprom dataEEPROM(0x50, I2C_DEVICESIZE_24LC256);
Adafruit_MPU6050 mpu;


HardwareSerial hwSerial(1);
DFRobotDFPlayerMini dfPlayer;
int volume = 30;

void setup() 
{
  Serial.begin(115200);
  hwSerial.begin(9600, SERIAL_8N1, 25, 26);
  

  pinMode(vibrationPin, OUTPUT);

  SetupData();
  ConnectWiFi(ssid, pw);

  StartWebServer();   
  
  if (!connectedToWiFi) { return; }

  SetupTime(); 
  SetupGyro();
  SetupSound();
  SetupDisplay();

  Serial.print("Diese URL zum Verbinden aufrufen: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void LoadData()
{
  int timerStart = micros();
  int byteLength = dataEEPROM.readBlock(0, (uint8_t*) alarms, sizeof(alarms));
  int byteLength6 = dataEEPROM.readBlock(12000, (uint8_t*) &wiFiCredentials, sizeof(wiFiCredentials));
  int timerEnd = micros() - timerStart;

  Serial.println(wiFiCredentials.ssid);
  Serial.println(wiFiCredentials.pw);

  Serial.println("Gelesen: " + String(byteLength) + " Bytes in " + String(timerEnd) + "µs");
}

void SaveAlarm(int id) 
{
  
  int addresse = (id) * sizeof(alarms[id]);
  Serial.println("Daten werden gespeichert");
  Serial.println(addresse);
  
  int timerStart = micros();
  dataEEPROM.writeBlock(addresse, (uint8_t*) &alarms[id], sizeof(alarms[id]));
  int timerEnd = micros() - timerStart;

  Serial.println("Gespeichert: " + String(sizeof(alarms[id])) + " Bytes in " + String(timerEnd) + "µs");
}

void UpdateAlarmEEPROM(int id)
{
  
  int addresse = (id) * sizeof(alarms[id]);
  Serial.println("Daten werden gespeichert");
  Serial.println(addresse);
  
  int timerStart = micros();
  dataEEPROM.updateBlock(addresse, (uint8_t*) &alarms[id], sizeof(alarms[id]));
  int timerEnd = micros() - timerStart;

  Serial.println("Beschrieben: " + String(sizeof(alarms[id])) + " Bytes in " + String(timerEnd) + "µs");
}

void UpdateConnectionEEPROM(String data)
{
  String dataSeperated[2];
  
  int stringCount = 0;
  while (data.length() > 0)
  {
    int index = data.indexOf(',');
    if (index == -1) 
    {
      dataSeperated[stringCount++] = data;
      break;
    }
    else
    {
      dataSeperated[stringCount++] = data.substring(0, index);
      data = data.substring(index+1);
    }
  } 

  Serial.println(dataSeperated[0]);
  Serial.println(dataSeperated[1]);

  WiFiCredentials _wiFiCredentials;
  dataSeperated[0].toCharArray(_wiFiCredentials.ssid, sizeof(_wiFiCredentials.ssid));
  dataSeperated[1].toCharArray(_wiFiCredentials.pw, sizeof(_wiFiCredentials.pw));
  
  int timerStart = micros();
  dataEEPROM.writeBlock(12000, (uint8_t*) &_wiFiCredentials, sizeof(_wiFiCredentials));
  int timerEnd = micros() - timerStart;

  Serial.println("64 Bytes beschrieben in " + String(timerEnd) + "µs");
}


void SaveArrayData()
{
  dataEEPROM.writeBlock(0, (uint8_t*) &alarms, sizeof(alarms));
}

void StartWebServer()
{
  webServer.on("/status", SendStatus);         
  webServer.on("/data/alarms", SendAlarmData); 
  webServer.on("/data/connection", SendConnectionData);
  webServer.on("/send/alarm", GetAlarmData);
  webServer.on("/remove/alarm", RemoveAlarmData);
  

  webServer.onNotFound(handleNotFound);
  webServer.begin();

  Serial.println("WebServer gestartet");
}

void handleNotFound() {
  webServer.send(404, "text/plain", "Gibts net");
}

//Sende Status zu Webseite
void SendStatus()
{
  webServer.send(200, "text/plain", "OK"); 
}

// Sende Alarm Daten zur Webseite
void SendAlarmData()
{
  String message = "";
  
  for (int i = 0; i <= 99; i++) 
  {
    AlarmEncoded alarm = alarms[i];
    if (alarm.settingsByte1 == outOfContext && alarm.settingsByte2 == outOfContext && alarm.timeByte1 == outOfContext & alarm.timeByte2 == outOfContext)
    {
      continue;
    }

    message += String(alarm.settingsByte1) + "," +
               String(alarm.settingsByte2) + "," + 
               String(alarm.displayName) + "," + 
               String(alarm.timeByte1) + "," + 
               String(alarm.timeByte2) + "," + 
               String(alarm.idByte) + ";";  
  }

  Serial.println(message);
  webServer.send(200, "text/plain", message); 
}

void SendConnectionData()
{
  if (webServer.hasArg("data"))
  {
    String data = webServer.arg("data");
    if (data != "")
    {
      webServer.send(200, "text/plain", data); 
      UpdateConnectionEEPROM(data);
      ESP.restart();
    }
  }
}

void GetAlarmData()
{
    if (webServer.hasArg("data"))
    {
      String data = webServer.arg("data");
      if(data != "")
      {
        webServer.send(200, "text/plain", data); 

        AlarmEncoded alarm = ReadEncodedAlarm(data);
        alarms[alarm.idByte] = alarm;
        UpdateAlarmEEPROM(alarm.idByte);
      }
      else
      {      
        webServer.send(200, "text/plain","no_params");       
      }
    }
    else
    {
      webServer.send(200, "text/plain", "no_args"); 
    }
}

void RemoveAlarmData()
{
  if(webServer.hasArg("id"))
    {
      String data = webServer.arg("id");
      if(data != "")
      {
        webServer.send(200, "text/plain", data); 
        
        AlarmEncoded alarm;
        alarm.settingsByte1 = outOfContext;
        alarm.settingsByte2 = outOfContext;
        
        String t = "";
        t.toCharArray(alarm.displayName, 16);
        alarm.timeByte1 = outOfContext;
        alarm.timeByte2 = outOfContext;
        alarm.idByte = outOfContext;

        int id = data.toInt();
        alarms[id] = alarm;
        UpdateAlarmEEPROM(id);
      }
    }
}


void CreateWiFi()
{
  WiFi.disconnect();
  WiFi.softAPConfig(local_IP, gateway, subnet);
    
  WiFi.softAP("WLAN Wecker");
  MDNS.begin("wifi-clock");
  delay(5000);
}
void ConnectWiFi(String wiFiName, String pin)
{
  int attempts = 0;

  // Connect to WiFi
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wiFiCredentials.ssid, wiFiCredentials.pw);
  while (WiFi.status() != WL_CONNECTED)
  {
    attempts++;

    delay(300 * 1);
    Serial.print(".");

    if (attempts == 90)
    {
      Serial.println("");
      Serial.println("Eine Verbindung konnte nicht aufgebaut werden");
      connectedToWiFi = false;
      CreateWiFi();
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi Verbundung aufgebaut");

  connectedToWiFi = true;

  // Finde eine addresse die noch verfügbar ist:
  Serial.println("Suche freie Domain...");

  bool addressFound = false;
  int addressID = 0;
  while (!addressFound)
  {
    addressID++;

    HTTPClient http;
    http.begin(wiFiClient, "http://" + String(dns_name) + addressID + ".local/");
    int httpCode = http.GET();

    Serial.println(httpCode);

    if (httpCode == -1)
    {
      addressFound = true;
      Serial.println("Domain gefunden");
    }

    delay(200);
  }

  //char* dnsName = String(dns_name) + addressID;
  if (MDNS.begin(dns_name))
  {
    Serial.println("DNS gestartet, erreichbar unter: ");
    Serial.println("http://" + String(dns_name) + addressID + ".local/");
  }

  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "prop1", "test");
  MDNS.addServiceTxt("http", "tcp", "prop2", "test2");
}

void CheckDisplayWakeUp(int hour, int minute, int second, float gForce, bool startup = false)
{
  if ((gForce >= touchToWakeForce) || (startup == true)) 
  {
    currentDisplayState = true;
    startCountdown = true;
    
    int addedSeconds = second + touchToWakeTime;
    waitTimeUntilHours = addedSeconds >= 60 && minute == 59 ? hour + 1 : hour;
    waitTimeUntilMinutes = addedSeconds >= 60 ? (waitTimeUntilHours != hour ? minute = 0 : minute + 1) : minute;
    waitTimeUntilSeconds = addedSeconds >= 60 ? addedSeconds - 60 : addedSeconds;


    Serial.println("S1: " + String(waitTimeUntilHours) + ":" + String(waitTimeUntilMinutes) + ":" + String(waitTimeUntilSeconds));
    Serial.println("S2: " + String(hour) + ":" + String(minute) + ":" + String(second));
    tft.displaySleepOUT();
  }
}

void CheckDisplaySleep(int hour, int minute, int second)
{
  if (hour >= waitTimeUntilHours && minute >= waitTimeUntilMinutes && second >= waitTimeUntilSeconds)
  {
    currentDisplayState = false;
    startCountdown = false;
    tft.displaySleepIN();
  }
}

void CheckAlarms(int hour, int minute, int second, int day)
{
  // Gehe durch alle Alarme durch (255 Stück)
  for (int i = 0; i <= 255; i++)
  {
    Serial.println(i);
    
    AlarmEncoded alarm = alarms[i];
    if (alarm.settingsByte1 == outOfContext && alarm.settingsByte2 == outOfContext && alarm.timeByte1 == outOfContext & alarm.timeByte2 == outOfContext)
    {
      continue;
    }

    Serial.println("Found Alarm!");
    
    Serial.println(alarm.timeByte1);
    Serial.println(hour);


    if (alarm.timeByte1 == hour && alarm.timeByte2 == minute)
    {
      Serial.println("Ich dekodiere jetzt!");
      AlarmDecoded decodedAlarm = DecodeAlarm(alarm);
      Serial.println("Ich habe fertig!");
      
      if (!decodedAlarm.isEnabled) { continue; }

      bool weekdayEqualsSettings = decodedAlarm.repeat ? false : true; 
      // Wenn wiederholen nicht ausgewählt ist, sind die Tage egal, daher direkt auf true.

      if (decodedAlarm.monday && day == 1) { weekdayEqualsSettings = true; }
      if (decodedAlarm.tuesday && day == 2) { weekdayEqualsSettings = true; }
      if (decodedAlarm.wednesday && day == 3) { weekdayEqualsSettings = true; }
      if (decodedAlarm.thursday && day == 4) { weekdayEqualsSettings = true; }
      if (decodedAlarm.friday && day == 5) { weekdayEqualsSettings = true; }
      if (decodedAlarm.saturday && day == 6) { weekdayEqualsSettings = true; }
      if (decodedAlarm.sunday && day == 7) { weekdayEqualsSettings = true; }

      if (!weekdayEqualsSettings) { continue; }

      //String alarmName = decodedAlarm.displayName;
      //StartAlarm(decodedAlarm.makeSound, decodedAlarm.doVibrate, "hn");

      if (decodedAlarm.makeSound)
      {
        dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
        dfPlayer.play(1);
      }

      if (decodedAlarm.doVibrate)
      {
        digitalWrite(vibrationPin, HIGH);
      }
      
      if (!decodedAlarm.repeat)
      {
        decodedAlarm.isEnabled = false;
        
        AlarmEncoded encodedAlarm = EncodeAlarm(decodedAlarm);
        alarms[encodedAlarm.idByte] = encodedAlarm;
        UpdateAlarmEEPROM(encodedAlarm.idByte); 
      }

      alarmActive = true;
      alarmActiveVibration = decodedAlarm.doVibrate;
      strcpy(alarmActiveText, decodedAlarm.displayName);
     
      CheckDisplayWakeUp(hour, minute, second, 0.0, true);
    }
  }
}

void StartAlarm(bool makeSound, bool vibrate, String alarmName)
{
    
    alarmActive = true;
}


void SetupData()
{
  dataEEPROM.begin();
  if (!dataEEPROM.isConnected())
  {
    Serial.println("Speicher wurde nicht gefunden.");
    delay(1000);
  }
  else
  {
    Serial.println("Speicher gefunden");
    uint32_t Size = dataEEPROM.determineSize();
    Serial.println(Size);
  }

  LoadData();
}

void SetupTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeInfo;
  if(!getLocalTime(&timeInfo)){
    Serial.println("NTP Server nicht aufrufbar");
    return;
  }
  Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S");
}

void SetupDisplay()
{

  tft.begin();
  tft.fillScreen(DISPLAY_BLACK);
  tft.setTextColor(DISPLAY_WHITE); 
  tft.setTextBackground(DISPLAY_BLACK); 
  tft.setTextSize(2); 
  tft.setCursor(38,55);

  if (!connectedToWiFi)
  {
    tft.print("Keine Verbindung");
    return;
  }

  struct tm _timeInfo;
  if(getLocalTime(&_timeInfo))
  {
    for (int i = 0; i <= 12; i++)
    {
      float tiltAngleRad = (i * segmentSize * 5) * DEG_TO_RAD;
      int xEnd = 64 * sin (tiltAngleRad); 
      int yEnd = 64 * cos (tiltAngleRad);
  
      int xStart = 48 * sin (tiltAngleRad); 
      int yStart = 48 * cos (tiltAngleRad);
      tft.drawLine (xStart, yStart, xEnd, yEnd, DISPLAY_BLUE);
    }
  
    for (int i = 0; i <= _timeInfo.tm_sec; i++)
    {
      AdvanceSecondsGrid(i);
    }
    
    int leHour = _timeInfo.tm_hour;
    int leMinute = _timeInfo.tm_min;
    int leSeconds = _timeInfo.tm_sec;
    tft.print(GetTime(leHour,leMinute));

    CheckDisplayWakeUp(leHour, leMinute, leSeconds, 0.0, true);
    
  }
}

void SetupGyro()
{
  if (!mpu.begin()) 
  {
    Serial.println("MPU 6050 nicht gefunden");
    while (1) 
    {
      delay(10);
    }
  }
  
  Serial.println("MPU6050 gefunden!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

void SetupSound()
{
  
  dfPlayer.begin(hwSerial);
  delay(1000);
  dfPlayer.setTimeOut(6000); 
  dfPlayer.volume(volume);  
  dfPlayer.EQ(DFPLAYER_EQ_BASS);
  dfPlayer.outputDevice(DFPLAYER_DEVICE_SD);
}

void EmptySecondsGrid()
{
  for (int i = 0; i <= 60; i++) 
    {
      float tiltAngleRad = (((i * segmentSize) - 180) * DEG_TO_RAD) * -1; 
      int xEnd = 64 * sin (tiltAngleRad); 
      int yEnd = 64 * cos (tiltAngleRad);
  
      int xStart = 54 * sin (tiltAngleRad); 
      int yStart = 54 * cos (tiltAngleRad);
      tft.drawLine (xStart, yStart, xEnd, yEnd, DISPLAY_BLACK);
    } 
}

void AdvanceSecondsGrid(int i)
{
  // Winkel = Aktuelle Sekunde * Abstandsgröße (360 / 60 = 6) in radianten angegeben
  float tiltAngleRad = (((i * segmentSize) - 180) * DEG_TO_RAD) * -1; 
  int xEnd = 64 * sin (tiltAngleRad); // End Koordinate, Distanz zum Null Punkt; 64
  int yEnd = 64 * cos (tiltAngleRad); // End Koordinate, Distanz zum Null Punkt; 64

  int xStart = 54 * sin (tiltAngleRad); // Start Koordinate, Distanz zum Null Punkt; 54
  int yStart = 54 * cos (tiltAngleRad); // Start Koordinate, Distanz zum Null Punkt; 54

  // Linie wird gezeichnet
  tft.drawLine (xStart, yStart, xEnd, yEnd, DISPLAY_YELLOW);
}

int lastSecond = 0;
int lastMinute = 0;

void loop() 
{

  webServer.handleClient();
  delay(10);
  
  if (!connectedToWiFi) { return; }
  
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float overallAcceleration = a.acceleration.x + a.acceleration.y + a.acceleration.z;
  float overallGeeForce = overallAcceleration / 9.81;



  struct tm _timeInfo;
  if(getLocalTime(&_timeInfo))
  {  
    CheckDisplayWakeUp(_timeInfo.tm_hour, _timeInfo.tm_min, _timeInfo.tm_sec, overallGeeForce);
    CheckDisplaySleep(_timeInfo.tm_hour, _timeInfo.tm_min, _timeInfo.tm_sec);


      if (alarmActive == true)
      {
        if (overallGeeForce >= 2.0)
        {
          dfPlayer.stop();
          
          digitalWrite(vibrationPin, LOW);
          alarmActiveVibration = false;
          strcpy(alarmActiveText, "");
          alarmActive = false;
        }
   
        if (alarmActiveVibration)
        {
          // Meh - Das geht besser. Zwei dimensionalles Array, Sekunde / State
          if (_timeInfo.tm_sec == 1) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 2){ digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 4){ digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 5){ digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 6){ digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 8){ digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 10){ digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 13){ digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 16) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 20) { digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 25) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 29) { digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 40) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 45) { digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 50) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 55) { digitalWrite(vibrationPin, HIGH); }
          else if(_timeInfo.tm_sec == 57) { digitalWrite(vibrationPin, LOW); }
          else if(_timeInfo.tm_sec == 59) { digitalWrite(vibrationPin, HIGH); }
        } 

        if (alarmActiveText != "")
        {
          tft.print(alarmActiveText);
        }
      }
    
    if (_timeInfo.tm_sec != lastSecond)
    {
      lastSecond = _timeInfo.tm_sec;

      if (_timeInfo.tm_sec == 0)
      {
        EmptySecondsGrid();
        AdvanceSecondsGrid(_timeInfo.tm_sec);
      }
      else
      {
        AdvanceSecondsGrid(_timeInfo.tm_sec);
      }
    }

    if (_timeInfo.tm_min != lastMinute)
    {
      lastMinute = _timeInfo.tm_min;
      
      int leHour = _timeInfo.tm_hour;
      int leMinute = _timeInfo.tm_min;
      
      tft.setCursor(38,55);
      tft.print(GetTime(leHour,leMinute));
      CheckAlarms(_timeInfo.tm_hour, _timeInfo.tm_min, _timeInfo.tm_sec, _timeInfo.tm_wday);
    }
  }

  if (dfPlayer.available()) {
    printDetail(dfPlayer.readType(), dfPlayer.read()); 
  }
}

String GetTime(int hour, int minute)
{
  String returnMinute = String(minute);
  String returnHour = String(hour);
  
  if (minute <= 9)
  {
    returnMinute = "0" + String(minute);
  }

  if (hour <= 9)
  {
    returnHour = "0" + String(hour);
  }

  return String(returnHour) + ":" + String(returnMinute);
}


AlarmDecoded DecodeAlarm(AlarmEncoded alarmEncoded)
{
  AlarmDecoded alarm;

  alarm.isEnabled = alarmEncoded.settingsByte1 & B00000001; // 1 Bit zählt nur 
  alarm.makeSound = (alarmEncoded.settingsByte1 & B00000010) >> 1; // 2 Bit zählt nur & ziehe Byte nach vorne
  alarm.doVibrate = (alarmEncoded.settingsByte1 & B00000100) >> 2; // 3 Bit zählt nur & ziehe Byte nach vorne
  alarm.repeat = (alarmEncoded.settingsByte1 & B00001000) >> 3; // 4 Bit zählt nur & ziehe Byte nach vorne

  // SettingsByte1 Bit 4-8 sind ungenutzt!

  alarm.monday    = alarmEncoded.settingsByte2 & B00000001;
  alarm.tuesday   = (alarmEncoded.settingsByte2 & B00000010) >> 1;
  alarm.wednesday = (alarmEncoded.settingsByte2 & B00000100) >> 2;
  alarm.thursday  = (alarmEncoded.settingsByte2 & B00001000) >> 3;
  alarm.friday    = (alarmEncoded.settingsByte2 & B00010000) >> 4;
  alarm.saturday  = (alarmEncoded.settingsByte2 & B00100000) >> 5;
  alarm.sunday    = (alarmEncoded.settingsByte2 & B01000000) >> 6;
  
  strcpy(alarm.displayName, alarmEncoded.displayName);

  alarm.hour = alarmEncoded.timeByte1;
  alarm.minute = alarmEncoded.timeByte2;

  alarm.idByte = alarmEncoded.idByte;

  return alarm;
}

AlarmEncoded EncodeAlarm(AlarmDecoded alarmDecoded)
{
  // Bytes müsses nun in verkehrter Reihenfolge addiert werden.
  // So wird das erst gesetzte Byte am Ende ganz hinten liegen
  // B 0000 0000 -> B 0000 0001 -> B 0000 0010 -> B 0000 0100 -> etc.

  AlarmEncoded alarm;
  
  alarm.settingsByte1 = alarmDecoded.repeat ? 1 : 0 << 3;
  alarm.settingsByte1 += alarmDecoded.doVibrate ? 1 : 0 << 2;
  alarm.settingsByte1 += alarmDecoded.makeSound ? 1 : 0 << 1;
  alarm.settingsByte1 += alarmDecoded.isEnabled ? 1 : 0 << 0;

  alarm.settingsByte2 = alarmDecoded.sunday ? 1 : 0 << 6;
  alarm.settingsByte2 += alarmDecoded.saturday ? 1 : 0   << 5;
  alarm.settingsByte2 += alarmDecoded.friday ? 1 : 0     << 4;
  alarm.settingsByte2 += alarmDecoded.thursday ? 1 : 0   << 3;
  alarm.settingsByte2 += alarmDecoded.wednesday ? 1 : 0  << 2;
  alarm.settingsByte2 += alarmDecoded.tuesday ? 1 : 0    << 1;
  alarm.settingsByte2 += alarmDecoded.monday ? 1 : 0     << 0;

  strcpy(alarm.displayName, alarmDecoded.displayName);

  alarm.timeByte1 = alarmDecoded.hour;
  alarm.timeByte2 = alarmDecoded.minute;

  alarm.idByte = alarmDecoded.idByte;

  return alarm;
}

AlarmEncoded ReadEncodedAlarm(String data)
{
  String dataSeperated[6];
  
  int stringCount = 0;
  while (data.length() > 0)
  {
    int index = data.indexOf(',');
    if (index == -1) 
    {
      dataSeperated[stringCount++] = data;
      break;
    }
    else
    {
      dataSeperated[stringCount++] = data.substring(0, index);
      data = data.substring(index+1);
    }
  }  
        
  AlarmEncoded alarm;
  alarm.settingsByte1 = dataSeperated[0].toInt();
  alarm.settingsByte2 = dataSeperated[1].toInt();
  dataSeperated[2].toCharArray(alarm.displayName, 16);
  alarm.timeByte1 = dataSeperated[3].toInt();
  alarm.timeByte2 = dataSeperated[4].toInt();
  alarm.idByte = dataSeperated[5].toInt();

  return alarm;
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
