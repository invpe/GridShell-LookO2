/*
   This is a simple integration of the LookO2 sensor (https://looko2.com/) with GridShell network.
   The device will poll the sensor for Air Quality Data every 10 minutes and store it as telemetry.

   Board: M5STAMP PICO
   Partition: Default
   Sensor: PMS

   ! Important

   Since this device is using GridShell library we disable auto updates to not wipe out this sketch
   when GridShell version changes. If this happens, you will have to bump up the library version manually.
   You can register a callback to : CGridShell::eEvent::EVENT_VERSIONS_MISMATCH to get notified when there's a need to upgrade.

   (c) invpe https://github.com/invpe 2k23

*/
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <HardwareSerial.h>
/*------------------*/
#include "CGridShell.h"
// Put your grid name here
#define GRID_N ""
// Put your grid hash here
#define GRID_U ""
/*------------------*/
#define WIFI_A ""
#define WIFI_P ""
/*------------------*/
#define NUM_LEDS 4
#define LEDS_PORT 32
#define PICO_LED_PORT 27
#define PICO_LED_COUNT 1
#define LED_BRIGHTNESS 50
#define TELEMETRY_MINUTES 60000ULL * 10
#define AVERAGE_TASK_TIMER 60000ULL * 5
/*------------------*/
// Set your NTP settings here
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
/*------------------*/
Adafruit_NeoPixel mPixels(NUM_LEDS, LEDS_PORT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel mPixelsPico(PICO_LED_COUNT, PICO_LED_PORT, NEO_GRB + NEO_KHZ800);
HardwareSerial m_SensorSerial(1);
uint32_t uiSensorTick = 0;
uint32_t uiAveragesTaskID = 0;
uint32_t uiAPICheckTimer = 0;
/*------------------*/
struct tSensor {
  tSensor() {
    m_strPM1 = "0";
    m_strPM25 = "0";
    m_strPM10 = "0";
    m_strHCHO = "0";
    m_strHumi = "0";
    m_strTemp = "0";
    m_strPress = "0";
    m_strIJP = "0";
    m_bHCHO = false;
  }

  void Start(const bool& rbHCHO) {
    m_SensorSerial.begin(9600, SERIAL_8N1, 1, 10);
    m_bHCHO = rbHCHO;
  }
  void Tick() {
    if (m_SensorSerial.read() == 0x42) {

      if (m_bHCHO) {
        byte tempData[39];

        byte fullPacket[40];
        fullPacket[0] = 0x42;

        // Read the rest
        m_SensorSerial.readBytes(tempData, 39);

        // Rewrite
        for (int a = 0; a < 39; a++)
          fullPacket[a + 1] = tempData[a];

        if (fullPacket[1] == 0x4D) {

          // Calculate Checksum
          unsigned int CR1 = 0, CR2 = 0;
          CR1 = (fullPacket[38] << 8) + fullPacket[39];
          CR2 = 0;
          for (int i = 0; i < 38; i++)
            CR2 += fullPacket[i];

          if (CR1 == CR2) {


            // ug/m3
            uint16_t pm1CF1 = (uint16_t)((fullPacket[4] * 256) + fullPacket[5]);
            uint16_t pm25CF1 = (uint16_t)((fullPacket[6] * 256) + fullPacket[7]);
            uint16_t pm10CF1 = (uint16_t)((fullPacket[8] * 256) + fullPacket[9]);

            uint16_t pm1ATMO = (uint16_t)((fullPacket[10] * 256) + fullPacket[11]);
            uint16_t pm25ATMO = (uint16_t)((fullPacket[12] * 256) + fullPacket[13]);
            uint16_t pm10ATMO = (uint16_t)((fullPacket[14] * 256) + fullPacket[15]);

            uint16_t hcho = (uint16_t)((fullPacket[28] * 256) + fullPacket[29]);
            int16_t temperature = (int16_t)((fullPacket[30] * 256) + fullPacket[31]);
            uint16_t humidity = (uint16_t)((fullPacket[32] * 256) + fullPacket[33]);

            m_strHumi = String(humidity / 10.0);
            m_strTemp = String(temperature / 10.0);
            m_strHCHO = String(hcho);
            m_strPM1 = String(pm1ATMO);
            m_strPM25 = String(pm25ATMO);
            m_strPM10 = String(pm10ATMO);
          }
        }
      } else {
        byte tempData[31];

        byte fullPacket[32];
        fullPacket[0] = 0x42;

        // Read the rest
        m_SensorSerial.readBytes(tempData, 31);

        // Rewrite
        for (int a = 0; a < 31; a++)
          fullPacket[a + 1] = tempData[a];

        //
        if (fullPacket[1] == 0x4D) {

          // Calculate Checksums
          uint16_t uiReceivedSum = 0;

          //
          for (int a = 0; a < 32; a++)
            uiReceivedSum += fullPacket[a];

          // Calculate Checksum
          uint16_t uiSum = (fullPacket[30] * 256 + fullPacket[31]) + fullPacket[30] + fullPacket[31];

          // Calculate to Checksum Received
          if (uiSum == uiReceivedSum) {
            // CF=1 20% higher than athmospheric
            //uint16_t pm1 = (uint16_t)((fullPacket[4] * 256) + fullPacket[5]);
            //uint16_t pm25 = (uint16_t)((fullPacket[6] * 256) + fullPacket[7]);
            //uint16_t pm10 = (uint16_t)((fullPacket[8] * 256) + fullPacket[9]);

            // Under Atmospheric
            uint16_t pm1Atmo = (uint16_t)((fullPacket[10] * 256) + fullPacket[11]);
            uint16_t pm25Atmo = (uint16_t)((fullPacket[12] * 256) + fullPacket[13]);
            uint16_t pm10Atmo = (uint16_t)((fullPacket[14] * 256) + fullPacket[15]);

            m_strPM1 = String(pm1Atmo);
            m_strPM25 = String(pm25Atmo);
            m_strPM10 = String(pm10Atmo);
          }
        }
      }
    }
  }

  String m_strPM1;
  String m_strPM25;
  String m_strPM10;
  String m_strHCHO;
  String m_strHumi;
  String m_strTemp;
  String m_strPress;
  String m_strIJP;
  bool m_bHCHO;
} m_Sensor;

void SetPICOLed(const int& iR, const int& iG, const int& iB) {
  for (int a = 0; a < PICO_LED_COUNT; a++)
    mPixelsPico.setPixelColor(a, mPixels.Color(iR, iG, iB));
  mPixelsPico.show();
}
void SetRGBColor(const int& iR, const int& iG, const int& iB) {
  for (int a = 0; a < NUM_LEDS; a++) {
    mPixels.setPixelColor(a, mPixels.Color(iR, iG, iB));
  }
  mPixels.show();
}
void NotifyLED5Seconds(const int& iR, const int& iG, const int& iB) {
  int a = 0;
  for (a = 0; a < 10; a++) {
    if (a % 2 == 0)
      SetRGBColor(iR, iG, iB);
    else
      SetRGBColor(0, 0, 0);
    yield();
    delay(500);
  }
}
void Reboot() {
  ESP.restart();
}
String GetMACAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  String strUniqueID = "";
  for (int i = 0; i < 6; i++) strUniqueID += String(mac[i], HEX);

  strUniqueID.toLowerCase();
  return strUniqueID;
}
void setup() {

  Serial.begin(115200);

  ///////////////////////////////////////////////////
  // Initialize leds :)                            //
  ///////////////////////////////////////////////////
  mPixels.begin();
  mPixels.setBrightness(LED_BRIGHTNESS);
  mPixelsPico.begin();
  mPixelsPico.setBrightness(LED_BRIGHTNESS);
  SetRGBColor(255, 255, 255);
  SetPICOLed(255, 255, 255);

  ///////////////////////////////////////////////////
  // Initialize Flash storage                      //
  ///////////////////////////////////////////////////
  Serial.println("Mounting FS...");
  while (!SPIFFS.begin()) {
    SPIFFS.format();
    Serial.println("Failed to mount file system");
    delay(1000);
  }

  ///////////////////////////////////////////////////
  // Initialize WiFi connection                    //
  ///////////////////////////////////////////////////
  WiFi.setHostname("GSLOOKO2");
  WiFi.begin(WIFI_A, WIFI_P);

  // Give it 10 seconds to connect, otherwise reboot
  uint8_t iRetries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    delay(1000);
    iRetries += 1;

    if (iRetries >= 10)
      ESP.restart();
  }

  Serial.println("Connected " + WiFi.localIP().toString());

  /////////////////////////////////////////////////////////////
  // Turn OTA, since programming stamp with pins is tricky   //
  /////////////////////////////////////////////////////////////

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";
    })
    .onEnd([]() {
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      yield();
    })
    .onError([](ota_error_t error) {
      ESP.restart();
    });
  ArduinoOTA.setHostname("GSLOOKO2");
  ArduinoOTA.begin();


  ///////////////////////////////////////////////////
  // Initialize GridShell Node with your user hash//
  ///////////////////////////////////////////////////

  // Do not enable Auto Updates - you will have to update the sketch yourself
  // Since this is not a Vanilla Node
  if (CGridShell::GetInstance().Init(GRID_U, false) == true) {
  } else {
    // Light up to inform issues with setting up grid
    NotifyLED5Seconds(255, 0, 0);
    ESP.restart();
  }

  // Fire off NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Fire off Sensor
  m_Sensor.Start(false);

  // Light up to inform we're good to go
  NotifyLED5Seconds(0, 255, 0);
}
void loop() {

  // Check if WiFi available
  if (WiFi.status() != WL_CONNECTED) {
    // Light up to note we're having WiFi issues
    NotifyLED5Seconds(255, 255, 0);
    ESP.restart();
  }

  // Process ota
  ArduinoOTA.handle();

  // Tick the GS
  CGridShell::GetInstance().Tick();

  // Tick every TELEMETRY_MINUTES
  if (millis() - uiSensorTick >= TELEMETRY_MINUTES) {
    // Tick the PMS sensor
    m_Sensor.Tick();

    // Extract the time from NTP server
    tm local_tm;
    if (!getLocalTime(&local_tm)) {
      Serial.println("Failed to obtain time");
      return;
    }
    // Get epoch
    time_t timeSinceEpoch = mktime(&local_tm);

    // Ensure grid online & store telemetry
    if (CGridShell::GetInstance().Connected()) {

      // Mark adding tasks
      SetPICOLed(255, 255, 255);

      String strPayload = "";
      uint32_t uiTaskID = 0;

      String strFileSettings = "L2" + String(local_tm.tm_year + 1900) + String(local_tm.tm_mon + 1) + String(local_tm.tm_mday) + ",1,";
      strPayload = String(timeSinceEpoch) + ",";
      strPayload += GetMACAddress() + ",";
      strPayload += m_Sensor.m_strPM1 + ",";
      strPayload += m_Sensor.m_strPM25 + ",";
      strPayload += m_Sensor.m_strPM10 + ",";
      strPayload += m_Sensor.m_strTemp + ",";
      strPayload += m_Sensor.m_strHumi + ",";
      strPayload += m_Sensor.m_strHCHO + ",";
      strPayload += m_Sensor.m_strPress + ",";
      strPayload += m_Sensor.m_strIJP + ",";
      strPayload += String(WiFi.RSSI()) + "\n";

      // Write CSV telemetry data (append)
      String strTaskPayload = strFileSettings + CGridShell::GetInstance().EncodeBase64(strPayload) + ",";
      CGridShell::GetInstance().AddTask("writedfs", strTaskPayload);

      // Submit averages, this will determine the colours of the main leds
      String strAveragesFile = GRID_N "L2" + String(local_tm.tm_year + 1900) + String(local_tm.tm_mon + 1) + String(local_tm.tm_mday);
      uint32_t uiNewTaskID = CGridShell::GetInstance().AddTask("l2daily", strAveragesFile + "," + GetMACAddress() + ",");

      if (uiNewTaskID != 0)
        uiAveragesTaskID = uiNewTaskID;

      Serial.println("TASK:" + String(uiAveragesTaskID));

      // Write JSON data (overwrite) - optional
      /*
        strFileSettings = "LOOKO2" + GetMACAddress() + "J,0,";
        strPayload = "{\"Sensor\": \"" + GetMACAddress() + "\",";
        strPayload += "\"PM1\": " + m_Sensor.m_strPM1 + ",";
        strPayload += "\"PM2.5\": " + m_Sensor.m_strPM25 + ",";
        strPayload += "\"PM10\": " + m_Sensor.m_strPM10 + ",";
        strPayload += "\"HCHO\": " + m_Sensor.m_strHCHO + ",";
        strPayload += "\"Temperature\": " + m_Sensor.m_strTemp + ",";
        strPayload += "\"Epoch\": " + String(timeSinceEpoch) + ",";
        strPayload += "\"Time\": \"" + String(local_tm.tm_year + 1900) + String(local_tm.tm_mon + 1) + String(local_tm.tm_mday) + " " + String(local_tm.tm_hour) + ":" + String(local_tm.tm_min) + "\"";
        strPayload += "}";

        strTaskPayload = strFileSettings + CGridShell::GetInstance().EncodeBase64(strPayload) + ",";
        CGridShell::GetInstance().AddTask("writedfs", strTaskPayload);
      */

      // Turn on green, to notify GRID UP
      SetPICOLed(0, 255, 0);
    } else {
      // Grid offline, mark red
      SetPICOLed(255, 0, 0);
    }


    //
    uiSensorTick = millis();
  }

  //
  // Check if uiAveragesTaskID have completed and pull out the last IJP data to shine the leds with index color
  //
  if (millis() - uiAPICheckTimer > AVERAGE_TASK_TIMER && CGridShell::GetInstance().Connected()) {


    if (GRID_N != "" && uiAveragesTaskID != 0) {
      String strExecPayload = CGridShell::GetInstance().GetTask(uiAveragesTaskID);

      if (strExecPayload != "") {
        DynamicJsonDocument jsonBuffer(512);
        deserializeJson(jsonBuffer, CGridShell::GetInstance().DecodeBase64(strExecPayload));

        int iIJP = jsonBuffer["IJP"].as<int>();

        if (iIJP == 0)  // blue
          SetRGBColor(0, 0, 25);
        else if (iIJP == 1 || iIJP == 2)  // green
          SetRGBColor(0, 255, 0);
        else if (iIJP == 3 || iIJP == 4)  // yellow
          SetRGBColor(255, 255, 0);
        else if (iIJP == 5 || iIJP == 6)  // orange
          SetRGBColor(255, 200, 0);
        else if (iIJP >= 7)  // red
          SetRGBColor(255, 0, 0);
      }
    }
    uiAPICheckTimer = millis();
  }
}
