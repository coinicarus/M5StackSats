#include "BLITZSplash.c"
#include <M5Stack.h> 
#include <string.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <math.h>

#define KEYBOARD_I2C_ADDR     0X08
#define KEYBOARD_INT          5

//Wifi details
char wifiSSID[] = "WIFI_NAME";   //Case Sensitive, also no special characters
char wifiPASS[] = "WIFI_PASS";   

//BLITZ DETAILS
const char*  server = "Server Name Or IP"; 
const int httpsPort = 443;
const int lndport = 8080;
String pubkey;
String totcapacity;

String readmacaroon = "LND READ MACAROON";
String invoicemacaroon = "LND INVOICE MACAROON";
const char* test_root_ca =   //SSL must be in this format, SSL for the node can be exported from yournode.com:8080 in firefox
    "-----BEGIN CERTIFICATE-----\n" \
    "MIICITCCAcegAwIBAgIRAPt2SSkLWPX61GtcPjHPvh4wCgYIKoZIzj0EAwIwNjEf\n" \
    "MB0GA1UEChMWbG5kIGF1dG9nZW5lcmF0ZWQgY2VydDETMBEGA1UEAxMKUmFzcGlC\n" \
    "bGl0ejAeFw0xOTA1MjYxMjEwMjdaFw0yMDA3MjAxMjEwMjdaMDYxHzAdBgNVBAoT\n" \
    "FmxuZCBhdXRvZ2VuZXJhdGVkIGNlcnQxEzARBgNVBAMTClJhc3BpQmxpdHowWTAT\n" \
    "BgcqhkjOPQIBBggqhkjOPQMBBwNCAAQbl+SzxDpLnbGbrKEjzaZOkHh7qWwLzvTB\n" \
    "5x4SBKB5IEyKrP42F/yB8UURiLtnVn61K4B+quE4uk83QtpyiEuuo4G1MIGyMA4G\n" \
    "A1UdDwEB/wQEAwICpDAPBgNVHRMBAf8EBTADAQH/MIGOBgNVHREEgYYwgYOCClJh\n" \
    "c3BpQmxpdHqCCWxvY2FsaG9zdIIEdW5peIIKdW5peHBhY2tldIcEfwAAAYcQAAAA\n" \
    "AAAAAAAAAAAAAAAAAYcECgAA14cQJgEBioMB5ucAAAAAAADRiYcQJgEBioMB5ueZ\n" \
    "p2i3PfXHa4cQ/oAAAAAAAADJyTrZT3diWYcEAAAAADAKBggqhkjOPQQDAgNIADBF\n" \
    "AiB0yC6e2quMe6fIfMbDxdrWVqDy0ZWcLN3t9e4Jan8orwIhAK+hJVHctKGL7uB0\n" \
    "2Xa83Qs+KTZeA0uLUOssso/Uf737\n" \ 
     "-----END CERTIFICATE-----\n";

String choice = "";

String on_currency = "BTCUSD"; //currency can be changed here ie BTCUSD BTCGBP etc
String on_sub_currency = on_currency.substring(3);
String memo = "Memo "; //memo suffix, followed by the price then a random number

  String key_val;
  String cntr = "0";
  String inputs = "";
  int keysdec;
  int keyssdec;
  float temp;  
  String fiat;
  float satoshis;
  String nosats = "";
  float conversion;
  String postid = "";
  String data_id = "";
  String data_lightning_invoice_payreq = "";
  String data_status = "";
  bool settle = false;
  String payreq = "";
  String hash = "";

void page_input()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.println("Amount then C");
  M5.Lcd.println("");
  M5.Lcd.println("USD: ");
  M5.Lcd.println("");
  M5.Lcd.println("SATS: ");
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(50, 200);
  M5.Lcd.println("TO RESET PRESS A");
}

void page_processing()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
 
}

void get_keypad(){

   if(digitalRead(KEYBOARD_INT) == LOW) {
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);  // request 1 byte from keyboard
    while (Wire.available()) { 
       uint8_t key = Wire.read();                  // receive a byte as character
       key_val = key;

       if(key != 0) {
        if(key >= 0x20 && key < 0x7F) { // ASCII String
          if (isdigit((char)key)){
          key_val = ((char)key);
          }
          else {
          key_val = "";
        } 
        }
      }
    }
  }
}

void setup() {
  M5.begin();
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)BLITZSplash_map);
  Wire.begin();


  //connect to local wifi            
  WiFi.begin(wifiSSID, wifiPASS);   
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(i >= 5){
     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(55, 80);
     M5.Lcd.setTextSize(2);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("WIFI NOT CONNECTED");
    }
    delay(1000);
    i++;
  }
  

    
  pinMode(KEYBOARD_INT, INPUT_PULLUP);
  pinMode(26, OUTPUT);

  
  on_rates();
  nodecheck();
}

void loop() {

 page_input();

  cntr = "1";
  while (cntr == "1"){
   M5.update();
   get_keypad(); 

    if (M5.BtnC.wasReleased()) {
   // if (key_val == "="){

     page_processing();
     
     reqinvoice(nosats);

     gethash(payreq);

     page_qrdisplay(payreq);

     int counta = 0;
     int tempi = 0;
     settle = false;

     while (tempi == 0){
      
      

     checkpayment(hash);

     if (settle == false){
        counta ++;
        Serial.print(counta);
        if (counta == 100){
         tempi = 1;
        }
 
     }
      
       else{
        tempi = 1;

     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.println("Place hand above sensor");
     digitalWrite(26, HIGH);
     delay(2000);
     digitalWrite(26, LOW);
     delay(1000);
     counta = 1000;
     M5.Lcd.setCursor(60, 80);
     M5.Lcd.setTextSize(4);
     M5.Lcd.setTextColor(TFT_GREEN);
     M5.Lcd.println("COMPLETE");

     delay(1000);
     nodecheck();;
     cntr = "2";
 
      }
      
     int bee = 0;
     while ((bee < 120) && (tempi==0)){

      M5.update();

     if (M5.BtnA.wasReleased()) {

        tempi = -1;
     
     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(50, 80);
     M5.Lcd.setTextSize(4);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("CANCELLED");

     delay(1000);
     nodecheck();;
     cntr = "2";
     
      }
      
      delay(10);
      bee++;
     key_val = "";
     inputs = "";
     
     }
     
      
     }
     
     key_val = "";
     inputs = "";
    }

     else if (M5.BtnA.wasReleased()) {

     M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(0, 0);
     M5.Lcd.setTextColor(TFT_WHITE);
      page_input();
      key_val = "";
      inputs = "";  
      nosats = "";
    }
    
    
      Serial.print(key_val);
      inputs += key_val;
      
      temp = inputs.toInt();
      
      temp = temp / 100;

      fiat = temp;
      
      satoshis = temp/conversion;

      int intsats = (int) round(satoshis*100000000.0);

      nosats = String(intsats);
        M5.Lcd.setTextSize(3);
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.setCursor(70, 88);
        M5.Lcd.println(fiat);
        M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        M5.Lcd.setCursor(87, 136);
        M5.Lcd.println(nosats);


      delay(100);
      key_val = "";
        
    
    
  }
}



//OPENNODE REQUESTS

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", httpsPort)) {
    return;
  }

  String url = "/v1/rates";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.opennode.co\r\n" +
               "User-Agent: ESP32\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
  Serial.println("poo");

      break;
    }
  }
  String line = client.readStringUntil('\n');
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);
    conversion = doc["data"][on_currency][on_currency.substring(3)]; 

}

void nodecheck(){
  bool checker = false;
  while(!checker){
  WiFiClientSecure client;
   //client.setCACert(test_root_ca);
  if (!client.connect(server, lndport)){

    M5.Lcd.fillScreen(BLACK);
     M5.Lcd.setCursor(20, 80);
     M5.Lcd.setTextSize(3);
     M5.Lcd.setTextColor(TFT_RED);
     M5.Lcd.println("NO NODE DETECTED");
     delay(1000);
  }
  else{
    checker = true;
  }
  }
  
}

void reqinvoice(String value){

   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
      return;   
  }

    
   String topost = "{\"value\": \""+ value +"\", \"memo\": \""+ memo + String(random(1,1000)) +"\", \"expiry\": \"1000\","+ +"\"private\": true}";
  
       client.print(String("POST ")+ "https://" + server + ":" + String(lndport) + "/v1/invoices HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "User-Agent: ESP322\r\n" +
                 "Grpc-Metadata-macaroon:" + invoicemacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close\r\n" +
                 "Content-Length: " + topost.length() + "\r\n" +
                 "\r\n" + 
                 topost + "\n");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    
    String content = client.readStringUntil('\n');
  

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(3) + 320;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* r_hash = doc["r_hash"];
    hash = r_hash;
    const char* payment_request = doc["payment_request"]; 
    payreq = payment_request;
 
}



void gethash(String xxx){
  
   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }
   

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/payreq/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
       
        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();

    const size_t capacity = JSON_OBJECT_SIZE(7) + 270;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    const char* payment_hash = doc["payment_hash"]; 
    hash = payment_hash;
}


void checkpayment(String xxx){
  
   WiFiClientSecure client;

  //client.setCACert(test_root_ca);

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server, lndport)){
       return;
  }

       client.println(String("GET ") + "https://" + server +":"+ String(lndport) + "/v1/invoice/"+ xxx +" HTTP/1.1\r\n" +
                 "Host: "  + server +":"+ String(lndport) +"\r\n" +
                 "Grpc-Metadata-macaroon:" + readmacaroon + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Connection: close");
                 
       client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {

        break;
      }
    }
    

    String content = client.readStringUntil('\n');

    client.stop();
    
    const size_t capacity = JSON_OBJECT_SIZE(9) + 460;
    DynamicJsonDocument doc(capacity);

    deserializeJson(doc, content);

    settle = doc["settled"]; 
    
  
}



void page_qrdisplay(String xxx)
{  

  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payreq,45,0,240,14);
  delay(100);

}
