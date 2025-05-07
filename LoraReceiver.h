#include <Arduino.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <CayenneLPP.h>
#include "src/config.h"

/*****************************************************************/
/* GLOBAL CONSTS                                                 */
/*****************************************************************/
#ifndef LIGHT_SLEEP_ENABLED
#define LIGHT_SLEEP_ENABLED 1
#endif

#ifndef POWER_MODE
#define POWER_MODE PowerMode::HIGH_POWER_MODE
#endif

#define mS_TO_S_FACTOR 1000    // us
#define uS_TO_S_FACTOR 1000000 // us
#define ONE_HOUR_US (3600ULL * uS_TO_S_FACTOR)
#define SERIAL_BAUD 115200     // bps
#define RFM95_COMM_FREQ 868E6
#define RFM95_SEND_RATE SendRate::ONE_MINUTE
#define INACTIVITY_THRESHOLD_MS (5 * RFM95_SEND_RATE * mS_TO_S_FACTOR)
#define WIFI_CONNECTION_TIMEOUT_MS 20000 // ms
#define NO_OF_RECEIVED_DATA 7
#define LPP_DATA_ID_SIZE 1      // byte
#define LPP_DATA_CHANNEL_SIZE 1 // byte
#define LPP_DATA_SIZE (LPP_TEMPERATURE_SIZE +         \
                      LPP_RELATIVE_HUMIDITY_SIZE +   \
                      LPP_BAROMETRIC_PRESSURE_SIZE + \
                      LPP_DIGITAL_INPUT_SIZE +       \
                      (LPP_PERCENTAGE_SIZE * 3)) // bytes
#define PAYLOAD_SIZE ((LPP_DATA_ID_SIZE * NO_OF_RECEIVED_DATA) +      \
                      (LPP_DATA_CHANNEL_SIZE * NO_OF_RECEIVED_DATA) + \
                      LPP_DATA_SIZE) // bytes

#define RFM95_RESET_PIN 25
#define RFM95_DIO0_PIN 26
#define SPI_CS0_PIN 5
#define SPI_SCLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23
#define BUZZER_PIN 27
#define DEBUG_MODE_EN_PIN 14

#define DEVICE_ID 0
#define BME_280_TEMPERATURE_SENSOR_ID 0
#define BME_280_HUMIDITY_SENSOR_ID 1
#define BME_280_PRESSURE_SENSOR_ID 2
#define UV_SENSOR_ID 3
#define SOIL_MOISTURE_SENSOR_ID 4
#define RAIN_SENSOR_ID 5
#define BAT_LEVEL_ID 6

/*****************************************************************/
/* PREDIFINED CONSTS FOR OPTIMIZING POWER CONSUMPTION- START     */
/*****************************************************************/
#define HIGH_POWER_SPREADING_FACTOR 12
#define MEDIUM_POWER_SPREADING_FACTOR 10
#define LOW_POWER_SPREADING_FACTOR 7

#define HIGH_AND_MEDIUM_POWER_SIGNAL_BANDWIDTH 125E3 // Hz
#define LOW_POWER_SIGNAL_BANDWIDTH 250E3 // Hz

#define HIGH_POWER_CODING_RATE_DENOMINATOR 8           // 4/8
#define MEDIUM_AND_LOW_POWER_CODING_RATE_DENOMINATOR 5 // 4/5
/*****************************************************************/
/* PREDIFINED CONSTS FOR OPTIMIZING POWER CONSUMPTION- END       */
/*****************************************************************/

/*****************************************************************/
/* GLOBAL VARIABLES                                              */
/*****************************************************************/
extern volatile bool isPacketReceived;
extern uint8_t* receivedPayload;
extern int16_t packetRSSI;
extern float packetSNR;

/*****************************************************************/
/* STRUCTURES                                                    */
/*****************************************************************/
struct MeasureData
{
  float temperature;
  float humidity;
  float pressure;
  uint8_t uvIndex;
  uint8_t soilMoisture;
  uint8_t rainPercent;
  uint8_t batLevel;
};

/*****************************************************************/
/* ENUMS                                                         */
/*****************************************************************/
enum PowerMode
{
  LOW_POWER_MODE,
  MEDIUM_POWER_MODE,
  HIGH_POWER_MODE
};

enum SendRate
{
  TEN_SECONDS = 10, // s
  ONE_MINUTE = 60, // s
  FIVE_MINUTES = 300, // s
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

///////////////////////////////////////////////////////////////
/// Connect to a WiFi network with given ssid and password.
void connectToWifi(const char *ssid, const char *password);

///////////////////////////////////////////////////////////////
/// Set the LoRa parameters for the given power mode.
void setLoRaParametersFor(PowerMode powerMode);

/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

///////////////////////////////////////////////////////////////
/// Callback function called after channel activity was detected.
void onCadDone(boolean signalDetected);

///////////////////////////////////////////////////////////////
/// Callback function called when packet is received.
void onReceive(int packetSize);

///////////////////////////////////////////////////////////////
/// Check if the debug mode switch is ON.
bool isDebugMode();

///////////////////////////////////////////////////////////////
/// Calculates the light sleep time in us
/// based on the given send rate.
float getLightSleepTime_uS(SendRate sendRate);

///////////////////////////////////////////////////////////////
/// Play a beeping sound using the Piezo Buzzer.
void playBeepingSound();

///////////////////////////////////////////////////////////////
/// Convert the received Low Power Payload to a JSON array.
void decodePacketToJsonArray(JsonArray &JSONArrayPacket);

void parseJsonArrayPacketToMeasureDataStruct(const JsonArray &JSONArrayPacket, MeasureData &measureData);

///////////////////////////////////////////////////////////////
/// Create a JSON string payload for the HTTP POST request.
void createPayloadForHTTPRequest(const JsonArray &JSONArrayPacket, String &HTTPPayload);

///////////////////////////////////////////////////////////////
/// Send the JSON string payload via HTTP POST request.
void sendPayloadViaHTTPRequest(String &HTTPPayload);

///////////////////////////////////////////////////////////////
/// Prints the measured data to the Serial Monitor.
void printMeasureDataToSerialMonitor(MeasureData &measureData);

///////////////////////////////////////////////////////////////
/// Stop the used libraries.
void endLibraries();

///////////////////////////////////////////////////////////////
/// Enter deep sleep mode for an hour.
void enterDeepSleepForAnHour();

///////////////////////////////////////////////////////////////
/// If not receiving packets, enter deep sleep mode.
void handleInactivity(unsigned long &lastActivityTime);
