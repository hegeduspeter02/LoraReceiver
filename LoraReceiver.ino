#include <LoraReceiver.h>
#include <SPI.h>

WeatherData weatherData;
String JSONPacket;

void setup() {
  /*esp_task_wdt_deinit();  // This will stop and clear the previously initialized TWDT

  // Configure and reinitialize the TWDT
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 10000,       // 10 seconds timeout
      .idle_core_mask = 1 << 0,   // Monitor idle task on core 0 only
      .trigger_panic = false     // Continue running if timeout happens(don't reset)
  };

  esp_task_wdt_init(&twdt_config);

  esp_task_wdt_add(NULL); // subscribe the current running task to the wdt*/

  initializeSerialCommunication();

  SPIClass spi;
  spi.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS0_PIN); // set SPI pins

  configureGPIO();
  
  LoRa.setPins(SPI_CS0_PIN, RFM95_RESET_PIN, RFM95_DIO0_PIN);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onReceive(onReceive); // register the receive callback

  gpio_wakeup_enable((gpio_num_t) RFM95_DIO0_PIN, (gpio_int_type_t) GPIO_INTR_POSEDGE);
  esp_sleep_enable_gpio_wakeup();
}

void loop() {
  if(is_packet_received) {
    is_packet_received = false; 
    String message = readPacket();

    // create a JSON document
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray JSONArrayPacket = jsonBuffer.to<JsonArray>();

    decodePacketToJsonArray(message, JSONArrayPacket);
    serializeJson(JSONArrayPacket, JSONPacket);

    parseJsonArrayPacketToWeatherDataStruct(JSONArrayPacket, weatherData);

    #if DEBUG_MODE
        printWeatherDataToSerialMonitor(weatherData);
    #endif

    LoRa.sleep();
    esp_light_sleep_start();

    //esp_task_wdt_reset(); // Reset the wdt
  }
}
