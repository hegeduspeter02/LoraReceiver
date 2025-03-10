#include <LoraReceiver.h>
#include <CayenneLPP.h>
#include <HTTPClient.h>

volatile bool is_packet_received = false;
char receivedMessage[PAYLOAD_SIZE + 1];
volatile int16_t packetRSSI = 0;

void initializeSerialCommunication()
{
  Serial.begin(SERIAL_BAUD);
  while (!Serial)
  {
  } // wait until the serial port is ready and connected
  delay(100); // wait for Serial Monitor to initialize
}

void configureGPIO()
{
  pinMode(RFM95_RESET_PIN, OUTPUT);
  pinMode(RFM95_DIO0_PIN, INPUT);
  pinMode(DEBUG_MODE_EN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
}

void connectToWifi(const char *ssid, const char *password)
{
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  unsigned long connectStartTime = millis();

  if (isDebugMode())
  {
    Serial.print("Connecting to WiFi");
  }

  while (WiFi.status() != WL_CONNECTED && (millis() - connectStartTime) < WIFI_CONNECTION_TIMEOUT_MS)
  {
    delay(500);
    if (isDebugMode())
    {
      Serial.print(".");
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    if (isDebugMode())
    {
      Serial.println("\nWiFi connected");
    }
  }
  else
  {
    enterDeepSleepForever();
    if (isDebugMode())
    {
      Serial.println("\nWiFi connection failed");
    }
  }
}

void onCadDone(boolean signalDetected)
{
  if (signalDetected)
  {
    LoRa.receive(); // put the radio into continuous receive mode
  }
  else
  {
    LoRa.channelActivityDetection(); // try next activity dectection
  }
}

void onReceive(int packetSize)
{
  memset(receivedMessage, 0, sizeof(receivedMessage)); // clear previous message

  for (int i = 0; i < PAYLOAD_SIZE; i++)
  {
    receivedMessage[i] = ((char)LoRa.read());
  }

  receivedMessage[PAYLOAD_SIZE] = '\0';
  packetRSSI = LoRa.rssi();

  LoRa.end(); // put the radio into sleep mode & disable spi bus
  is_packet_received = true;
}

void playBeepingSound()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

String getReceivedMessage()
{
  return String(receivedMessage);
}

void convertHexStringToByteArray(const String &hexString, uint8_t *byteArray, size_t &byteArraySize)
{
  byteArraySize = hexString.length() / 2; // each byte is represented with two hex characters
  for (size_t i = 0; i < byteArraySize; i++)
  {
    sscanf(hexString.substring(i * 2, i * 2 + 2).c_str(), "%02hhX", &byteArray[i]);
  }
}

void decodePacketToJsonArray(const String &hexString, JsonArray &JSONArrayPacket)
{
  uint8_t buffer[PAYLOAD_SIZE];
  size_t bufferSize;

  convertHexStringToByteArray(hexString, buffer, bufferSize);

  CayenneLPP lpp(PAYLOAD_SIZE);
  lpp.decode(buffer, bufferSize, JSONArrayPacket);
}

void sendPayloadViaHTTPRequest(String &HTTPPayload)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(HTTPPayload); // send request

    if (isDebugMode())
    {
      if (httpResponseCode > 0)
      {
        String responseText = http.getString();
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        Serial.println(responseText);
      }
      else
      {
        Serial.print("Error in sending request, code: ");
        Serial.println(httpResponseCode);
      }
    }

    http.end(); // free resources
  }
}

void parseJsonArrayPacketToMeasureDataStruct(const JsonArray &JSONArrayPacket, MeasureData &measureData)
{
  measureData.temperature = JSONArrayPacket[BME_280_TEMPERATURE_SENSOR_IDENTIFIER]["value"];
  measureData.humidity = JSONArrayPacket[BME_280_HUMIDITY_SENSOR_IDENTIFIER]["value"];
  measureData.pressure = JSONArrayPacket[BME_280_PRESSURE_SENSOR_IDENTIFIER]["value"];
  measureData.uvIndex = JSONArrayPacket[UV_SENSOR_IDENTIFIER]["value"];
  measureData.soilMoisture = JSONArrayPacket[SOIL_MOISTURE_SENSOR_IDENTIFIER]["value"];
  measureData.rainPercent = JSONArrayPacket[RAIN_SENSOR_IDENTIFIER]["value"];
  measureData.batLevel = JSONArrayPacket[BAT_LEVEL_IDENTIFIER]["value"];
}

void createPayloadForHTTPRequest(const JsonArray &JSONArrayPacket, String &HTTPPayload)
{
  JsonDocument doc;

  doc["deviceId"] = DEVICE_ID;

  JsonArray measurements = doc.createNestedArray("measurements");
  for (JsonObject jsonPacket : JSONArrayPacket)
  {
    JsonObject measurement = measurements.createNestedObject();
    measurement["sensorId"] = jsonPacket["channel"];
    measurement["value"] = String(jsonPacket["value"]);
  }

  serializeJson(doc, HTTPPayload);
}

bool isDebugMode()
{
  return digitalRead(DEBUG_MODE_EN_PIN) == LOW;
}

void printMeasureDataToSerialMonitor(MeasureData &measureData)
{
  Serial.printf("Temp: %.1f °C\n"
                "Humidity: %.1f%% RH\n"
                "Pressure: %.1f hPa\n"
                "UV index: %d\n"
                "Soil moisture: %d%%\n"
                "Rain intensity: %d%%\n"
                "With RSSI: %d dBm\n"
                "Battery level: %d%%\n"
                "---------------------------------------------------------\n",
                measureData.temperature,
                measureData.humidity,
                measureData.pressure,
                measureData.uvIndex,
                measureData.soilMoisture,
                measureData.rainPercent,
                packetRSSI,
                measureData.batLevel);
  Serial.flush();
}

void endLibraries()
{
  LoRa.end(); // put the RFM95 in sleep mode & disable spi bus
  WiFi.disconnect(true);
}

void enterDeepSleepForever()
{
  if (isDebugMode())
  {
    Serial.println("Going to deep sleep.");
  }

  endLibraries();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  esp_deep_sleep_start();
}

void handleInactivity(unsigned long &lastActivityTime)
{
  if ((millis() - lastActivityTime) > INACTIVITY_THRESHOLD_MS)
  {
    enterDeepSleepForever();
  }
}
