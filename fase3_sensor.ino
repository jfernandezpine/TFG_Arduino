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

// Asignar pines digitales para la pantalla OLED.
#define DIGITAL_PIN_SCK 2
#define DIGITAL_PIN_SDA 3
#define DIGITAL_PIN_DC 4
#define DIGITAL_PIN_CS 5

// El pin RES debe estar a 5 V, podemos conectarlo a alimentación o usar un pin digital
// a nivel alto. Prefiero usar la conexión a alimentación porque así preservamos el pin
// digital para otros usos.

// Invocamos el constructor para crear el objeto que representa la pantalla.
U8G2_SH1106_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, DIGITAL_PIN_SCK, DIGITAL_PIN_SDA, DIGITAL_PIN_CS, DIGITAL_PIN_DC);

void setup() {
#ifdef DEBUG
  // Configuramos la depuración.
  Serial.begin(9600);
#endif

  // Configuramos el pin analógico como entrada.
  pinMode(PIN_ANALOGICO_MEDIDA, INPUT);

  // Configuramos la pantalla OLED.
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setFontPosTop();
  u8g2.begin();

  // Configuramos el puerto serie que nos permite interactuar con el módulo Bluetooth.
  BT.begin(BAUD);
}

void ShowSensorValue(int value){
  u8g2.firstPage();  
  do {

      u8g2.setCursor(0, 0);
      u8g2.print("TEMPERATURE IS");
      u8g2.setCursor(0, 15);
      u8g2.print(String(value).c_str());
      u8g2.print(" C");
  } while(u8g2.nextPage());  
}

float CurrentSensorValue(){
  // *** IMPORTANTE ***
  // Estoy usando un sensor de temperatura TMP36 que devuelve un valor de tensión que
  // debe transformarse en una temperatura considerando las indicaciones del fabricante
  // para este modelo.

  // Leemos el valor del sensor.
  int rawValue = analogRead(PIN_ANALOGICO_MEDIDA);

  // Transformamos en tensión.
  float value_mV = map(rawValue, 0, 1023, 0, VS_mV);

  // Ahora transformamos en temperatura a partir de la ecuación de la recta que se
  // deduce de la gráfica de las especificaciones.
  double value_C = (value_mV - 500) / 10;

  #ifdef DEBUG
    Serial.println(rawValue);
    Serial.print(value_C);
    Serial.println(+ " ºC");
  #endif

  return value_C;
}

int AvgSensorValue(){
  // El sensor utilizado acepta variaciones en el resultado de hasta 2ºC debido
  // a sus limitaciones de precisión. Para evitar que lecturas consecutivas muestren
  // valores ligeramente distintos por este motivo que puedan resultar confusos y
  // generar "ruido" en los datos que se usarán más adelante, calcularemos una
  // media móvil y ese será el valor que consideraremos como temperatura.
  // Además, como la precisión tiene de orden de magnitud 1ºC, no tiene sentido
  // trabajar con decimales, por lo que tras calcular la media móvil haremos un
  // redondeo que completaremos con el casting a int del retorno de la función.
  
  static float avg [3];
  static int n = 0;

  // Primera invocación de la función. Todavía no hay ningún valor en el array.
  // El nuevo valor es el primero y él mismo es la media.
  if (n == 0)
  {
    avg[0] = CurrentSensorValue();
    n++;
    return round(avg[0]);
  }

  // Segunda invocación de la función. Ya hay un valor en el array.
  // El nuevo valor es el segundo y hay que calcular la media junto con el
  // otro que ya está en el array.
  if (n == 1)
  {
    avg[1] = CurrentSensorValue();
    n++;
    return round((avg[0] + avg[1]) / 2);
  }

  // Tercera invocación de la función. Ya hay dos valores en el array.
  // El nuevo valor es el tercero y hay que calcular la media con los
  // otros dos que ya están en el array.
  if (n == 2)
  {
    avg[2] = CurrentSensorValue();
    n++;
    return round((avg[0] + avg[1] + avg[2]) / 3);
  }

  // A partir de la cuarta invocación siempre hay que hacer lo mismo:
  // Descartar el valor más antiguo, mover los demás por el array
  // y añadir el nuevo.
  if (n >= 3)
  {
    avg[0] = avg[1];
    avg[1] = avg[2];
    avg[2] = CurrentSensorValue();
    return round((avg[0] + avg[1] + avg[2]) / 3);
  }
}

void loop() {

  // Calculamos el valor del sensor.
  int val = AvgSensorValue();

  // Mostramos el valor en la pantalla OLED.
  ShowSensorValue(val);

  // Para normalizar el intercambio de información enviaremos el
  // número transformado en un string y terminado con un salto de
  // línea.
  String data = String(val);
  data += '\n';

  // Enviamos el valor por el puerto Bluetooth.
  BT.write(data.c_str());
  
  // Esperamos un segundo.
  delay(1000);
}
