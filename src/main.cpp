#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp_now.h>
#include <WiFi.h>

const byte numChars = 80;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;
HardwareSerial SerialPort(2);
#define BUTTON_PIN 15
#define MODEMT1_PIN 13
#define MODEMT2_PIN 12
#define MODEAND_PIN 14

// Variables to store the current and previous states of the button
int buttonState;
int lastButtonState = LOW;
// Debounce delay
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
// MAC Address of ESP32 Receiver
// uint8_t address[6] = {0xcc, 0xdb, 0xa7, 0x3e, 0x01, 0x28};
//uint8_t address[6] = {0xcc, 0xdb, 0xa7, 0x3f, 0x53, 0x60};
uint8_t address[6] = {0xcc, 0xdb, 0xa7, 0x3e, 0xb7, 0xf8};

typedef struct test_struct
{
  char a[80];
} test_struct;

test_struct myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  // Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.print(macStr);
  // Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void recvWithEndMarker()
{
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (SerialPort.available() > 0 && newData == false)
  {
    rc = SerialPort.read();

    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars)
      {
        ndx = numChars - 1;
      }
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}
String padLeft(String str, char padChar, int length)
{
  while (str.length() < length)
  {
    str = padChar + str;
  }

  return str;
}

void showNewData()
{
  if (newData == true)
  {
    Serial.print("This just in ... ");
    Serial.println(receivedChars);
    String trimmed = String(receivedChars);
    String trimmedCopy = String(receivedChars);

    trimmed.trim();
    // Serial.println("Setting: " + String(digitalRead(MODEMT1_PIN)) + String(digitalRead(MODEMT2_PIN)) + String(digitalRead(MODEAND_PIN)));
    if (trimmed.indexOf("C308720092") != -1)
    {
      newData = false;
      return;
    }

    int len = trimmed.length();

    if (len == 16 && (trimmed.startsWith("S S") || trimmed.startsWith("S D")))
    {
      // MODEMT1 pattern
      if (trimmed.startsWith("S S"))
      {
        trimmed.replace("S S", " * ");
      }
      else
      {
        trimmed.replace("S D", "   ");
      }
      int stable = 1;
      if (trimmedCopy.startsWith("S S"))
      {
        trimmedCopy.replace("S S", "");
        stable = 1;
      }
      if (trimmedCopy.startsWith("S D"))
      {
        trimmedCopy.replace("S D", "");
        stable = 0;
      }
      if (digitalRead(BUTTON_PIN) == HIGH && digitalRead(MODEAND_PIN) == HIGH)
      {
        trimmedCopy.replace("g", "");
        trimmedCopy.trim();
        float tlsFloat = trimmedCopy.toFloat();
        tlsFloat = tlsFloat / 37.7994;
        String tlsString = String(tlsFloat, 3);
        tlsString = tlsString + " t";
        tlsString = padLeft(tlsString, ' ', 16);
        if (stable)
        {
          tlsString[1] = '*';
          Serial.println(tlsString);
        }
        trimmed = tlsString;
      }
    }
    else if (len == 14 && (trimmed.startsWith("SD") || trimmed.startsWith("S")))
    {
      // MODEMT2 pattern
      if (trimmed.startsWith("SD"))
      {
        trimmed.replace("SD", "    ");
      }
      else
      {
        trimmed.replace("S", " * ");
      }
      int stable = 1;
      if (trimmedCopy.startsWith("S S"))
      {
        trimmedCopy.replace("S S", "");
        stable = 1;
      }
      if (trimmedCopy.startsWith("S D"))
      {
        trimmedCopy.replace("S D", "");
        stable = 0;
      }
      trimmedCopy.replace("g", "");
      trimmedCopy.trim();
      float tlsFloat = trimmedCopy.toFloat();
      tlsFloat = tlsFloat / 37.7994;
      String tlsString = String(tlsFloat, 3);
      tlsString = tlsString + " t";
      tlsString = padLeft(tlsString, ' ', 16);
      if (stable)
      {
        tlsString[1] = '*';
        Serial.println(tlsString);
      }
      trimmed = tlsString;
    }
    else if (len == 15 && (trimmed.startsWith("ST,+") || trimmed.startsWith("US,+") ||
                           trimmed.startsWith("ST,-") || trimmed.startsWith("US,-")))
    {
      // MODEAND pattern

      bool isStable = trimmed.startsWith("ST,");
      bool isNegative = trimmed.charAt(3) == '-';

      int prefixLen = isNegative ? 6 : 5; // ST,- vs ST,+

      // Count zeros
      int count = 0;
      while (trimmed.charAt(prefixLen + count) == '0')
      {
        count++;
      }

      // Extract the number part after leading zeros
      String numberPart = trimmed.substring(prefixLen + count);
      numberPart.replace("  g", " g");
      if (numberPart.startsWith("."))
      {
        numberPart = "0" + numberPart;
      }
      // Build prefix
      String prefix = isStable ? " *" : "  ";

      // Calculate how many spaces go between prefix and number
      int spaceCount = 16 - prefix.length() - numberPart.length();
      for (int i = 0; i < spaceCount; i++)
      {
        prefix += ' ';
      }

      // Final result
      trimmed = prefix + numberPart;

      trimmedCopy = trimmed;
      Serial.println(trimmedCopy);
      if (digitalRead(BUTTON_PIN) == HIGH)
      {
        int stable = 1;
        if (trimmedCopy.startsWith(" *"))
        {
          stable = 1;
          trimmedCopy.replace(" *", "");
        }
        else
        {
          stable = 0;
        }

        trimmedCopy.replace("g", "");
        trimmedCopy.trim();
        float tlsFloat = trimmedCopy.toFloat();
        tlsFloat = tlsFloat / 37.7994;
        String tlsString = String(tlsFloat, 3);
        tlsString = tlsString + " t";
        tlsString = padLeft(tlsString, ' ', 16);
        if (stable)
        {
          tlsString[1] = '*';
          Serial.println(tlsString);
        }
        trimmed = tlsString;
      }
    }
    else
    {
      newData = false;
      return;
    }

    const char *data = trimmed.c_str();
    // uint8_t *data = (uint8_t *)trimmed.c_str();
    strcpy(myData.a, data);
    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(address, (uint8_t *)&myData, sizeof(test_struct));
    // esp_err_t result_two = esp_now_send(addresstwo, (uint8_t *)&myData, sizeof(test_struct));

    newData = false;
  }
}

void setup()
{
  Serial.begin(9600);
  SerialPort.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODEMT1_PIN, INPUT_PULLUP);
  pinMode(MODEMT2_PIN, INPUT_PULLUP);
  pinMode(MODEAND_PIN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // register first peer
  memcpy(peerInfo.peer_addr, address, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  // register second peer
  // memcpy(peerInfo.peer_addr, addresstwo, 6);
  // if (esp_now_add_peer(&peerInfo) != ESP_OK)
  //{
  //  Serial.println("Failed to add peer");
  //  return;
  //}
}

void loop()
{
  recvWithEndMarker();
  showNewData();
}