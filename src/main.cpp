#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEDevice *pAdvertising;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic_TX;
BLECharacteristic *pCharacteristic_RX;
int connected = 0;
int connCount = 0;

class MyCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    connected = 1;
  }
  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Disconnected!!");
    connected = 0;
    BLEDevice::startAdvertising();
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("Long name works now");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic_TX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic_RX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  // pCharacteristic->setValue("Hello World says Neil");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop()
{
  // put your main code here, to run repeatedly:
  Serial.println("Hey");

  delay(2000);

  if (connected)
  {
    pCharacteristic_TX->setValue("lol");
    Serial.println("dsfsdfs");
  }
}