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
  Serial.print("RSSI ");
  Serial.println(LoRa.packetRssi());

  return message;
}

/*****************************************************************/
void onReceive(int packetSize) {
  is_packet_received = true;
}
