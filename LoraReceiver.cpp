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
}

void connectToWifi(const char *ssid, const char *password)
{
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  unsigned long connectStartTime = millis();

#if DEBUG_MODE
  Serial.print("Connecting to WiFi");
#endif
  while (WiFi.status() != WL_CONNECTED && (millis() - connectStartTime) < WIFI_CONNECTION_TIMEOUT_MS)
  {
    delay(500);
#if DEBUG_MODE
    Serial.print(".");
#endif
  }

  if (WiFi.status() == WL_CONNECTED)
  {
#if DEBUG_MODE
    Serial.println("\nWiFi connected");
#endif
  }
  else
  {
    enterDeepSleepForever();
#if DEBUG_MODE
    Serial.println("\nWiFi connection failed");
#endif
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

void sendPacketViaHTTPRequest(String &JSONPacket)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(JSONPacket); // send request

#if DEBUG_MODE
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
#endif

    http.end(); // free resources
  }
}

void parseJsonArrayPacketToWeatherDataStruct(const JsonArray &JSONArrayPacket, WeatherData &weatherData)
{
  for (JsonObject obj : JSONArrayPacket)
  {
    String dataTypeName = obj["name"];
    float value = obj["value"];

    if (dataTypeName == "temperature")
    {
      weatherData.temperature = value;
    }
    else if (dataTypeName == "humidity")
    {
      weatherData.humidity = value;
    }
    else if (dataTypeName == "pressure")
    {
      weatherData.pressure = value;
    }
    else if (dataTypeName == "digital_out")
    {
      switch ((uint8_t)obj["channel"])
      {
      case UV_SENSOR_IDENTIFIER:
        weatherData.uvIndex = (uint8_t)value;
        break;
      case SOIL_MOISTURE_SENSOR_IDENTIFIER:
        weatherData.soilMoisture = (uint8_t)value;
        break;
      case RAIN_SENSOR_IDENTIFIER:
        weatherData.rainPercent = (uint8_t)value;
        break;
      }
    }
  }
}

void printWeatherDataToSerialMonitor(WeatherData &weatherData)
{
  Serial.printf("Temp: %.1f °C\n"
                "Humidity: %.1f%% RH\n"
                "Pressure: %.1f hPa\n"
                "UV index: %d\n"
                "Soil moisture: %d%%\n"
                "Rain intensity: %d%%\n"
                "With RSSI: %d dBm\n"
                "---------------------------------------------------------\n",
                weatherData.temperature,
                weatherData.humidity,
                weatherData.pressure,
                weatherData.uvIndex,
                weatherData.soilMoisture,
                weatherData.rainPercent,
                packetRSSI);
  Serial.flush();
}

void endLibraries()
{
  LoRa.end(); // put the RFM95 in sleep mode & disable spi bus
  WiFi.disconnect(true);
}

void enterDeepSleepForever()
{
#if DEBUG_MODE
  Serial.println("Going to deep sleep.");
#endif

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
