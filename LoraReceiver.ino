#include <LoraReceiver.h>
#include <SPI.h>
#include "config.h"

WeatherData weatherData;
String JSONPacket;
unsigned long lastActivityTime = 0;

void setup()
{
  initializeSerialCommunication();
  
  setCpuFrequencyMhz(80); // MHz

  connectToWifi(ssid, password);

  configureGPIO();

  SPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS0_PIN); // set SPI pins
  LoRa.setSPI(SPI);
  
  LoRa.setPins(SPI_CS0_PIN, RFM95_RESET_PIN, RFM95_DIO0_PIN);

  if (!LoRa.begin(RFM95_COMM_FREQ)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onCadDone(onCadDone); // register channel activity detection callback
  LoRa.onReceive(onReceive); // register packet receive callback
  LoRa.channelActivityDetection(); // put the radio into CAD mode

  esp_sleep_enable_timer_wakeup(ESP_WAKE_UP_PERIOD_US);

  lastActivityTime = millis(); // now
}

void loop()
{
  if(is_packet_received) {
    is_packet_received = false;

    lastActivityTime = millis();

    // create a JSON document
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray JSONArrayPacket = jsonBuffer.to<JsonArray>();

    decodePacketToJsonArray(getReceivedMessage(), JSONArrayPacket);
    serializeJson(JSONArrayPacket, JSONPacket);

    parseJsonArrayPacketToWeatherDataStruct(JSONArrayPacket, weatherData);

    #if DEBUG_MODE
      printWeatherDataToSerialMonitor(weatherData);
    #endif

    // disconnect from network and turn the radio off
    WiFi.disconnect(true);

    esp_light_sleep_start();

    // after wakeup
    connectToWifi(ssid, password);

    LoRa.begin(RFM95_COMM_FREQ); // reset library
    LoRa.channelActivityDetection();
  }

  handleInactivity(lastActivityTime);
}
