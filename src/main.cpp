#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

typedef struct
{
  unsigned int p_type : 3;
  unsigned int num_packets : 4;
  unsigned int repeat : 1;
} header;

typedef struct
{
  uint16_t sequence;
  uint16_t duration;
  uint8_t part;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t note;
} song_note;

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
void broadcast(char *message, size_t len);
void setup_broadcaster();
uint16_t checksum_calc(uint16_t *data, int dataLen);
uint8_t check_checksum(uint16_t *data, int dataLen);
void get_header(header &head, uint8_t data);
void get_song_packets(song_note *packets, uint8_t *data, uint8_t num_packets);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t *message_buffer;
size_t message_len;

void setup()
{
  message_buffer = (uint8_t *)malloc(250 * sizeof(uint8_t));
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

void loop()
{
  // put your main code here, to run repeatedly:
  // Serial.println(millis());
  // broadcast(buffer,10);
  // delay(1000);
  if (Serial.available() > 0)
  {
    char first[1];
    // Serial.printf("%d ", Serial.available());
    Serial.readBytes(first, 1);
    uint8_t num_bytes = (uint8_t)first[0];
    char _bytes[num_bytes];
    int read = Serial.readBytes(_bytes, num_bytes);
    // Serial.write(_bytes);
    uint16_t *_16bytes = (uint16_t *)_bytes;
    // Serial.printf("%d %d  %d \t%d ", ntohs(_16bytes[0]), ntohs(_16bytes[1]), ntohs(_16bytes[2]), read);
    if (!check_checksum(_16bytes, num_bytes / 2 - 1))
    {
      Serial.println("Checksum failed");
      return;
    }
    // Serial.printf("%d %d %d + %d = %d, %d", num_bytes, num_bytes / 2 - 1, ntohs(_16bytes[0]), checksum_calc(_16bytes + 1, num_bytes / 2 - 1), ntohs(_16bytes[0]) + checksum_calc(_16bytes + 1, num_bytes / 2 - 1), check_checksum(_16bytes, num_bytes / 2 - 1));
    header p_head;
    get_header(p_head, (uint8_t)_bytes[2]);
    Serial.printf("%d %d %d    ", p_head.p_type, p_head.num_packets, p_head.repeat);
    song_note *notes = (song_note *)malloc(2 * sizeof(song_note));
    get_song_packets(notes, (uint8_t *)(_bytes + 3), p_head.num_packets);
    Serial.printf("%d %d %d %d %d %d %d     %d %d %d %d %d %d %d", notes[0].sequence, notes[0].duration, notes[0].part, notes[0].red, notes[0].green, notes[0].blue, notes[0].note, notes[1].sequence, notes[1].duration, notes[1].part, notes[1].red, notes[1].green, notes[1].blue, notes[1].note);
    // char num_bytes[1];
    // Serial.readBytes(num_bytes, 1); // get how many bytes to read
    // char send_bytes[(uint8_t)num_bytes[0] + 1];
    // Serial.readBytes(send_bytes, (uint8_t)num_bytes[0] + 1);
    // Serial.write(send_bytes);
    // Serial.flush();
    // Serial.printf("%d %x\n", Serial.available(), num_bytes);
    // Serial.end();
    // Serial.begin(115200);
  }
}

void get_song_packets(song_note *packets, uint8_t *data, uint8_t num_packets)
{
  uint8_t index = 0;
  for (int i = 0; i < num_packets; i++)
  {
    packets[i].sequence = ntohs(*((uint16_t *)((data + index))));
    packets[i].duration = ntohs(*((uint16_t *)((data + index + 2))));
    packets[i].part = data[index + 4];
    packets[i].red = data[index + 5];
    packets[i].green = data[index + 6];
    packets[i].blue = data[index + 7];
    packets[i].note = data[index + 8];
    index += 9;
  }
}

void get_header(header &head, uint8_t data)
{
  head.p_type = (data & 0b11100000) >> 5;
  head.num_packets = (data & 0b00011110) >> 1;
  head.repeat = data & 0b1;
}

uint16_t checksum_calc(uint16_t *data, int dataLen)
{
  uint16_t checksum = 0;
  for (int i = 0; i < dataLen; i++)
  {
    checksum += ntohs(data[i]);
  }
  return checksum;
}

uint8_t check_checksum(uint16_t *data, int dataLen)
{
  return (ntohs(data[0]) + checksum_calc(data + 1, dataLen)) == 0xFFFF ? 1 : 0;
}

void setup_broadcaster()
{
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
}

void broadcast(char *message, size_t len)
{
  // this will broadcast a message to everyone in range

  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message, len);
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
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

void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}