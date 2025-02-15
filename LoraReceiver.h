#include <Arduino.h>
#include <LoRa.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <CayenneLPP.h>

/*****************************************************************/
/* GLOBAL CONSTS                                                 */
/*****************************************************************/
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#define SERIAL_BAUD 9600 // bps

#define RFM95_RESET_PIN 25
#define RFM95_DIO0_PIN 26

#define SPI_CS0_PIN 5
#define SPI_SCLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define MAX_PAYLOAD_SIZE 51 // bytes
#define BME_280_SENSOR_IDENTIFIER 1
#define UV_SENSOR_IDENTIFIER 2
#define SOIL_MOISTURE_SENSOR_IDENTIFIER 3
#define RAIN_SENSOR_IDENTIFIER 4

/*****************************************************************/
/* GLOBAL VARIABLES                                              */
/*****************************************************************/
extern volatile bool is_packet_received;

/*****************************************************************/
/* STRUCTURES                                                    */
/*****************************************************************/
struct WeatherData {
  float temperature;
  float humidity;
  float pressure;
  uint8_t uvIndex;
  uint8_t soilMoisture;
  uint8_t rainPercent;
  int8_t packetRSSI;
};

/*****************************************************************/
/* INIT FUNCTIONS                                                */
/*****************************************************************/

  ///////////////////////////////////////////////////////////////
  /// Start the Serial communication at SERIAL_BAUD rate.
  /// Wait for the serial port and the Monitor to be ready.
void initializeSerialCommunication();

  ///////////////////////////////////////////////////////////////
  /// Configure the ESP32's pins.
void configureGPIO();

/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

  ///////////////////////////////////////////////////////////////
  /// Read the message in the packet.
String readPacket();

void convertHexStringToByteArray(const String& hexString, uint8_t* byteArray, size_t& byteArraySize);

  ///////////////////////////////////////////////////////////////
  /// Convert the received hexadecimal encoded Low Power Payload 
  /// string to JSON array.
void decodePacketToJsonArray(const String& hexString, JsonArray& JSONArrayPacket);

void parseJsonArrayPacketToWeatherDataStruct(const JsonArray& JSONArrayPacket, WeatherData& weatherData);

  ///////////////////////////////////////////////////////////////
  /// Prints the weatherData to the Serial Monitor.
void printWeatherDataToSerialMonitor(WeatherData& weatherData);

void ARDUINO_ISR_ATTR onDio0Rise();

  ///////////////////////////////////////////////////////////////
  /// Callback function called when packet is received.
void onReceive(int packetSize);
