#include <LoraReceiver.h>
#include <SPI.h>

WeatherData weatherData;

void setup() {
  esp_task_wdt_deinit();  // This will stop and clear the previously initialized TWDT

  // Configure and reinitialize the TWDT
  esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 10000,       // 10 seconds timeout
      .idle_core_mask = 1 << 0,   // Monitor idle task on core 0 only
      .trigger_panic = false     // Continue running if timeout happens(don't reset)
  };

  esp_task_wdt_init(&twdt_config);

  esp_task_wdt_add(NULL); // subscribe the current running task to the wdt

  Serial.begin(9600);
  while (!Serial);

  SPIClass spi;
  spi.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS0_PIN); // set SPI pins
  
  LoRa.setPins(RFM95_CS0_PIN, RFM95_RESET_PIN, RFM95_DIO0_PIN);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onReceive(onReceive);
  LoRa.receive(); // put the radio into receive mode

  // LoRa.sleep();
}

void loop() {
  if(is_packet_received) {
    is_packet_received = false; 
    String message = readPacket();

    // create a JSON document
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray JSONArrayPacket = jsonBuffer.to<JsonArray>();

    decodePacketToJsonArray(message, JSONArrayPacket);

    String JSONPacket;
    serializeJson(JSONArrayPacket, JSONPacket);
    Serial.println(JSONPacket);

    parseJsonArrayPacketToWeatherDataStruct(JSONArrayPacket, weatherData);

    printMeasureToSerialMonitor(weatherData);

    esp_task_wdt_reset(); // Reset the wdt
  }
}
