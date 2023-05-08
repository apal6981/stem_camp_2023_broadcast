#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "pitches.h"

#define TONE_PIN 33
#define LED_PIN 5
#define LED_COUNT 20

#define COM

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

typedef struct
{
  uint8_t part;
  uint16_t num_notes;
} song_meta_data;

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen);
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status);
void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength);
void broadcast(char *message, size_t len);
void setup_broadcaster();
uint16_t checksum_calc(uint16_t *data, int dataLen);
uint8_t check_checksum(uint16_t *data, int dataLen);
void get_header(header &head, uint8_t data);
void get_song_packets(song_note *packets, uint8_t *data, uint8_t num_packets);
void play_note_color(uint16_t index, uint8_t on);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// uint8_t *message_buffer;
// size_t message_len;
static song_note **notes = NULL;
static song_meta_data meta_data = {0, 0xFFFF};

static  uint8_t play_song = 0;
static  uint16_t song_index = 0;
static  unsigned long start_time = 0;
static  unsigned long target_time = 0;
static  unsigned long cur_time = 0;
static  unsigned long new_target_time = 0;
static  unsigned long extra_time = 0;
static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
  // message_buffer = (uint8_t *)malloc(250 * sizeof(uint8_t));
  // put your setup code here, to run once:
  #ifdef COM
  Serial.begin(115200);
  // delay(5000);

  Serial.println("ESPNow Example");
  #endif
  // WiFi.mode(WIFI_STA);

  // Output my MAC address - useful for later
  #ifdef COM
  Serial.print("My MAC Address is: ");
  Serial.println(WiFi.macAddress());
  #endif
  // shut down wifi
  // WiFi.disconnect();

  if (esp_now_init() == ESP_OK)
  {
    #ifdef COM
    Serial.println("ESPNow Init Success");
    #endif
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    #ifdef COM
    Serial.println("ESPNow Init Failed");
    #endif
    delay(3000);
    ESP.restart();
  }
  setup_broadcaster();
  strip.begin();
  strip.show();
  strip.setBrightness(127);
  // clear out the serial buffer of all garbage
  // Serial.readBytes((char *)NULL, Serial.available());
  play_song = 0;
  song_index = 0;
  start_time = 0;
  target_time = 0;
  cur_time = 0;
  extra_time = 0;
}

void loop()
{
  // put your main code here, to run repeatedly:
  // Serial.println(millis());
  // broadcast(buffer,10);
  // delay(1000);
  #ifdef COM
  if (Serial.available() > 0) // used for receiving input from the host computer for sending information
  {
    pinMode(TONE_PIN, OUTPUT);
    analogWrite(TONE_PIN, 0);
    char first[1];
    // Serial.printf("%d ", Serial.available());
    Serial.readBytes(first, 1);
    uint8_t num_bytes = (uint8_t)first[0];
    char _bytes[num_bytes];
    int read = Serial.readBytes(_bytes, num_bytes);
    broadcast(_bytes, num_bytes);

    // Serial.write(_bytes);
    // uint16_t *_16bytes = (uint16_t *)_bytes;
    // // Serial.printf("%d %d  %d \t%d ", ntohs(_16bytes[0]), ntohs(_16bytes[1]), ntohs(_16bytes[2]), read);
    // if (!check_checksum(_16bytes, num_bytes / 2 - 1))
    // {
    //   Serial.println("Checksum failed");
    //   return;
    // }
    // // Serial.printf("%d %d %d + %d = %d, %d", num_bytes, num_bytes / 2 - 1, ntohs(_16bytes[0]), checksum_calc(_16bytes + 1, num_bytes / 2 - 1), ntohs(_16bytes[0]) + checksum_calc(_16bytes + 1, num_bytes / 2 - 1), check_checksum(_16bytes, num_bytes / 2 - 1));
    // header p_head;
    // get_header(p_head, (uint8_t)_bytes[2]);
    // Serial.printf("%d %d %d    ", p_head.p_type, p_head.num_packets, p_head.repeat);
    // song_note *notes = (song_note *)malloc(2 * sizeof(song_note));
    // get_song_packets(notes, (uint8_t *)(_bytes + 3), p_head.num_packets);
    // Serial.printf("%d %d %d %d %d %d %d     %d %d %d %d %d %d %d", notes[0].sequence, notes[0].duration, notes[0].part, notes[0].red, notes[0].green, notes[0].blue, notes[0].note, notes[1].sequence, notes[1].duration, notes[1].part, notes[1].red, notes[1].green, notes[1].blue, notes[1].note);
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
  #endif
  if (play_song)
  {
    cur_time = millis();
    if (cur_time >= target_time)
    {
      song_index++;
      if (song_index >= meta_data.num_notes)
      {
        play_song = 0;
        play_note_color(0, 0);
        return;
      }
      new_target_time = target_time + notes[song_index]->duration;
      #ifdef COM
      Serial.printf("index: %d, new target time: %d, Target_time: %d, current time: %d, duration: %d\n", song_index, new_target_time, target_time, cur_time, notes[song_index]->duration);
      #endif
      target_time = new_target_time;
      play_note_color(song_index, 1);
    }
  }
}

void play_note_color(uint16_t index, uint8_t on)
{
  if (index >= meta_data.num_notes) // note enough notes so don't do anything
  {
    tone(TONE_PIN, 0);
    strip.clear();
    strip.show();
    return;
  }
  // Serial.printf("Drawing note %d\n", index);
  if (on)
  {
    // tone(TONE_PIN, note_pitches[notes[song_index].note - PITCH_OFFSET]);
    for (int i = 0; i < LED_COUNT; i++)
    {
      strip.setPixelColor(i, notes[song_index]->red, notes[song_index]->green, notes[song_index]->blue);
    }
  }
  else
  {
    tone(TONE_PIN, 0);
    // analogWrite(TONE_PIN, 0);
    strip.clear();
  }
  strip.show();
}

void get_song_packets(song_note **packets, uint8_t *data, uint8_t num_packets)
{
  uint8_t index = 0;
  for (int i = 0; i < num_packets; i++)
  {
    uint16_t seq = ntohs(*((uint16_t *)((data + index))));
    packets[seq] = (song_note*)malloc(sizeof(song_note));
    packets[seq]->sequence = seq;
    packets[seq]->duration = ntohs(*((uint16_t *)((data + index + 2))));
    packets[seq]->part = data[index + 4];
    packets[seq]->red = data[index + 5];
    packets[seq]->green = data[index + 6];
    packets[seq]->blue = data[index + 7];
    packets[seq]->note = data[index + 8];
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
  // char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  // int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  // strncpy(buffer, (const char *)data, msgLen);
  // // make sure we are null terminated
  // buffer[msgLen] = 0;
  // // format the mac address
  // char macStr[18];
  // formatMacAddress(macAddr, macStr, 18);
  // // debug log the message to the serial port
  // Serial.printf("Received message from: %s - %s\n", macStr, buffer);
  // Serial.write(_bytes);
  uint16_t *_16bytes = (uint16_t *)data;
  // Serial.printf("%d %d  %d \t%d ", ntohs(_16bytes[0]), ntohs(_16bytes[1]), ntohs(_16bytes[2]), read);
  if (!check_checksum(_16bytes, dataLen / 2 - 1))
  {
    #ifdef COM
    Serial.println("Checksum failed");
    #endif
    return;
  }
  // checksum was good now grab the header off
  // Serial.printf("%d %d %d + %d = %d, %d", num_bytes, num_bytes / 2 - 1, ntohs(_16bytes[0]), checksum_calc(_16bytes + 1, num_bytes / 2 - 1), ntohs(_16bytes[0]) + checksum_calc(_16bytes + 1, num_bytes / 2 - 1), check_checksum(_16bytes, num_bytes / 2 - 1));
  header p_head;
  get_header(p_head, (uint8_t)data[2]);
  // Serial.printf("%d %d %d    ", p_head.p_type, p_head.num_packets, p_head.repeat);
  // now grab and do specific things dependent on packet type
  switch (p_head.p_type)
  {
  case 0: // Announcment of song types and how many notes
  {
    if (p_head.repeat && meta_data.num_notes != 0xFFFF)
    { // already received the announcment packet so skip redoing
      break;
    }
    // process announcment data
    uint8_t number_of_parts = (uint8_t)data[3];
    meta_data.part = esp_random() % number_of_parts;
    meta_data.num_notes = ntohs(*((uint16_t *)(data + 4 + meta_data.part * 2)));
    notes = (song_note **)malloc(meta_data.num_notes * sizeof(song_note*));
    // notes = ()
    #ifdef COM
    Serial.printf("Processed announcment: %d, choose %d with %d notes\n", number_of_parts, meta_data.part, meta_data.num_notes);
    #endif
    song_index = 0;
    play_song = 0;
    extra_time = 0;
    break;
  }

  case 1: // Song notes and colors
    // need to check to see if part matches
    if (*(uint8_t *)(data + 7) != meta_data.part)
    {
      break;
    }
    #ifdef COM
    Serial.printf("getting song notes: %d\n", p_head.num_packets);
    #endif
    get_song_packets(notes, (uint8_t *)(data + 3), p_head.num_packets);
    #ifdef COM
    for (int i = 0; i < meta_data.num_notes; i++)
    {
      Serial.printf("note: %d, %d %d %d %d %d %d %d\n", i, notes[i]->sequence, notes[i]->duration, notes[i]->part, notes[i]->red, notes[i]->green, notes[i]->blue, notes[i]->note);
    }
    #endif
    break;
  case 2: // Start playing the song
  #ifdef COM
    Serial.printf("Play the song\n");
    #endif
    if (meta_data.num_notes == 0xFFFF || song_index >= meta_data.num_notes) // haven't loaded a song yet so don't start
    {
      #ifdef COM
      Serial.printf("No song loaded so can't play or past the song info\n");
      #endif
      return;
    }
    play_song = 1;
    if (extra_time)
    {
      #ifdef COM
      Serial.printf("Extra time for note %d", extra_time);
      #endif
      target_time = cur_time + extra_time;
      extra_time = 0;
    }
    else
    {
      // cur_time = millis();
      target_time = cur_time + notes[song_index]->duration;
      #ifdef COM
      Serial.printf("in start function, index: %d, Target_time: %d, current time: %d, duration: %d\n", song_index, target_time, cur_time, notes[song_index]->duration);
      #endif
    }
    play_note_color(song_index, 1);
    break;
  case 3: // stop playing the song
    #ifdef COM
    Serial.printf("Stop the song\n");
    #endif
    play_song = 0;
    extra_time = target_time - cur_time;
    play_note_color(0, 0); // turn off sound and color
    break;
  case 4: // reset the song stuff
    #ifdef COM
    Serial.printf("Reset the song man\n");
    #endif
    play_note_color(0, 0); // turn off sound and color
    for (int i =0; i < meta_data.num_notes; i++) {
      free(notes[i]);
    }
    free(notes);
    meta_data.part = 0;
    meta_data.num_notes = 0xFFFF;
    song_index = 0;
    play_song = 0;
    extra_time = 0;
    break;
  default: // anything else
    break;
  }
  // song_note *notes = (song_note *)malloc(2 * sizeof(song_note));
  // get_song_packets(notes, (uint8_t *)(data + 3), p_head.num_packets);
  // Serial.printf("%d %d %d %d %d %d %d     %d %d %d %d %d %d %d", notes[0].sequence, notes[0].duration, notes[0].part, notes[0].red, notes[0].green, notes[0].blue, notes[0].note, notes[1].sequence, notes[1].duration, notes[1].part, notes[1].red, notes[1].green, notes[1].blue, notes[1].note);
}

void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  #ifdef COM
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  #endif
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}