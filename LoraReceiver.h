#include <Arduino.h>
#include <LoRa.h>
#include <esp_task_wdt.h>

/*****************************************************************/
/* GLOBAL CONSTS                                                 */
/*****************************************************************/
#define RFM95_CS0_PIN 5
#define RFM95_DIO0_PIN 17
#define RFM95_RESET_PIN 16
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SCLK_PIN 18
#define SPI_CS0_PIN 5

/*****************************************************************/
/* GLOBAL VARIABLES                                              */
/*****************************************************************/
extern bool is_packet_received;

/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

  ///////////////////////////////////////////////////////////////
  /// Read the message in the packet.
String readPacket();

  ///////////////////////////////////////////////////////////////
  /// Callback function called when packet is received.
void onReceive(int packetSize);
