#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include "BLE2902.h"

// Definicija unikatnega identifikatorja NUS storitve
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Spremenljivke
BLEDevice *pAdvertising;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic_TX;
BLECharacteristic *pCharacteristic_RX;
int connected = 0;
int connCount = 0;
int subscribed = 0;
char receivedString[1024];
uint16_t serialRecLen = 0;
uint16_t testVar = 0;
uint32_t currMillis, prevMillis = 0;
SemaphoreHandle_t distanceVarMutex;

/* Forward function definitions*/
void process_serial(char *receivedString, uint16_t len);
/*end functions definitions
 */

// Funkcija za  lovljenje dogodkov na BLE povezavi
class MyCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connected = 1;
    subscribed = 0;
    Serial.println("Connected!");
  }
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Disconnected!!");
    connected = 0;
    subscribed = 0;
    BLEDevice::startAdvertising();
  }
};
// Funkcija za  lovljenje dogodkov na TX značilnosti
class NusTxCallbacks : public BLECharacteristicCallbacks
{
  void onNotify(BLECharacteristic *pCharacteristic_TX)
  {
    Serial.println("Notify");
    subscribed = 1;
  }
  void onStatus(BLECharacteristic *pCharacteristic_TX, Status s, uint32_t code)
  {
    Serial.println("on status tx");
  }
  void onRead(BLECharacteristic *pCharacteristic)
  {
    Serial.println("On read");
  }
};
// Funkcija za  lovljenje dogodkov na RX značilnosti
class NusRxCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic_RX)
  {
    char dataString[100];
    Serial.println("Writing happened");
    uint32_t len = pCharacteristic_RX->getLength();
    uint8_t *p = pCharacteristic_RX->getData();
    strncpy(dataString, (char *)p, len);
    Serial.printf("Length %d, %s\r\n", len, dataString);
    process_serial(dataString, len);
  }
  void onStatus(BLECharacteristic *pCharacteristic_RX, Status s, uint32_t code)
  {
    Serial.println("On status rx");
  }
};

// Funkcija za testiranje preko serijskih komand
void process_serial(char *receivedString, uint16_t len)
{
  char responseString[100];
  memset(responseString, 100, 0);

  if (strncmp((char *)receivedString, "PING", 4) == 0)
  {
    Serial.println("PONG");
  }

  if (strncmp((char *)receivedString, "VAR ", 4) == 0) // set variable to selected value
  {
    char varStr[10];
    strcpy(varStr, receivedString + 4);
    xSemaphoreTake(distanceVarMutex, portMAX_DELAY);
    testVar = strtol(varStr, NULL, 10);
    xSemaphoreGive(distanceVarMutex);
    Serial.printf("Var is %d\r\n", testVar);
  }

  if (strncmp((char *)receivedString, "TARGET", 7) == 0)
  {
    Serial.println("Setting target name to:");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  distanceVarMutex = xSemaphoreCreateMutex(); // Ustvari mutex za varno branje spremenljivke

  BLEDevice::init("BLE_SRVR");                                                                                                                           // Inicializiraj moj BLE server in ga poimenuj
  pServer = BLEDevice::createServer();                                                                                                                   // Ustvari server
  pServer->setCallbacks(new MyCallbacks());                                                                                                              // Nastavi funkcije ki lovijo dogodke za povezavo
  pService = pServer->createService(SERVICE_UUID);                                                                                                       // Ustvari storitev ki bo na BLE serverju
  pCharacteristic_TX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);                                       // Nastavi značilnost storitve pošiljanja podatkov
  pCharacteristic_RX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR); // Nastavi značilnost storitve prejemanja podatkov
  pCharacteristic_TX->addDescriptor(new BLE2902());
  pCharacteristic_TX->setCallbacks(new NusTxCallbacks()); // Nastavi funkcije ki lovijo dogodke na storitvi oddajanja podatkov
  pCharacteristic_RX->setCallbacks(new NusRxCallbacks()); // Nastavi funkcije ki lovijo dogodke na storitvi prejemanja podatkov

  pService->start(); // Zaženi storitev

  // Nastavi parametre oglaševanja in začni oglaševanje
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop()
{

  // Sprejemanje serijskih komand preko serijskega porta za namen testiranja
  while (Serial.available())
  {
    receivedString[serialRecLen] = (char)Serial.read();

    if (receivedString[serialRecLen - 1] == '\r' && receivedString[serialRecLen] == '\n')
    {
      process_serial(receivedString, serialRecLen);
      serialRecLen = 0;
    }
    else
    {
      serialRecLen++;
      if (serialRecLen >= 1022)
      {
        serialRecLen = 0;
      }
    }
  }

  // Funkcija ki se požene vsako sekundo in pošlje podatke če ima naprava vzpostavljeno povezavo
  if (currMillis - prevMillis > 1000)
  {
    prevMillis = currMillis;

    if (connected)
    {
      // Izmeri razdaljo in pošlji
      char buf[20];
      testVar = rand() % (65 + 1 - 0) + 0;
      sprintf(buf, "Var:%d\r\n", testVar);
      pCharacteristic_TX->setValue(buf);
      Serial.println("Podatki poslani");
    }
  }

  currMillis = millis();
}