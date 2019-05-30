#include <U8g2lib.h>
#include <SoftwareSerial.h>

// Incluimos la clase que hemos implementado para gestionar una media móvil
// y el conjunto de atributos del elemento sensor.
#include "MovingAverage.h"
#include "SensorData.h"

// Clave con la IP.
#define MY_IP "192.168.0.8"

// Clave para la publicación de datos en ThingSpeak.com.
#define THINGSPEAK_KEY "N7VK389S52SRWNTV"

// Frecuencia (aproximada) de envío (en ms) a Thingspeak.com.
#define THINGSPEAK_SENDING_FREQ_MS 60000

// Descomentar la siguiente línea para activar el modo debug.
#define DEBUG

// Establecemos los pines que usaremos para conectar el módulo Bluetooth HC-05.
#define PIN_RX 10 // pin Arduino a conectar con el pin TX del HC-05
#define PIN_TX 11 // pin Arduino a conectar con el pin RX del HC-05

// Establecemos la velocidad de conexión con el módulo HC-05 que previamente
// se configuró.
#define BT_BAUD 9600

// Creamos el objeto que representa el módulo Bluetooth.
SoftwareSerial BT(PIN_RX,PIN_TX);

// Establecemos los pines que usaremos para conectar el módulo ESP8266.
#define PIN_RX 13 // pin Arduino a conectar con el pin TX del ESP8266
#define PIN_TX 12 // pin Arduino a conectar con el pin RX del ESP8266

// Establecemos la velocidad a la que está configurado el módulo ESP8266.
#define WIFI_BAUD 115200

// Creamos el objeto que representa el módulo ESP8266 que previamente se
// habrá configurado como host con los valores SSID y password de la red wifi.
// AT+CWMODE=1
// AT+CWJAP="SSID","PASSWORD"
SoftwareSerial esp8266(PIN_RX, PIN_TX);

// Asignar el pin analógico para la medida del sensor.
#define PIN_ANALOGICO_MEDIDA 5

// Asignamos la tensión de referencia para el sensor.
#define VS_mV 5000

// Asignar el pin analógico para la medida del potenciómetro.
#define PIN_ANALOGICO_POTENCIOMETRO 4

// Asignar pines digitales para la pantalla OLED.
#define DIGITAL_PIN_SCK 2
#define DIGITAL_PIN_SDA 3
#define DIGITAL_PIN_DC 4
#define DIGITAL_PIN_CS 5

// El pin RES debe estar a 5 V, podemos conectarlo a alimentación o usar
// un pin digital a nivel alto. Prefiero usar la conexión a alimentación
// porque así preservamos el pin digital para otros usos.

// Invocamos el constructor para crear el objeto que representa la pantalla.
U8G2_SH1106_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, DIGITAL_PIN_SCK,
DIGITAL_PIN_SDA, DIGITAL_PIN_CS, DIGITAL_PIN_DC);

void setup() {
#ifdef DEBUG
  // Configuramos la depuración.
  Serial.begin(9600);
#endif

  // Configuramos los pines analógicos como entradas.
  pinMode(PIN_ANALOGICO_MEDIDA, INPUT);
  pinMode(PIN_ANALOGICO_POTENCIOMETRO, INPUT);

  // Configuramos la pantalla OLED.
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setFontPosTop();
  u8g2.begin();

  // Configuramos el puerto serie que nos permite interactuar con el módulo
  // Bluetooth.
  BT.begin(BT_BAUD);

  // Configuramos el puerto serie que nos permite interactuar con el
  // modulo wifi.
  esp8266.begin(WIFI_BAUD);
}

float Volt2Temp(float value_mV) {
  // *** IMPORTANTE ***
  // Estoy usando un sensor de temperatura TMP36 que devuelve un valor de
  // tensión que debe transformarse en una temperatura considerando las
  // indicaciones del fabricante para este modelo.
  return (value_mV - 500) / 10;
}

float CurrentAnalogValue(int pin) {
  // Leemos el valor del sensor.
  int rawValue = analogRead(pin);

  // Transformamos en tensión.
  float value_mV = map(rawValue, 0, 1023, 0, VS_mV);

  // Transformamos en temperatura.
  float value_C = Volt2Temp(value_mV);

#ifdef DEBUG
  Serial.print("PIN = ");
  Serial.println(pin);
  Serial.print("value_mV = ");
  Serial.println(value_mV);
  Serial.print("value_C = ");
  Serial.println(value_C);
#endif

  return value_C;
}

void DisplaySensor(int valSensor, int valPot, bool alarm) {
  u8g2.firstPage();  
  do {
      u8g2.setCursor(0, 0);
      u8g2.print("TEMP = ");
      u8g2.print(String(valSensor).c_str());
      u8g2.print(" C");
      u8g2.setCursor(0, 15);
      u8g2.print("REF TEMP = ");
      u8g2.print(String(valPot).c_str());
      u8g2.print(" C");
      u8g2.setCursor(0, 30);
      if (alarm)
        u8g2.print("ALARM !!!");
  } while(u8g2.nextPage());
}

String DoSerial (int valSensor, int valPot, bool alarm) {
  String str(valSensor);
  str += "#";
  str += (alarm)? "1" : "0";
  str += '\n';
  return str;
}

void SendTemperature2ThingSpeakDotCom (int temp) {
  
    // Habilitamos la escucha por el puerto serie que representa
    // el módulo ESP8266 que nos ofrece la conexión WiFi.
    esp8266.listen();

    // Preparamos y enviamos los distintos comandos.
    String cmd;
    
    cmd = "AT+CIPMODE=0\r\n";
    esp8266.print(cmd.c_str());
#ifdef DEBUG
    Serial.println(cmd.c_str());
#endif
    delay(500);

    cmd = "AT+CIPMUX=0\r\n";
    esp8266.print(cmd.c_str());
#ifdef DEBUG
    Serial.println(cmd.c_str());
#endif
    delay(500);

    cmd = "AT+CIPSSLSIZE=4096\r\n";
    esp8266.print(cmd.c_str());
#ifdef DEBUG
    Serial.println(cmd.c_str());
#endif
    delay(500);

    cmd = "AT+CIPSTART=\"SSL\",\"api.thingspeak.com\",443\r\n";
    esp8266.print(cmd.c_str());
#ifdef DEBUG
    Serial.println(cmd.c_str());
#endif
    delay(1000);

    String httpGet = "GET /update?api_key=";
    httpGet += THINGSPEAK_KEY;
    httpGet += "&field1=";
    httpGet += String(temp);
    httpGet += "&headers=false HTTP/1.1\r\nHost: ";
    httpGet += MY_IP;
    httpGet += "\r\nConnection: close\r\nAccept: */*\r\n\r\n";

    cmd = "AT+CIPSEND=" + String(httpGet.length()) + "\r\n";
    esp8266.print(cmd.c_str());
    delay(500);
    esp8266.println(httpGet.c_str());
    delay(1000);
#ifdef DEBUG
    Serial.println(cmd.c_str());
    Serial.print(httpGet);
#endif

    cmd = "AT+CIPCLOSE\r\n";
    esp8266.print(cmd.c_str());
#ifdef DEBUG
    Serial.println(cmd.c_str());
#endif
    delay(1000);

    // Leemos toda la información que haya recibido el puerto.
    while (esp8266.available())
      esp8266.read();

    // Volvemos a hacer que el puerto de escucha sea el puerto BT (aunque
    // por ahora los wearables no responden a nuestros envíos).
    BT.listen();
}

void loop() {

  // Creamos un objeto que representa la situación del elemento sensor que,
  // al ser static, se preservará entre invocaciones de la función.
  static SensorData sensor;

  // Creamos los objetos que representan las medias móviles. Ponemos
  // una media móvil con más tamaño para el sensor para atenuar más
  // las pequeñas variaciones que las del potenciómetro. Esto hace
  // más cómodo variar la temperatura de referencia.
  static MovingAverage avgSensor(30);
  static MovingAverage avgPot(10);

  // Calculamos el valor de temperatura del sensor y del potenciómetro.
  int valSensor = avgSensor.Add(CurrentAnalogValue(PIN_ANALOGICO_MEDIDA));
  int valPot = avgPot.Add(CurrentAnalogValue(PIN_ANALOGICO_POTENCIOMETRO));

  // Crearemos un objeto que representa la situación ahora mismo.
  SensorData current(valSensor, valPot);

  // Si la situación actual del sensor ha variado respecto a la anterior
  // entonces actuamos.
  if (current != sensor)
  {
    // Mostramos el valor de la temperatura del sensor y de la
    // temperatura de referencia establecida por el potenciómetro
    // y la condición de alarma en la pantalla OLED.
    DisplaySensor(current.GetSensorValue(), current.GetPotValue(), current.GetAlarm());

    // Para normalizar el intercambio de información invocaremos una
    // función que recibe el valor del sensor, el valor de referencia
    // y si hay condición de alarma y devuelve un string formateado.
    // El formato debe ser el mismo que se asuma en el código del
    // wearable para realizar la operación inversa.
    String data = DoSerial(current.GetSensorValue(), current.GetPotValue(), current.GetAlarm());
  
    // Enviamos el dato serializado por el puerto Bluetooth.
    BT.write(data.c_str());
    
#ifdef DEBUG
    Serial.println("Status changed. Refreshing display and sending new data.");
    Serial.println(data.c_str());
#endif

    // Actualizamos el estado.
    sensor = current;
  }
 
  // Esperamos entre iteraciones del bucle principal.
  const unsigned int iter_delay_ms = 100;
  delay(iter_delay_ms);

  // Para hacer un envío periódico a ThingSpeak.com tenemos en cuenta
  // la frecuencia prevista de envío y el número de iteraciones.
  static unsigned int numIter = 0;
  if (numIter >= THINGSPEAK_SENDING_FREQ_MS / iter_delay_ms)
  {
#ifdef DEBUG
    Serial.println("Sending data to ThingSpeak.com");
#endif

    // Enviamos el dato.
    SendTemperature2ThingSpeakDotCom(sensor.GetSensorValue());

    // Reseteamos el contador de iteraciones.
    numIter = 0;
  }
  else
    numIter++;
}
