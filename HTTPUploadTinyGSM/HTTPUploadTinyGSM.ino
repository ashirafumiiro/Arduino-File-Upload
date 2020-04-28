#define TINY_GSM_MODEM_SIM800
// Increase RX buffer if needed
//#define TINY_GSM_RX_BUFFER 512
#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
//#define DUMP_AT_COMMANDS  // Uncomment this if you want to see all AT commands
#define SerialMon Serial
#define SerialAT Serial2
#define SD_CS 53
#define GSM_PIN 22

const char apn[]  = "internet";
const char user[] = "";
const char pass[] = "";
const char server[] = "cedatshare.azurewebsites.net";
const char resource[] = "/upload.php";
const int  port = 80;
char fileName[] = "datafile.csv";

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, 1);
  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);
  // To skip it, call init() instead of restart()
  SerialMon.println(F("Initializing modem..."));
  modem.restart();
  String modemInfo = modem.getModemInfo();
  SerialMon.print(F("Modem: "));
  SerialMon.println(modemInfo);
}

void loop() {
  SerialMon.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");
  if (!SD.begin(53)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  File dataFile = SD.open("datafile.csv");
  // if the file is available
  if (dataFile) {/*if  available, continue*/}
  else{
    SerialMon.println("Error Opening file");
    return;
  }
  SerialMon.print(F("Connecting to "));
  SerialMon.print(server);
  if (!client.connect(server, port)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  char fileName[] = "datafile.txt";
  String fileType = "text/plain";
  String content = "--boundary1\r\n";
  content += "Content-Disposition: form-data; name=\"fileToUpload\"; filename="+String(fileName)+"\r\n";  // the fileToUpload is the form parameter
  content += "Content-Type: "+fileType+"\r\n\r\n";
  //after this, post the file data.
  String closingContent = "\r\n--boundary1--";
  
  // Make a HTTP GET request:
  client.print(F("POST /upload.php HTTP/1.1\r\n"));  // replace with your /path/to/uploat/script
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=boundary1\r\n");
  client.print("Content-Length: "+String(content.length()+dataFile.size()+closingContent.length())+"\r\n");
  client.print(F("Connection: close\r\n\r\n"));
  client.print(content);
  if (dataFile) {  // start sending file content 
    String data = "";
    while (dataFile.available()) {
      char c = dataFile.read();
      data += c;
      if(c == '\n'){
        client.print(data);
        data = "";
      }     
    }
    dataFile.close();
  }
  client.print(closingContent);
  // Read respnse
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 50000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      SerialMon.print(c);
      timeout = millis();
    }
  }
  SerialMon.println();
  // Shutdown
  client.stop();
  SerialMon.println(F("Server disconnected"));
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));
  // Do nothing forevermore
  while (true) {
    delay(1000);
  }
}
