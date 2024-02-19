#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>
#include<Wire.h>

const char *GScriptId = "AKfycbzyy3wgcWUgU_GIqNc2dwKn_BPiTFAf12vXYhIDDvwTNfc4N_0dL_sl6x4qFHJ-i2MukQ";
const int led1 = D2;
const int led2 = D3;
const char* ssid     = "Manilka";
const char* password = "Manilka@99";

String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

const char* host        = "script.google.com";
const int   httpsPort   = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

String student_id;

int blocks[] = {4,5,6,8,9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))

#define RST_PIN  0 
#define SS_PIN   2 
#define BUZZER   5  

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;

//---------------------------
/* keep watch out for Sector Trailer Blocks */
int blockNum = 2; 
 
/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
//------------------------------------------------------------

/* Main Setup Function */
void setup() {
  
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');
  
  SPI.begin();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.println();
  
 
  pinMode(BUZZER, OUTPUT);
  SPI.begin();
  
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
 
  // Here HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  
  Serial.print("Connecting to ");
  Serial.println(host);

  
  // Try to connect for a maximum of 5 times
  bool flag = false;
  for(int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    
    if (retval == 1){
      flag = true;
      String msg = "Connected. OK";
      Serial.println(msg);
      delay(2000);
      digitalWrite(led1, HIGH);
      delay(1000);
      digitalWrite(led1, LOW);
      break;
    }
    
    else
      Serial.println("Connection failed. Retrying...");
      digitalWrite(led2, HIGH);
      delay(1000);
      digitalWrite(led2, LOW);
   
  }
  //----------------------------------------------------------
  if (!flag){
    
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(5000);
    digitalWrite(led2, HIGH);
    delay(2000);
    digitalWrite(led2, LOW);
    return;
    
  }
  
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  
}

/* Main loop Function */

void loop() {
 
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected())
      {client->connect(host, httpsPort);}
  }
  else{Serial.println("Error creating client object!");}
      digitalWrite(led2, HIGH);
      delay(750);
      digitalWrite(led2, LOW);
      
  Serial.println("Scan your Tag");


  
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));  
  //----------------------------------------------------------------
  String values = "", data;

  
  /*Payload - method*/
  
  for (byte i = 0; i < total_blocks; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    //*************************************************
    if(i == 0){
      data = String((char*)readBlockData);
      data.trim();
      student_id = data;
      values = "\"" + data + ",";
    }
    //*************************************************
    else if(i == total_blocks-1){
      data = String((char*)readBlockData);
      data.trim();
      values += data + "\"";
    }
    //*************************************************
    else{
      data = String((char*)readBlockData);
      data.trim();
      values += data + ",";
    }
  }
  //------------------------------
  payload = payload_base + values;
  
  /*Buzzer sound for RFID Read*/
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(200);
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(3000);  
  
  
  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if(client->POST(url, host, payload)){ 
    // do stuff here if publish was successful
    Serial.println("Student ID: "+student_id);
    Serial.println("Present");
    digitalWrite(led1, HIGH);
    delay(1000);
    digitalWrite(led1, LOW);
  }
  
  else{
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
    Serial.println("Failed.");
    Serial.println("Try Again");
    //-------------------------------
    digitalWrite(led2, HIGH);
    delay(3000);
    digitalWrite(led2, LOW);
  }
  
  // Delay to required before publishing again    
  delay(1000);
}


/* ReadDataFromBlock() function */

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 

  /* Prepare the ksy for authentication - All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  
  //--------------------------------------------------
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  
  else {
    Serial.println("Authentication success");
  }
  
  /* Reading data from the Block */
  
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(led2, HIGH);
    delay(2000);
    digitalWrite(led2, LOW);
    return;
  }
  
  else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
}
