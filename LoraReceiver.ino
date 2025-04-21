#include <LoraReceiver.h>
#include <SPI.h>

MeasureData measureData;
String HTTPPayload;
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

  if (!LoRa.begin(RFM95_COMM_FREQ))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setGain(1);
  setLoRaParametersFor(HIGH_POWER_MODE);
  LoRa.onCadDone(onCadDone);       // register channel activity detection callback
  LoRa.onReceive(onReceive);       // register packet receive callback
  LoRa.channelActivityDetection(); // put the radio into CAD mode

  #if LIGHT_SLEEP_ENABLED
    esp_sleep_enable_timer_wakeup(ESP_WAKE_UP_PERIOD_US);
  #endif

  lastActivityTime = millis(); // now
}

void loop()
{
  if (isPacketReceived)
  {
    isPacketReceived = false;
    lastActivityTime = millis();

    if(isDebugMode())
    {
      playBeepingSound();
    }

    JsonDocument jsonBuffer;
    JsonArray JSONArrayPacket = jsonBuffer.to<JsonArray>();

    decodePacketToJsonArray(JSONArrayPacket);
    parseJsonArrayPacketToMeasureDataStruct(JSONArrayPacket, measureData);
    createPayloadForHTTPRequest(JSONArrayPacket, HTTPPayload);

    if (isDebugMode())
    {
      printMeasureDataToSerialMonitor(measureData);
    }

    sendPayloadViaHTTPRequest(HTTPPayload);

    #if LIGHT_SLEEP_ENABLED
      // disconnect from network and turn the radio off
      WiFi.disconnect(true);
      esp_light_sleep_start();

      // after wakeup
      connectToWifi(ssid, password);
    #endif

    LoRa.begin(RFM95_COMM_FREQ); // reset library
    setLoRaParametersFor(HIGH_POWER_MODE);
    LoRa.channelActivityDetection();
  }

  //handleInactivity(lastActivityTime);
}
