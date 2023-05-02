#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
void broadcast(char *message,size_t len);
void setup_broadcaster();

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t *message_buffer;
size_t message_len;

void setup() {
  message_buffer = (uint8_t *)malloc(250*sizeof(uint8_t));
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(5000);

  Serial.println("ESPNow Example");
  WiFi.mode(WIFI_STA);
  
  // Output my MAC address - useful for later
  Serial.print("My MAC Address is: ");
  Serial.println(WiFi.macAddress());
  // shut down wifi
  WiFi.disconnect();

  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESPNow Init Failed");
    delay(3000);
    ESP.restart();
  }
  setup_broadcaster();
}


void loop() {
  // put your main code here, to run repeatedly:
  // Serial.println(millis());
  // broadcast(buffer,10);
  // delay(1000);
}

void setup_broadcaster() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
}

void broadcast(char *message,size_t len) {
  // this will broadcast a message to everyone in range
  
  
  esp_err_t result = esp_now_send(broadcastAddress,(const uint8_t *) message, len);

}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen) {
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
  // make sure we are null terminated
  buffer[msgLen] = 0;
  // format the mac address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  // debug log the message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);
}

void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status) {
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength) {
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}