#include <LoraReceiver.h>
#include <SPI.h>

WeatherData weatherData;
String JSONPacket;

void setup()
{
  esp_task_wdt_deinit(); // stop and clear the previously initialized TWDT

  // configure the watchdog timer
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = TWDT_TIMEOUT_MS,
      .idle_core_mask = (1 << 0) | (1 << 1), // monitor idle task on core 0 and 1
      .trigger_panic = true // reset if timeout happens
  };

  esp_task_wdt_init(&twdt_config);
  esp_task_wdt_add(NULL); // subscribe the current running task to the twdt

  initializeSerialCommunication();

  SPIClass spi;
  spi.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS0_PIN); // set SPI pins

  configureGPIO();
  
  LoRa.setPins(SPI_CS0_PIN, RFM95_RESET_PIN, RFM95_DIO0_PIN);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onCadDone(onCadDone); // register channel activity detection callback
  LoRa.onReceive(onReceive); // register packet receive callback
  LoRa.channelActivityDetection(); // put the radio into CAD mode

  esp_sleep_enable_timer_wakeup(ESP_WAKE_UP_PERIOD_US);
}

void loop()
{
  if(is_packet_received) {
    is_packet_received = false;

    // create a JSON document
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray JSONArrayPacket = jsonBuffer.to<JsonArray>();

    decodePacketToJsonArray(getReceivedMessage(), JSONArrayPacket);
    serializeJson(JSONArrayPacket, JSONPacket);

    parseJsonArrayPacketToWeatherDataStruct(JSONArrayPacket, weatherData);

    #if DEBUG_MODE
        printWeatherDataToSerialMonitor(weatherData);
    #endif

    esp_task_wdt_reset(); // reset the twdt
    esp_light_sleep_start();

    // after wakeup
    LoRa.begin(868E6); // reset library
    LoRa.channelActivityDetection();
  }
}
