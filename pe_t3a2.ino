#include <DHT.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Servo.h>

#define DHTPIN 22
DHT dht(DHTPIN, DHT11);

#define SERVOPIN 23
Servo servo;

bool servoEnabled = false;

#define BLE_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_SERVICE_NAME "esp32-ble"

BLECharacteristic temperatureCharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26a8", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x2902));

BLECharacteristic servoCharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26a7", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
BLEDescriptor servoDescriptor(BLEUUID((uint16_t)0x2902));

BLECharacteristic ledsCharacteristics[] = {
  BLECharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26a9", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE),
  BLECharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26aa", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE),
  BLECharacteristic("beb5483e-36e1-4688-b7f5-ea07361b26ab", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE),
};
BLEDescriptor ledsDescriptors[] = {
  BLEDescriptor(BLEUUID((uint16_t)0x2902)),
  BLEDescriptor(BLEUUID((uint16_t)0x2902)),
  BLEDescriptor(BLEUUID((uint16_t)0x2902)),
};

bool ledsValues[] = { false, false, false };
int ledsPins[] = { 18, 19, 21 };

bool deviceConnected = false;

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 1) {
      for (int i = 0; i < 3; i++) {
        if (pCharacteristic == &ledsCharacteristics[i]) {
          ledsValues[i] = value[0] == '1';
          digitalWrite(ledsPins[i], ledsValues[i] ? HIGH : LOW);
        }
      }
    }
  }
};

class ServoCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 1) {
      servoEnabled = value[0] == '1';

      if (servoEnabled) {
        servo.write(90);
      } else {
        servo.write(0);
      }
    }
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);
  dht.begin();

  servo.attach(SERVOPIN);

  BLEDevice::init(BLE_SERVICE_NAME);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *dhtService = pServer->createService(BLE_SERVICE_UUID);

  temperatureDescriptor.setValue("Temperatura");
  temperatureCharacteristic.addDescriptor(&temperatureDescriptor);
  dhtService->addCharacteristic(&temperatureCharacteristic);

  servoDescriptor.setValue("Servo Motor");
  servoCharacteristic.addDescriptor(&servoDescriptor);
  servoCharacteristic.setCallbacks(new ServoCharacteristicCallbacks());
  dhtService->addCharacteristic(&servoCharacteristic);

  for (int i = 0; i < 3; i++) {
    pinMode(ledsPins[i], OUTPUT);
    ledsDescriptors[i].setValue("LED " + (i + 1));
    ledsCharacteristics[i].addDescriptor(&ledsDescriptors[i]);
    ledsCharacteristics[i].setCallbacks(new CharacteristicCallbacks());
    dhtService->addCharacteristic(&ledsCharacteristics[i]);
  }

  dhtService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pServer->getAdvertising()->start();
}

void loop() {
  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("No se pudo leer la temperatura.");
    return;
  }

  if (deviceConnected) {
    static char temperature[6];
    sprintf(temperature, "%.2f", t);

    temperatureCharacteristic.setValue(temperature);
    temperatureCharacteristic.notify();
  }

  delay(5000);
}
