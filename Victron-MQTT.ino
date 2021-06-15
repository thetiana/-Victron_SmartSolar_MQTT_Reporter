//this code is for ESP32, its take UART telemetry from Victron Solar chargers and relay the data to MQTT server.
//this code is based on project: http://www.svpartyoffive.com/2018/02/28/victron-monitors-technical/
//Be advide Victron UART port use 5V logic levels, ESP32 is designed for 3.3V logic levels
//use logic level converter to avoid damage your ESP32!!!
// define UART2 at pins 16 and 17
/*
PID 0xA043      -- Product ID for BlueSolar MPPT 100/15
FW  119     -- Firmware version of controller, v1.19
SER#  HQXXXXXXXXX   -- Serial number
V 13790     -- Battery voltage, mV
I -10     -- Battery current, mA
VPV 15950     -- Panel voltage, mV
PPV 0     -- Panel power, W
CS  5     -- Charge state, 0 to 9
ERR 0     -- Error code, 0 to 119
LOAD  ON      -- Load output state, ON/OFF
IL  0     -- Load current, mA
H19 0       -- Yield total, kWh
H20 0     -- Yield today, kWh
H21 397     -- Maximum power today, W
H22 0       -- Yield yesterday, kWh
H23 0     -- Maximum power yesterday, W
HSDS  0     -- Day sequence number, 0 to 365
Checksum  l:A0002000148   -- Message checksum

MQTT topics
victron/mode      reports the charger mode 0=OFF, 1=LOW_Power, 2=fault, 3=BULK, 4=Absorption, 5=FLOAT, 6=Inverter
victron/solar/w   reports the power of the solar panels
victron/solar/v   reports the voltage of the solar panels
victron/batery/c  reports the charging current
victron/batery/v  reports the battery voltage

TODO
1. to make JSON format of the reported data to MQTT
2. to add H19 H20 H21 H22 H23 
 */
#include <WiFi.h>
#include <PubSubClient.h>
#define RXD2 16
#define TXD2 17
#define DEBUG

//Cnange this section
const char* ssid = "SSID";
const char* password = "yourpass";
const char* mqtt_server = "mqtt_server_ip";

WiFiClient espClient11;
PubSubClient client(espClient11);
long lastMsg = 0;
char msg[50];
int value = 0;

int CS1;
float V2, I2, VPV2;
const byte numChars = 30;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;
const char * labelsOfInterest[] = {"V", "I", "VPV", "PPV", "CS"};
const unsigned int maxLabelsOfInterest = sizeof(labelsOfInterest) / sizeof(*labelsOfInterest);
boolean sleep1 = false;


void setup() {
  Serial.begin(19200);
  delay(500);
  Serial.println("initial....");
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(1000);
  }

  void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
   #endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.print(".");
    #endif
  }

  #ifdef DEBUG
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
   #endif
}

void reconnect() {
  // Loop until we're reconnected
 // while (!client.connected()) {
    if (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
    #endif
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      #ifdef DEBUG
      Serial.println("connected");
      #endif
      // Subscribe
      client.subscribe("b_node1/sorce");
    } else {
      #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      #endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

  
void loop() {
    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  recvWithEndMarker();
  if (newData) { // we have received a ful line, deal with it
 parseNewData(); // this messes up receivedChars[]
newData = false;
  }
}
void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
    while (Serial2.available() > 0 && newData == false) {
    rc = Serial2.read();
    if (rc != '\r') { // we ignore that character
      if (rc != endMarker) {
        if (rc == '\t') rc = ' '; // change tabs into just a space
        receivedChars[ndx] = rc;
        ndx++; // we go to the next space in our buffer
        if (ndx >= numChars) { // and check bounds.
          ndx = numChars - 1; // that means we will loose the end of long strings
        }
      } else {
        receivedChars[ndx] = '\0'; // terminate the string
        ndx = 0; // ndx is static, so we prepare for next turn
        newData = true;
      }
    }
  }
}
void parseNewData()
{
  const char *delim  = " ";
  char * item;
  boolean labelFound;
  long int labelValue;
  int labelIndex;
item = strtok (receivedChars, delim);
labelFound = false;
  for (int i = 0; i < maxLabelsOfInterest; ++i) {
    if (!strcmp(labelsOfInterest[i], item)) {
     item = strtok(NULL, delim);
      if (item != NULL) {
        labelFound = true;
        labelIndex = i;
       labelValue = atol(item);
break; // ends the for loop as we found a label
      } // end if we had a value for the label
  } // end string compare
  } // end for
  if (labelFound) {
    switch (labelIndex) {
      case 0:
        V2 = labelValue / 1000.00f;
        Serial.print("Battery Volate = ");
        Serial.println(V2);
        char V2_String[8];
        dtostrf(V2, 1, 2, V2_String);
        client.publish("victron/batery/v", V2_String);
        break;
      case 1:
        I2 = labelValue / 1000.00f;
        if (I2 < 0.05) {
        Serial.println("Charger Sleeping ");
        char I2_String[8];
        dtostrf(I2, 1, 2, I2_String);
        client.publish("victron/batery/c", I2_String);
        sleep1=true;
        }
        else {
        Serial.print("Current Into Batts = ");
        Serial.println(I2);
        char I2_String[8];
        dtostrf(I2, 1, 2, I2_String);
        client.publish("victron/batery/c", I2_String);
        sleep1=false;
        }
        break;
      case 2:
        VPV2 = labelValue / 1000.0f;
        if (sleep1 == true) {
        Serial.println("Panels Sleeping ");
        char VPV2_String[8];
        dtostrf(VPV2, 1, 2, VPV2_String);
        client.publish("victron/solar/v", VPV2_String);
        }
        else {
        Serial.print("Panel Voltage = ");
        Serial.println(VPV2);
        char VPV2_String[8];
        dtostrf(VPV2, 1, 2, VPV2_String);
        client.publish("victron/solar/v", VPV2_String);
        }
        break;
      case 3:
        Serial.print("Panel Wattage = ");
        Serial.println(labelValue);
        char labelValue_String[8];
        dtostrf(labelValue, 1, 2, labelValue_String);
        client.publish("victron/solar/w", labelValue_String);
        break;
      case 4:
        CS1=labelValue;
                      switch (CS1) {
                        case 0:
                          Serial.println("Charger is OFF");
                          client.publish("victron/mode", "0");
                          CS1 = NULL;
                          break;
                        case 1:
                          Serial.println("Charger is in low power mode");
                          client.publish("victron/mode", "1");
                          CS1 = NULL;
                          break;
                        case 2:
                          Serial.println("Charger has a fault");
                          client.publish("victron/mode", "2");
                          CS1 = NULL;
                          break;
                        case 3:
                          Serial.println("Charger is in BULK mode");
                          client.publish("victron/mode", "3");
                          CS1 = NULL;
                          break;
                        case 4:
                          Serial.println("Charger is in ABSORB mode");
                          client.publish("victron/mode", "4");
                          CS1 = NULL;
                          break;
                        case 5:
                          Serial.println("Charger is in FLOAT mode");
                          client.publish("victron/mode", "5");
                          CS1 = NULL;
                          break;
                       case 9:
                          Serial.println("Charger is in INVERT mode");
                          client.publish("victron/mode", "9");
                          CS1 = NULL;
                          break;
                       }
        Serial.print("Charger State = ");
        Serial.println(labelValue);
        Serial.print("\n");
        delay(3000);
       break;
    }
   } else {
   }
}

void callback(char* topic, byte* message, unsigned int length) {
  #ifdef DEBUG
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  #endif
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    #ifdef DEBUG
    Serial.print((char)message[i]);
    #endif
    messageTemp += (char)message[i];
  }
  #ifdef DEBUG
  Serial.println();
  #endif

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
}
