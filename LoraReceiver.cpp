#include <LoraReceiver.h>

volatile bool is_packet_received = false;
char receivedMessage[PAYLOAD_SIZE + 1];

void initializeSerialCommunication()
{
  Serial.begin(SERIAL_BAUD);
  while(!Serial) {} // wait until the serial port is ready and connected
  delay(1000); // wait for Serial Monitor to initialize
}

void configureGPIO()
{
  pinMode(RFM95_RESET_PIN, OUTPUT);
  pinMode(RFM95_DIO0_PIN, INPUT);
}

void convertHexStringToByteArray(const String& hexString, uint8_t* byteArray, size_t& byteArraySize)
{
  byteArraySize = hexString.length() / 2; // each byte is represented with two hex characters
  for (size_t i = 0; i < byteArraySize; i++) {
    sscanf(hexString.substring(i * 2, i * 2 + 2).c_str(), "%02hhX", &byteArray[i]);
  }
}

void decodePacketToJsonArray(const String& hexString, JsonArray& JSONArrayPacket)
{
  uint8_t buffer[PAYLOAD_SIZE];
  size_t bufferSize;

  convertHexStringToByteArray(hexString, buffer, bufferSize);

  CayenneLPP lpp(PAYLOAD_SIZE);
  lpp.decode(buffer, bufferSize, JSONArrayPacket);
}

void parseJsonArrayPacketToWeatherDataStruct(const JsonArray& JSONArrayPacket, WeatherData& weatherData)
{
  for (JsonObject obj : JSONArrayPacket) {
    String dataTypeName = obj["name"];
    float value = obj["value"];

    if (dataTypeName == "temperature") {
        weatherData.temperature = value;
    }
    else if (dataTypeName == "humidity") {
        weatherData.humidity = value;
    }
    else if (dataTypeName == "pressure") {
        weatherData.pressure = value;
    }
    else if (dataTypeName == "digital_out") {
      switch ((uint8_t)obj["channel"]) {
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

void printWeatherDataToSerialMonitor(WeatherData& weatherData)
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
                LoRa.rssi());
  delay(100); // Small delay to prevent buffer overflow
}

void onCadDone(boolean signalDetected)
{
  if (signalDetected) {
    LoRa.receive(); // put the radio into continuous receive mode
  }
}

void onReceive(int packetSize)
{
  memset(receivedMessage, 0, sizeof(receivedMessage)); // clear previous message

  for (int i = 0; i < PAYLOAD_SIZE; i++) {
    receivedMessage[i] = ((char)LoRa.read());
  }

  receivedMessage[PAYLOAD_SIZE] = '\0';

  LoRa.end(); // put the radio into sleep mode & stop SPI connection
  is_packet_received = true;
}

String getReceivedMessage() {
  return String(receivedMessage);
}
