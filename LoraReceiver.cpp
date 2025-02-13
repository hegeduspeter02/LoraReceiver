#include <LoraReceiver.h>
#include <esp_task_wdt.h>

/*****************************************************************/
volatile bool is_packet_received = false;

/*****************************************************************/
String readPacket() {
  String message;

  while (LoRa.available()) {
    message += ((char)LoRa.read());
  }

  return message;
}

void convertHexStringToByteArray(const String& hexString, uint8_t* byteArray, size_t& byteArraySize) {
  byteArraySize = hexString.length() / 2; // each byte is represented with two hex characters
  for (size_t i = 0; i < byteArraySize; i++) {
    sscanf(hexString.substring(i * 2, i * 2 + 2).c_str(), "%02hhX", &byteArray[i]);
  }
}

void decodeHexStringToJson(const String& hexString, DynamicJsonDocument& jsonBuffer, JsonArray& root)
{
  uint8_t buffer[MAX_PAYLOAD_SIZE];
  size_t bufferSize;

  convertHexStringToByteArray(hexString, buffer, bufferSize);

  CayenneLPP lpp(MAX_PAYLOAD_SIZE);
  lpp.decode(buffer, bufferSize, root);

  serializeJson(root, Serial);
  Serial.println();
}

/****************************************************************/
void printMeasureToSerialMonitor(WeatherData& weatherData)
{
  Serial.printf("Temp: %.2f\xB0C\n"
                "Humidity: %.2f%% RH\n"
                "Pressure: %.2f hPa\n"
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
                weatherData.packetRSSI);
}

/*****************************************************************/
void onReceive(int packetSize) {
  is_packet_received = true;
}
