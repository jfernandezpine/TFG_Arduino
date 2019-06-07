#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Incluimos la clase que hemos implementado para gestionar una media móvil
// y el conjunto de atributos del elemento sensor.
#include "MovingAverage.h"
#include "SensorData.h"

///////////////////////////////////////////////////////////////////////////////

// Temperatura de referencia por defecto.
const int DEFAULT_REF_TEMP = 18;

// Datos para la conexión WiFi.
const char* WIFI_SSID = "BakerTeam2.4G";
const char* WIFI_PWD = "Comando Baker";
const unsigned long WIFI_MS_TIMEOUT = 10*1000; // 10 segundos

// Frecuencia (aproximada) de envío (en ms) a Thingspeak.com.
#define THINGSPEAK_SENDING_FREQ_MS 60000

// Datos para la conexión a ThingSpeak.
const char* THINGSPEAK_KEY = "N7VK389S52SRWNTV";

// Parámetros de configuración BLE.
const char* BLE_SERVICE_UUID = "19686c12-2faa-486e-9b1c-a1c286cfc540";
const char* BLE_CHARACTERISTIC_UUID = "d053c0e2-36cf-432f-9ea5-681ef9d5995f";

// Pin al que está conectado el sensor de temperatura.
const int TMP_PIN = 36;

///////////////////////////////////////////////////////////////////////////////

void PaintBaseLCD() {
  // Limpiamos la pantalla.
  M5.Lcd.clear();
  
  // Pintamos el botón +.
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(58, 215);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.printf("+");
  M5.Lcd.drawRect(35, 212, 60, 30, RED);

  // Pintamos el botón -.
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(150, 215);
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.printf("-");
  M5.Lcd.drawRect(127, 212, 60, 30, BLUE);
}

void UpdateLCD (const SensorData& s) {
    PaintBaseLCD();
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(58, 15);
    M5.Lcd.printf("Current temp");
    M5.Lcd.setCursor(120, 55);
    M5.Lcd.printf(String(s.GetSensorValue()).c_str());
    M5.Lcd.printf(" C");
    if (s.GetAlarm())
    {
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(85, 95);
      M5.Lcd.printf("ALARM!!!");
    }
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(40, 180);
    M5.Lcd.setTextSize(2);
    String text = "Ref temp ";
    text += String(s.GetPotValue());
    M5.Lcd.printf(text.c_str());
    M5.update();
}

///////////////////////////////////////////////////////////////////////////////

String DoSerial (int valSensor, int valPot, bool alarm) {
  String str(valSensor);
  str += "#";
  str += (alarm)? "1" : "0";
  str += '\n';
  return str;
}

///////////////////////////////////////////////////////////////////////////////

void Sent2ThingSpeaks(const SensorData& s) {
  String request = "https://api.thingspeak.com/update?api_key=";
  request += THINGSPEAK_KEY;
  request += "&field1=";
  request += String(s.GetSensorValue());
  
  HTTPClient https;
  https.begin(request);
  int ret = https.GET();
  if (ret == HTTP_CODE_OK || ret == HTTP_CODE_MOVED_PERMANENTLY)
    String response = https.getString();
}

///////////////////////////////////////////////////////////////////////////////

float Volt2Temp(int value_mV) {
  // *** IMPORTANTE ***
  // Estoy usando un sensor de temperatura TMP36 que devuelve un valor de
  // tensión que debe transformarse en una temperatura considerando las
  // indicaciones del fabricante para este modelo.
  return (value_mV - 500) / 10;
}

///////////////////////////////////////////////////////////////////////////////

// Necesitamos una variable global para que sea accesible desde el bucle.
BLECharacteristic* pCharacteristic = 0;

void setup() {

  // Se inicializa el objeto que representa el M5Stack.
  M5.begin();

  // M5Stack tiene un problema documentado de ruido en el altavoz que
  // incluye bajo determinadas circunstancias. Para minimizarlo anulamos
  // el altavoz.
  dacWrite(25, 0);

  // Intentamos conectar a la red WiFi hasta que se alcance el timeout.
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - t > WIFI_MS_TIMEOUT)
      break;
    DisplayDebug("Connecting WiFi");
    delay(500);
  }

  // Informamos del resultado de la conexión WiFi.
  String text;
  if (WiFi.status() == WL_CONNECTED)
  {
    text = "WiFi connected!\n\nAddress is ";
    text += WiFi.localIP().toString();
  }
  else
  {
    text = "WiFi connection failed!";
  }
  DisplayDebug(text);
  delay(2000);

  // Configuramos y activamos la conectividad BLE.
  BLEDevice::init("Sensor");
  BLEServer* pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(BLE_SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    BLE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

///////////////////////////////////////////////////////////////////////////////

void loop() {

  // Creamos un objeto que representa la situación del elemento sensor que,
  // al ser static, se preservará entre invocaciones de la función.
  static SensorData sensor(0, DEFAULT_REF_TEMP);

  // Creamos el objeto que representa la media móvil para guardar los datos
  // del sensor y atenuar las pequeñas variaciones.
  static MovingAverage avgSensor(100);

  // Calculamos el valor de temperatura del sensor.
  int valSensor = avgSensor.Add(Volt2Temp(analogRead(TMP_PIN)));

  // Crearemos un objeto que representa la situación ahora mismo.
  SensorData current(valSensor, sensor.GetPotValue());

  // Si la situación actual del sensor ha variado respecto a la anterior
  // entonces actuamos. También lo hacemos si se pulsa el botón C.
  if ((current != sensor) || M5.BtnC.wasPressed())
  {
    sensor = current;

    // Refrescamos la información en la pantalla.
    UpdateLCD(sensor);

    // Para normalizar el intercambio de información invocaremos una
    // función que recibe el valor del sensor, el valor de referencia
    // y si hay condición de alarma y devuelve un string formateado.
    // El formato debe ser el mismo que se asuma en el código del
    // wearable para realizar la operación inversa.
    String data = DoSerial(current.GetSensorValue(), current.GetPotValue(), current.GetAlarm());
    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();
  }

  // Por último, gestionamos las pulsaciones de los botones.
  if (M5.BtnA.wasPressed())
  {
    sensor.SetPotValue(sensor.GetPotValue() + 1);

    // Notificamos el cambio.
    String data = DoSerial(sensor.GetSensorValue(), sensor.GetPotValue(), sensor.GetAlarm());
    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();

    // Refrescamos la información en la pantalla.
    UpdateLCD(sensor);
  }
  if (M5.BtnB.wasPressed())
  {
    sensor.SetPotValue(sensor.GetPotValue() - 1);

    // Notificamos el cambio.
    String data = DoSerial(sensor.GetSensorValue(), sensor.GetPotValue(), sensor.GetAlarm());
    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();

    // Refrescamos la información en la pantalla.
    UpdateLCD(sensor);
  }
  if (M5.BtnC.pressedFor(2000))
    M5.powerOFF();

  M5.update();

  // Esperamos entre iteraciones del bucle principal.
  const unsigned int iter_delay_ms = 100;
  delay(iter_delay_ms);

  // Para hacer un envío periódico a ThingSpeak.com tenemos en cuenta
  // la frecuencia prevista de envío y el número de iteraciones.
  static unsigned int numIter = 0;
  if (numIter >= THINGSPEAK_SENDING_FREQ_MS / iter_delay_ms)
  {
    // Enviamos el dato.
    Sent2ThingSpeaks(sensor);

    // Reseteamos el contador de iteraciones.
    numIter = 0;
  }
  else
    numIter++;
}

///////////////////////////////////////////////////////////////////////////////

void DisplayDebug (String str) {
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf(str.c_str());
  M5.update();
}
