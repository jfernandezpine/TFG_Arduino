#include <M5Stack.h>
#include "BLEDevice.h"

///////////////////////////////////////////////////////////////////////////////

// Parámetros de configuración BLE.
const char* BLE_SERVICE_UUID = "19686c12-2faa-486e-9b1c-a1c286cfc540";
const char* BLE_CHARACTERISTIC_UUID = "d053c0e2-36cf-432f-9ea5-681ef9d5995f";
const unsigned long BLE_S_TIMEOUT = 10; // 10 segundos

///////////////////////////////////////////////////////////////////////////////

// Variable para guardar los datos recibidos por BLE.
String BLE_Buffer;

// Variable para saber si el botón ya ha sido pulsado para reconocer la
// alarma.
bool AlarmAck = false;

// Variable para saber si ya estábamos en la condición de alarma.
bool InAlarmStatus = false;

///////////////////////////////////////////////////////////////////////////////

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    BLE_Buffer = (char*) pData;
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient* pclient) {
    }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice);
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(BLE_SERVICE_UUID));
  if (pRemoteService == nullptr)
  {
    pClient->disconnect();
    return false;
  }
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(BLE_CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr)
  {
    pClient->disconnect();
    return false;
  }
  if(pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  connected = true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(BLE_SERVICE_UUID)))
    {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

///////////////////////////////////////////////////////////////////////////////

void UnDoSerial (String str, int& valSensor, int& valPot, bool& alarm) {
  
  // El entero hasta el carácter # es valSensor.
  String aux = str.substring(0, str.indexOf('#'));
  valSensor = aux.toInt();
  
  // El 0/1 después del carácter # es alarm. Lo demás es el salto de línea
  // que podemos ignorar.
  aux = str[str.indexOf('#') + 1];
  alarm = aux.toInt();
}

///////////////////////////////////////////////////////////////////////////////

void UpdateLCD (int valSensor, bool alarm) {
  // Limpiamos la pantalla.
  M5.Lcd.clear();

  // Si la alarma está activa y todavía no se ha reconocido entonces pintamos
  // el botón !.
  if (alarm && !AlarmAck)
  {
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(58, 215);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.printf("!");
    M5.Lcd.drawRect(35, 212, 60, 30, RED);
  }

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(58, 15);
  M5.Lcd.printf("Current temp");
  M5.Lcd.setCursor(120, 55);
  M5.Lcd.printf(String(valSensor).c_str());
  M5.Lcd.printf(" C");

  if (alarm)
  {
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setCursor(85, 95);
    M5.Lcd.printf("ALARM!!!");
  }

  M5.update();
}

///////////////////////////////////////////////////////////////////////////////

void setup() {

  // Se inicializa el objeto que representa el M5Stack.
  M5.begin();
  M5.Speaker.begin();

  // Configuramos y activamos la conectividad BLE.
  BLEDevice::init("Wearable1");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(BLE_S_TIMEOUT, false);
}

///////////////////////////////////////////////////////////////////////////////

static bool firstIteration = true;

void loop() {

  // Establecemos la conexión BLE.
  if (doConnect)
  {
    connectToServer();
    doConnect = false;
  }

  // En la primera iteración del bucle no esperamos a recibir una
  // notificación sino que leemos directamente el dato publicado.  
  if (firstIteration)
  {
    if(pRemoteCharacteristic->canRead())
    {
      std::string value = pRemoteCharacteristic->readValue();
      BLE_Buffer = value.c_str();
    }
    firstIteration = false;
  }

  int valSensor;
  int valPot;
  bool alarm;
  bool DisplayUpdateNeeded = false;

  // Si hemos recibido datos por BLE entonces actuamos.
  if (BLE_Buffer != "")
  {
    // Extraemos la información.
    UnDoSerial(BLE_Buffer, valSensor, valPot, alarm);
    DisplayUpdateNeeded = true;
    BLE_Buffer = "";

    // Actuamos frente a la entrada/salida de la situación de alarma.
    if (alarm && !InAlarmStatus)
    {
      InAlarmStatus = true;
      AlarmAck = false;
      M5.Speaker.beep();
    }
    else if (!alarm && InAlarmStatus)
    {
      InAlarmStatus = false;
      AlarmAck = false;
      M5.Speaker.mute();
    }
  }

  // Tanto si hemos recibido datos como si no, actuamos
  // ante la aceptación de una alarma.
  if (InAlarmStatus && !AlarmAck && M5.BtnA.wasPressed())
  {
    AlarmAck = true;
    DisplayUpdateNeeded = true;
    M5.Speaker.mute();
  }

  // También actuamos ante una situación de alarma no reconocida.
  if (InAlarmStatus && !AlarmAck)
    M5.Speaker.beep();

  if (DisplayUpdateNeeded)
    UpdateLCD(valSensor, alarm);

  if (M5.BtnC.pressedFor(2000))
    M5.powerOFF();  

  M5.update();

  delay(100);
}

///////////////////////////////////////////////////////////////////////////////

void DisplayDebug (String str) {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf(str.c_str());
  M5.update();
}
