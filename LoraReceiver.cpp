#include <LoraReceiver.h>
#include <HTTPClient.h>

volatile bool isPacketReceived = false;
uint8_t *receivedPayload;
int16_t packetRSSI = 0;
float packetSNR = 0;

void initializeSerialCommunication()
{
  Serial.begin(SERIAL_BAUD);
  while (!Serial)
  {
  } // wait until the serial port is ready and connected
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
    if (isDebugMode())
    {
      Serial.println("\nWiFi connection failed");
    }
    enterDeepSleepForAnHour();
  }
}

void setLoRaParametersFor(PowerMode powerMode)
{
  switch (powerMode)
  {
  case LOW_POWER_MODE:
    LoRa.setSpreadingFactor(LOW_POWER_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(LOW_POWER_SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(MEDIUM_AND_LOW_POWER_CODING_RATE_DENOMINATOR);
    break;

  case MEDIUM_POWER_MODE:
    LoRa.setSpreadingFactor(MEDIUM_POWER_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(HIGH_AND_MEDIUM_POWER_SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(MEDIUM_AND_LOW_POWER_CODING_RATE_DENOMINATOR);
    break;

  case HIGH_POWER_MODE:
    LoRa.setGain(1); // set gain to maximum
    LoRa.setSpreadingFactor(HIGH_POWER_SPREADING_FACTOR);
    LoRa.setSignalBandwidth(HIGH_AND_MEDIUM_POWER_SIGNAL_BANDWIDTH);
    LoRa.setCodingRate4(HIGH_POWER_CODING_RATE_DENOMINATOR);
    break;
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
    LoRa.channelActivityDetection(); // try next activity detection
  }
}

void onReceive(int packetSize)
{
  static uint8_t payloadBuffer[PAYLOAD_SIZE];
  receivedPayload = payloadBuffer;
  memset(receivedPayload, 0, PAYLOAD_SIZE); // clear the previous payload

  for (int i = 0; i < PAYLOAD_SIZE; i++)
  {
    receivedPayload[i] = LoRa.read();
  }

  packetRSSI = LoRa.packetRssi();
  packetSNR = LoRa.packetSnr();

  //LoRa.end(); // put the radio into sleep mode & disable spi bus
  LoRa.sleep(); // put the radio into sleep mode
  isPacketReceived = true;
}

bool isDebugMode()
{
  return digitalRead(DEBUG_MODE_EN_PIN) == HIGH;
}

float getLightSleepTime_uS(SendRate sendRate)
{
  switch (sendRate)
  {
  case TEN_SECONDS:
    return (0.3f * TEN_SECONDS * uS_TO_S_FACTOR);
  case ONE_MINUTE:
    return (0.8f * ONE_MINUTE * uS_TO_S_FACTOR);
  case FIVE_MINUTES:
    return (0.95f * FIVE_MINUTES * uS_TO_S_FACTOR);
  }
}

void playBeepingSound()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

void decodePacketToJsonArray(JsonArray &JSONArrayPacket)
{
  CayenneLPP lpp(PAYLOAD_SIZE);
  lpp.decode(receivedPayload, PAYLOAD_SIZE, JSONArrayPacket);
}

void parseJsonArrayPacketToMeasureDataStruct(const JsonArray &JSONArrayPacket, MeasureData &measureData)
{
  measureData.temperature = JSONArrayPacket[BME_280_TEMPERATURE_SENSOR_ID]["value"];
  measureData.humidity = JSONArrayPacket[BME_280_HUMIDITY_SENSOR_ID]["value"];
  measureData.pressure = JSONArrayPacket[BME_280_PRESSURE_SENSOR_ID]["value"];
  measureData.uvIndex = JSONArrayPacket[UV_SENSOR_ID]["value"];
  measureData.soilMoisture = JSONArrayPacket[SOIL_MOISTURE_SENSOR_ID]["value"];
  measureData.rainPercent = JSONArrayPacket[RAIN_SENSOR_ID]["value"];
  measureData.batLevel = JSONArrayPacket[BAT_LEVEL_ID]["value"];
}

void createPayloadForHTTPRequest(const JsonArray &JSONArrayPacket, String &HTTPPayload)
{
  JsonDocument doc;

  doc["deviceId"] = DEVICE_ID;
  doc["packetRSSI"] = packetRSSI;
  doc["packetSNR"] = packetSNR;

  JsonArray measurements = doc.createNestedArray("measurements");
  for (JsonObject jsonPacket : JSONArrayPacket)
  {
    JsonObject measurement = measurements.createNestedObject();
    measurement["sensorId"] = jsonPacket["channel"];
    measurement["value"] = String(jsonPacket["value"]);
  }

  serializeJson(doc, HTTPPayload);
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

void printMeasureDataToSerialMonitor(MeasureData &measureData)
{
  Serial.printf("Temp: %.1f °C\n"
                "Humidity: %.1f%% RH\n"
                "Pressure: %.1f hPa\n"
                "UV index: %d\n"
                "Soil moisture: %d%%\n"
                "Rain intensity: %d%%\n"
                "Battery level: %d%%\n"
                "With RSSI: %d dBm\n"
                "With SNR: %.1f dB\n"
                "---------------------------------------------------------\n",
                measureData.temperature,
                measureData.humidity,
                measureData.pressure,
                measureData.uvIndex,
                measureData.soilMoisture,
                measureData.rainPercent,
                measureData.batLevel,
                packetRSSI,
                packetSNR);
  Serial.flush();
}

void endLibraries()
{
  LoRa.end(); // put the RFM95 in sleep mode & disable spi bus
  WiFi.disconnect(true);
}

void enterDeepSleepForAnHour()
{
  if (isDebugMode())
  {
    Serial.println("Going to deep sleep for an hour.");
  }

  endLibraries();
  esp_sleep_enable_timer_wakeup(ONE_HOUR_US);
  esp_deep_sleep_start();
}

void handleInactivity(unsigned long &lastActivityTime)
{
  if ((millis() - lastActivityTime) > INACTIVITY_THRESHOLD_MS)
  {
    enterDeepSleepForAnHour();
  }
}
