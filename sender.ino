//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_PCF8574.h>
#include <WiFi.h>
#include <AES.h>
#include <WiFiUDP.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <math.h>

// LCD Initialization
LiquidCrystal_PCF8574 lcd(0x27);
void handleFileUpload();
void processUploadedFile();

// Wi-Fi Credentials
const char* ssid = "RedmiNote135G";
const char* password = "12345678";

IPAddress targetIP(192,168,137,147);
const uint16_t targetPort = 8080;

// AES Key and IV (Same for Sender & Receiver)
byte aesKey[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

byte iv[16]     = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                   0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

AES aes;
WiFiUDP udp;
WebServer server(80);

// Storage Arrays
const int MAX_OBJECTS = 50;
float X[MAX_OBJECTS], Y[MAX_OBJECTS], Z[MAX_OBJECTS];
float doppler[MAX_OBJECTS];
int objectCount = 0;
bool fileUploaded = false;

// Extracted Radar Computed Results
float minDist, maxDist, avgDist;
float nearestDist;
float lastVelocity;
float lastAngleDeg;
String angleDirection;

const char* htmlPage = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <title>ESP32 Radar CSV Upload</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    body {
      font-family: Arial, sans-serif;
      background: #0B1A30;
      color: #ffffff;
      text-align: center;
      margin: 0;
      padding: 0;
    }

    .header {
      background: #007cba;
      padding: 18px;
      font-size: 22px;
      font-weight: bold;
      color: white;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }

    .container {
      background: #132441;
      width: 90%;
      max-width: 480px;
      margin: 50px auto;
      padding: 25px;
      border-radius: 15px;
      border: 2px solid #00aaff;
      box-shadow: 0px 4px 10px rgba(0,0,0,0.4);
    }

    input[type="file"] {
      width: 85%;
      padding: 12px;
      border-radius: 8px;
      border: 2px dashed #00c3ff;
      background: white;
      color: black;
      font-size: 15px;
      cursor: pointer;
    }

    input[type="submit"] {
      width: 85%;
      padding: 14px;
      margin-top: 22px;
      background: #0099ff;
      border: none;
      border-radius: 8px;
      color: white;
      font-size: 18px;
      font-weight: bold;
      cursor: pointer;
      transition: 0.3s;
    }

    input[type="submit"]:hover {
      background: #006bb3;
    }

    #statusText {
      margin-top: 22px;
      font-size: 16px;
      color: #00ffea;
      display: none;
    }

    .loader {
      border: 4px solid rgba(255,255,255,0.2);
      border-top: 4px solid #00d4ff;
      width: 35px;
      height: 35px;
      border-radius: 50%;
      animation: spin 1.2s linear infinite;
      display: none;
      margin: 15px auto;
    }

    @keyframes spin {
      100% { transform: rotate(360deg); }
    }
  </style>
</head>

<body>

<div class="header">📡 ESP32 Radar Encryption System</div>

<div class="container">
  <h3>Upload CSV Radar File</h3>
  <p>Select the radar tracking CSV file and upload it for processing.</p>

  <form id="uploadForm" method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="dataFile" accept=".csv" required><br>
    <input type="submit" value="🚀 Upload & Process">
  </form>

  <div class="loader" id="loader"></div>
  <div id="statusText">Processing… DO NOT refresh.</div>
</div>

<script>
document.getElementById("uploadForm").addEventListener("submit", function() {
  document.getElementById("loader").style.display = "block";
  document.getElementById("statusText").style.display = "block";
});
</script>

</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  lcd.begin(20,4);
  lcd.setBacklight(255);
  lcd.clear();

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    lcd.print("SPIFFS Failed");
    return;
  }

  // WiFi
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected!");
  lcd.setCursor(0,1);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  lcd.setCursor(0,2);
  lcd.print("Web Server Ready");
  lcd.setCursor(0,3);
  lcd.print("Use Browser");

  // Web Server Routes
  server.on("/", HTTP_GET, [](){ server.send(200,"text/html",htmlPage); });

  server.on("/upload", HTTP_POST, [](){
    server.send(200,"text/html","<h3>File Uploaded Successfully!</h3>");
  }, handleFileUpload);

  server.begin();
  udp.begin(8080);
}


void handleFileUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("[UPLOAD] Start: " + upload.filename);
    if (SPIFFS.exists("/uploaded.csv")) SPIFFS.remove("/uploaded.csv");
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Writing chunks
    File file = SPIFFS.open("/uploaded.csv", FILE_APPEND);
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    } else {
      Serial.println("[UPLOAD] Failed to open file for writing!");
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    Serial.println("[UPLOAD] Finished. Size: " + String(upload.totalSize));

    fileUploaded = true;

    lcd.clear();
    lcd.print("File Uploaded!");
    lcd.setCursor(0, 1);
    lcd.print("Processing...");

    // Directly process here (no need to wait for loop)
    processUploadedFile();
  }
}


void processUploadedFile(){
  
  if(!SPIFFS.exists("/uploaded.csv")) return;

  File file = SPIFFS.open("/uploaded.csv");
  String line;
  int count = 0, row = 0;
  objectCount = 0;

  while(file.available() && objectCount < MAX_OBJECTS){
    
    line = file.readStringUntil('\n');
    row++;

    if(row <= 2 || line.length() == 0) continue;

    String v[20];
    int idx = 0, last = 0, f;

    while ((f = line.indexOf(',', last)) != -1 && idx < 20) {
        v[idx++] = line.substring(last, f);
        last = f + 1;
    }
    if (idx < 20) v[idx] = line.substring(last);

    // Extract only if X and Y are valid numeric values
    float tempX = v[9].toFloat();
    float tempY = v[10].toFloat();

    if(isnan(tempX) || isnan(tempY)) continue; // skip invalid rows

    // Store only if enough columns were parsed AND values are valid
if(idx > 11){
    float tempX = v[9].toFloat();
    float tempY = v[10].toFloat();

    // ignore lines where X and Y are zero/invalid
    if(!(tempX == 0 && tempY == 0)){
        doppler[objectCount] = v[6].toFloat();
        X[objectCount] = tempX;
        Y[objectCount] = tempY;
        Z[objectCount] = v[11].toFloat();
        objectCount++;
    }
}
}

  file.close();

  lcd.clear();
  lcd.print("Objects Found:");
  lcd.setCursor(0,1);
  lcd.print(objectCount);
  lcd.print(" objects");

  delay(2000);

  if(objectCount >= 4) processRadarData();
}


void processRadarData(){

  float sum = 0;
  minDist = 9999;
  maxDist = 0;
  int nearestIndex = 0;

  for(int i=0; i<objectCount; i++){
    float d = calculateDistance(0,0,0,X[i],Y[i],Z[i]);
    sum += d;

    // FIX: compare rounded values to avoid tiny floating errors
    if(round(d*1000) < round(minDist*1000)) {
        minDist = d;
        nearestIndex = i;
    }

    if(d > maxDist) maxDist = d;
}

  avgDist = sum / objectCount;
  nearestDist = minDist;

  lastAngleDeg = atan2(Y[nearestIndex],X[nearestIndex]) * (180.0/PI);

  if(lastAngleDeg > 20) angleDirection="RIGHT";
  else if(lastAngleDeg < -20) angleDirection="LEFT";
  else angleDirection="CENTER";

  lastVelocity = doppler[nearestIndex] * 0.1;


  // -------- LCD Summary Screen --------
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("MinDist: "); lcd.print(minDist,2); lcd.print(" m");
  lcd.setCursor(0,1); lcd.print("MaxDist: "); lcd.print(maxDist,2); lcd.print(" m");
  lcd.setCursor(0,2); lcd.print("AvgDist: "); lcd.print(avgDist,2); lcd.print(" m");
  lcd.setCursor(0,3); lcd.print("Objects: "); lcd.print(objectCount);
  delay(3500);

  // -------- Nearest Object Screen --------
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Nearest Object");
  lcd.setCursor(0,1); lcd.print("X="); lcd.print(X[nearestIndex],2);
                     lcd.print(" Y="); lcd.print(Y[nearestIndex],2);
  lcd.setCursor(0,2); lcd.print("Direction: "); lcd.print(angleDirection);
  lcd.setCursor(0,3); lcd.print("Velocity: "); lcd.print(lastVelocity,2); lcd.print(" m/s");
  delay(3500);


  // -------- Sending ALL encrypted values --------

  sendEncrypted(minDist,"MinDist");
  sendEncrypted(maxDist,"MaxDist");
  sendEncrypted(avgDist,"AvgDist");
  sendEncrypted(nearestDist,"NearDist");
  sendEncrypted(lastVelocity,"Velocity");

  float dirCode = (angleDirection=="LEFT")?-1.0 : (angleDirection=="RIGHT")?1.0 : 0.0;
  sendEncrypted(dirCode,"DirCode");


  // -------- Reset LCD --------
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Processing Done!");
  lcd.setCursor(0,1); lcd.print("Ready for new");
  lcd.setCursor(0,2); lcd.print("file upload");
  lcd.setCursor(0,3); lcd.print("IP: "); lcd.print(WiFi.localIP());

  fileUploaded=false;
}


void sendEncrypted(float value, String label){

  byte encrypted[16];
  aes.do_aes_encrypt((byte*)&value, sizeof(value), encrypted, aesKey,128,iv);

  lcd.clear();
  lcd.print("Encrypting "); lcd.print(label);
  lcd.setCursor(0,1);

  for(int i=0;i<8;i++){
    if(encrypted[i] < 0x10) lcd.print("0");
    lcd.print(encrypted[i],HEX);
    lcd.print(" ");
  }

  delay(1500);

  lcd.clear();
  lcd.print("Sending "); lcd.print(label); lcd.print("...");

  udp.beginPacket(targetIP, targetPort);
  udp.write(encrypted, sizeof(encrypted));
  bool sent = udp.endPacket();

  lcd.setCursor(0,1);
  lcd.print(sent ? "Send SUCCESS!" : "Send FAILED!");
  delay(1500);
}


float calculateDistance(float x1,float y1,float z1,float x2,float y2,float z2){
  return sqrt(pow(x2-x1,2)+pow(y2-y1,2)+pow(z2-z1,2));
}


void loop() {
  server.handleClient();
  if (fileUploaded) {
    processUploadedFile();
    fileUploaded = false;
  }
}
