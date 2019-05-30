#include <U8g2lib.h>
#include <SoftwareSerial.h>

// Descomentar la siguiente línea para activar el modo debug.
#define DEBUG

// Establecemos los pines que usaremos para conectar el módulo Bluetooth HC-05.
#define PIN_RX 10 // pin Arduino a conectar con el pin TX del HC-05
#define PIN_TX 11 // pin Arduino a conectar con el pin RX del HC-05

// Establecemos la velocidad de conexión. Usaremos la que previamente se configuró
// en los módulos.
#define BAUD 9600

// Creamos el objeto que representa el módulo Bluetooth.
SoftwareSerial BT(PIN_RX,PIN_TX);

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
  BT.begin(BAUD);
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

void Display(int valSensor, int valPot, bool alarm) {
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

// Clase auxiliar para manejar un buffer circular.
template <class T>
class CircularBuffer {
  public:
  CircularBuffer (const int size);
  ~CircularBuffer ();
  void Add (const T& value);
  int NumberOfElements () const;
  T& operator[] (int index) const;

  private:
  int maxSize;
  int firstPos;
  int numberOfElements;
  T* buffer;
};

template <class T>
CircularBuffer<T>::CircularBuffer (const int size) {
  maxSize = size;
  firstPos = numberOfElements = 0;
  buffer = new T[maxSize];
}

template <class T>
CircularBuffer<T>::~CircularBuffer () {
    delete buffer;
}

template <class T>
void CircularBuffer<T>::Add (const T& value) {
  // Calculamos la posición de inserción.
  int firstFreePos = (firstPos + numberOfElements) % maxSize;

  // Insertamos el valor.
  buffer[firstFreePos] = value;

  // El primer elemento del buffer se desplaza si este
  // ya se ha llenado. En caso contrario no hace falta
  // descartarlo.
  if (numberOfElements >= maxSize)
    firstPos++;
  else
    numberOfElements++;  
}

template <class T>
int CircularBuffer<T>::NumberOfElements () const {
  return numberOfElements;
}

template <class T>
T& CircularBuffer<T>::operator[] (int index) const {
  // Transformamos el posicionamiento de index en el rango
  // 0 .. N-1 a las posiciones en el buffer circular.
  int pos = (firstPos + index) % maxSize;

  // Devolvemos el valor.
  return buffer[pos];
}

// Clase auxiliar para manejar una media móvil.
class MovingAverage {
  public:
  MovingAverage (const int size);
  float Add (const float newValue);

  private:
  CircularBuffer<float> avg;
};

MovingAverage::MovingAverage (const int size) :avg(size) {
}

float MovingAverage::Add (const float newValue) {
  avg.Add(newValue);

  float sum = 0;
  for (int i = 0; i < avg.NumberOfElements(); i++)
    sum += avg[i];

  return sum / avg.NumberOfElements();
}

// Clase auxiliar para representar los datos del sensor y su situación.
class SensorData {
  public:
  SensorData (int s, int p);
  void SetSensorValue (int value);
  int GetSensorValue () const;
  void SetPotValue (int value);
  int GetPotValue () const;
  bool GetAlarm () const;
  
  private:
  int valSensor;
  int valPot;
};

SensorData::SensorData (int s = 0, int p = 0) {
  SetSensorValue(s);
  SetPotValue(p);
}

void SensorData::SetSensorValue (int value) {
  valSensor = value;
}

int SensorData::GetSensorValue () const {
  return valSensor;
}

void SensorData::SetPotValue (int value) {
  valPot = value;
}

int SensorData::GetPotValue () const {
  return valPot;
}

bool SensorData::GetAlarm () const {
  return (valSensor > valPot);
}

bool operator== (const SensorData& left, const SensorData& right) {
  if (left.GetSensorValue() != right.GetSensorValue())
    return false;
  if (left.GetPotValue() != right.GetPotValue())
    return false;
  if (left.GetAlarm() != right.GetAlarm())
    return false;

  return true;
}

bool operator!= (const SensorData& left, const SensorData& right) {
  return !(left == right);
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
    Display(current.GetSensorValue(), current.GetPotValue(), current.GetAlarm());

    // Para normalizar el intercambio de información invocaremos una
    // función que recibe el valor del sensor, el valor de referencia
    // y si hay condición de alarma y devuelve un string formateado.
    // El formato debe ser el mismo que se asuma en el código del
    // wearable para realizar la operación inversa.
    String data = DoSerial(current.GetSensorValue(), current.GetPotValue(), current.GetAlarm());
  
    // Enviamos el valor por el puerto Bluetooth.
    BT.write(data.c_str());
    
#ifdef DEBUG
    Serial.println("Status changed. Refreshing display and sending new data.");
    Serial.println(data.c_str());
#endif

    // Actualizamos el estado.
    sensor = current;
  }
  else
  {
#ifdef DEBUG
    Serial.println("Status not changed.");
#endif
  }
  
  // Esperamos 100 ms.
  delay(100);
}
