#include "MFRC522.h"
#include <Adafruit_NeoPixel.h>

//RFID-RC522
#define RST_PIN 5 // RST-PIN for RC522 - RFID - SPI - Modul GPIO5 
#define SS_PIN  4 // SDA-PIN for RC522 - RFID - SPI - Modul GPIO4

//LED Ring
#define PIXEL_PIN 0
#define PIXEL_COUNT 7
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "XXX";
const char* password = "XXX";
const char* mqtt_server = "XXX";
const char* mqtt_topic = "XXX";


//rfid
int iState = 0;
String uidstr="";
String uidstrOld="";
String messageMacUid="";

//Wifi
WiFiClient espClient;
uint8_t MAC_array[6];
char MAC_char[18];

//MQTT
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//Tests
String serialString;

void setup() {
  Serial.begin(115200);
  
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'
  
  Serial.println("Setup Wifi start");
  setup_wifi();
  WiFi.macAddress(MAC_array);
  for (int i = 0; i < sizeof(MAC_array); ++i){
    sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  }

  Serial.println(MAC_char);
    
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //rfid
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522
}

void loop() {

  if (!client.connected()) {
    Serial.println("reconnect");
    reconnect();
  }
  if(WiFi.status() == 6 || WiFi.status() == 4){
    setup_wifi();
  }
  client.loop();

/*
  if(Serial.available() > 0){
    // read the incoming byte:
    serialString = Serial.readString();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(serialString);
    serialString.toCharArray(msg,50);
    client.publish("outTopic", msg, 1);
//    delay(1000);
  }
*/

  switch(iState){
      case 0: if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                theaterChase(strip.Color(127,   0,   0), 50); // Red
                iState++;
              }else{
                colorWipe(strip.Color(0, 255, 0), 50);  // Green
                delay(50);
              }
              break;
      case 1: uidstr="";
              for (byte i = 0; i < mfrc522.uid.size; i++){ 
                Serial.println(mfrc522.uid.uidByte[i],HEX);
                uidstr = uidstr + String(mfrc522.uid.uidByte[i],HEX);                 
              }              
              iState++;
              /*
              if(uidstr != uidstrOld){
                uidstrOld = uidstr;
                iState++;
              }else{
                iState = 0;
              }
              */         
              break;
      case 2: Serial.println("Eingelesene Kartennummer: "+uidstr);
              boolean succesfulPublished = false;
              messageMacUid = "{MAC:"+String(MAC_char)+"}{UID:"+uidstr+"}";
              messageMacUid.toCharArray(msg, 50);
              succesfulPublished = client.publish("rfid/reader/1", msg, 0);
              if(succesfulPublished){
                Serial.println("Published and waiting 5sek.");
                theaterChase(strip.Color(127, 127, 127), 50); // White
                delay(5000);
                iState=0;   
                break;
              } else {
                Serial.println("retry sending");
                setup_wifi();
                break;
              }
}
}
void setup_wifi() {
  colorWipe(strip.Color(255, 0, 0), 50);  // Red
  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(WiFi.status());
    Serial.print(".");
    delay(500);
  }

  colorWipe(strip.Color(255, 255, 51), 50);  // Yellow
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    colorWipe(strip.Color(200, 200, 40), 50);  // Yellow
    Serial.print(WiFi.status());
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client","llf", "ESB.LLF","rfid/will/1",1,false,"off")) {
      Serial.println("connected");
      colorWipe(strip.Color(0, 255, 0), 50);  // Green
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      //delay(1000);
    }
  }
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}
//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

