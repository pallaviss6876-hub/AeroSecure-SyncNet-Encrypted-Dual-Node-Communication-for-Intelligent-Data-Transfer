#include <LiquidCrystal_PCF8574.h>
#include <WiFi.h>
#include <AES.h>
#include <WiFiUDP.h>

// LCD
LiquidCrystal_PCF8574 lcd(0x27);

// WiFi
const char* ssid = "RedmiNote135G";
const char* password = "12345678";

// AES Key/IV
byte aesKey[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
byte iv[16]     = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};

AES aes;
WiFiUDP udp;

const uint16_t listenPort = 8080;

// Must match sender order:
String labels[6] = {"MinDist","MaxDist","AvgDist","NearDist","Velocity","DirCode"};
float receivedValues[6];
int receivedCount = 0;

void setup() {
  Serial.begin(115200);
  lcd.begin(20,4);
  lcd.setBacklight(255);

  lcd.clear();
  lcd.print("Connecting WiFi...");

  IPAddress ip(192,168,137,147);
  WiFi.config(ip, IPAddress(192,168,137,1), IPAddress(255,255,255,0));
  WiFi.begin(ssid,password);

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected!");
  lcd.setCursor(0,1); lcd.print("IP: "); lcd.print(WiFi.localIP());
  lcd.setCursor(0,2); lcd.print("Receiver Mode");

  delay(2000);

  lcd.clear();
  lcd.print("UDP Listening...");
  lcd.setCursor(0,1);
  lcd.print("Port "); lcd.print(listenPort);

  udp.begin(listenPort);
}

void loop() {

  int packetSize = udp.parsePacket();
  if(packetSize){

    byte encryptedData[16];
    udp.read(encryptedData, sizeof(encryptedData));

    // 1️⃣ Show encrypted bytes
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Encrypted Data:");
    lcd.setCursor(0,1);

    for(int i=0;i<8;i++){
      if(encryptedData[i] < 0x10) lcd.print("0");
      lcd.print(encryptedData[i],HEX);
      lcd.print(" ");
    }
    delay(2500);

    // 2️⃣ Decrypt silently (NO LCD message)
    byte decryptedData[16];
    aes.do_aes_decrypt(encryptedData,sizeof(encryptedData),decryptedData,aesKey,128,iv);

    float value;
    memcpy(&value, decryptedData, sizeof(value));
    receivedValues[receivedCount] = value;

    // 3️⃣ Show decrypted result instantly
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Decrypted Result");

    lcd.setCursor(0,1);
    lcd.print(labels[receivedCount]);
    lcd.print(": ");

    if(labels[receivedCount] == "DirCode") {
      if(value == -1) lcd.print("LEFT");
      else if(value == 1) lcd.print("RIGHT");
      else lcd.print("CENTER");
    } else {
      lcd.print(value,2);
    }

    delay(3000);

    receivedCount++;

    if(receivedCount == 6){
      showSummary();
      receivedCount = 0;
    }

    lcd.clear();
    lcd.print("Waiting for data");
  }
}

void showSummary(){

  lcd.clear();
  lcd.print("Radar Summary:");

  lcd.setCursor(0,1);
  lcd.print("Min:");
  lcd.print(receivedValues[0],2);
  lcd.print(" Max:");
  lcd.print(receivedValues[1],2);

  lcd.setCursor(0,2);
  lcd.print("Avg:");
  lcd.print(receivedValues[2],2);
  lcd.print(" Near:");
  lcd.print(receivedValues[3],2);

  lcd.setCursor(0,3);

  String direction = (receivedValues[5] == -1) ? "LEFT" :
                     (receivedValues[5] == 1)  ? "RIGHT" : "CENTER";

  lcd.print(direction);
  lcd.print(" Vel:");
  lcd.print(receivedValues[4],2);
  lcd.print("m/s");

  delay(6000);
}
