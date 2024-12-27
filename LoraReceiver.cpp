#include <LoraReceiver.h>
#include <esp_task_wdt.h>

/*****************************************************************/
bool is_packet_received = false;

/*****************************************************************/
String readPacket() {
  String message;

  while (LoRa.available()) {
    message += ((char)LoRa.read());
  }

  Serial.println(message);

  // print RSSI of packet(dBm)
  // Serial.print("RSSI ");
  // Serial.println(LoRa.packetRssi());

  return message;
}

WeatherData deSerializeWeatherData(const String& string)
{
  JsonDocument doc;
  deserializeJson(doc, string);

  WeatherData weatherData;
  JsonArray data = doc["data"];
  weatherData.temperature = (float) data[0];
  weatherData.humidity = (float) data[1];
  weatherData.pressure = (float) data[2];
  weatherData.uvIndex = (uint8_t) data[3];
  weatherData.soilMoisture = (uint8_t) data[4];
  weatherData.rainPercent = (uint8_t) data[5];

  return weatherData;
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
                "---------------------------------------------------------\n", 
                weatherData.temperature,
                weatherData.humidity,
                weatherData.pressure,
                weatherData.uvIndex,
                weatherData.soilMoisture,
                weatherData.rainPercent);
}

/*****************************************************************/
void onReceive(int packetSize) {
  is_packet_received = true;
}
