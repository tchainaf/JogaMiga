//Desenvolvido por: Thainá França

#define USING_AXTLS

#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecureAxTLS.h>
using namespace axTLS;

#define LED_RED 2   //D4
#define LED_GREEN 5 //D1
#define SS_PIN  4   //D2
#define RST_PIN 0   //D3
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

const char* host = "script.google.com";
const char* host2 = "script.googleusercontent.com";
const int httpsPort = 443;
const char* fingerprint = "46 B2 C3 44 9C 59 09 8B 01 B6 F8 BD 4C FB 00 74 91 2F EF F6";
const char* gScriptID = "AKfycbyD7Nl4i22pmP-uMYxF9Wt1M_aO_ST6yyyjPCeoZJauJkeuKEh1";

void setup()
{
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; //Prepara chave RFID - padrao de fabrica = FFFFFFFFFFFFh
  connectToWiFi();
}

void loop()
{
  readRFID();
  delay(10);
}

//********************************************* FUNÇÕES *********************************************

void connectToWiFi()
{
  WiFiManager wifiManager;

  if (!wifiManager.startConfigPortal("AP JogaMiga"))
  {
    Serial.println("Falha na conexão, tempo máximo atingido.");
    delay(1000);
    ESP.reset();
    delay(2000);
  }

  WiFi.mode(WIFI_STA);
  Serial.println("");
  Serial.println("Conectado!");
  sinalizaSucesso();
  Serial.println("IP: " + WiFi.localIP());
  delay(100);
}

void readRFID()
{
  Serial.println("Aproxime o cartão");

  while (!mfrc522.PICC_IsNewCardPresent())
  {
    delay(100); //Aguarda cartão
  }
  if (!mfrc522.PICC_ReadCardSerial())
  {
    Serial.println("Não foi possível ler o cartão!");
    return;
  }

  //Armazena UID
  String id = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    id.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : "%20")); //%20 = espaço
    id.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  id.trim();
  id.toUpperCase();
  Serial.println("UID da tag: " + id);
  sinalizaSucesso();
  sendToExcel(id);
}

void sendToExcel(String id)
{
  // Use WiFiClientSecure class to create TLS connection
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"
  WiFiClientSecure wifiClient;
#pragma GCC diagnostic pop

  if (!wifiClient.connect(host, httpsPort))
  {
    Serial.println("Falha na conexão ao host: " + String(host));
    showSheetsResponse("");
    return;
  }
  if (wifiClient.verify(fingerprint, host))
    Serial.println("Certificado OK");
  else
    Serial.println("Certificado NOK, verificar");

  String url = String("/macros/s/") + gScriptID + "/exec?ID=" + id;
  Serial.println("Requisitando URL: " + url);
  
  wifiClient.print("GET " + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "User-Agent: BuildFailureDetectorESP8266\r\n" +
                   "Connection: close\r\n\r\n");

  Serial.println("Request enviada");
  String bodyMsg = "";
  
  while (wifiClient.connected())
  {
    bodyMsg = redirectGetResponse(wifiClient, wifiClient.readString());
    break;
  }
  
  Serial.println("Resposta: " + bodyMsg);
  showSheetsResponse(bodyMsg);
  Serial.println("Fechando conexão!");
}

String redirectGetResponse (WiFiClientSecure wifiClient, String response)
{
  if (response.indexOf("HTTP/1.1 200") > -1)
  {
    return response.substring(response.indexOf("\r\n\r\n") + 4);
  }
  
  if (!wifiClient.connect(host2, httpsPort))
  {
    Serial.println("Falha na conexão ao host: " + String(host2));
    return "";
  }
  if (wifiClient.verify(fingerprint, host2))
    Serial.println("Certificado OK");
  else
    Serial.println("Certificado NOK, verificar");

  String urlRedirect = response.substring(response.indexOf("Location: ") + 10);
  String url = urlRedirect.substring(urlRedirect.indexOf("/macros"), urlRedirect.indexOf("\n"));
  Serial.println("Requisitando URL: " + url);
  
  wifiClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host2 + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("Request enviada");
  while (wifiClient.connected()) {
    return redirectGetResponse(wifiClient, wifiClient.readString());
  }
}

void showSheetsResponse (String ret)
{
  if(ret.startsWith("WARN")) //Alterna vermelho e verde
  {
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
    delay(500);
    digitalWrite(LED_GREEN, HIGH);
    delay(500);
    digitalWrite(LED_GREEN, LOW);
    delay(500);
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
    delay(500);
    digitalWrite(LED_GREEN, HIGH);
    delay(500);
    digitalWrite(LED_GREEN, LOW);
  }
  else if (ret.indexOf("ERRO") != -1) //Acende vermelho
  {
    digitalWrite(LED_RED, HIGH);
    delay(1500);
    digitalWrite(LED_RED, LOW);
  }
  else if (ret.indexOf("OK") != -1) //Acende verde
  {
    digitalWrite(LED_GREEN, HIGH);
    delay(1500);
    digitalWrite(LED_GREEN, LOW);
  }
  else if (ret.indexOf("Aluna nova cadastrada!") != -1) //Acende vermelho e verde
  {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    delay(1500);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
  }
  else //Outro: pisca vermelho
  {    
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
    delay(500);
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
    delay(500);
    digitalWrite(LED_RED, HIGH);
    delay(500);
    digitalWrite(LED_RED, LOW);
  }
}

void sinalizaSucesso()
{
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
  delay(150);
  digitalWrite(LED_GREEN, HIGH);
  delay(200);
  digitalWrite(LED_GREEN, LOW);
}
