#include <U8g2lib.h>

// Descomentar la siguiente línea para activar el modo debug.
#define DEBUG

// Asignar el pin analógico para la medida del sensor.
#define PIN_ANALOGICO_MEDIDA 5

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
}

void ShowSensorValue(float value){
  u8g2.firstPage();  
  do {

      u8g2.setCursor(0, 0);
      u8g2.print("SENSOR VALUE IS");
      u8g2.setCursor(0, 15);
      u8g2.print(String(value).c_str());
      u8g2.print(" V");
  } while(u8g2.nextPage());  
}

void loop() {
  // *** IMPORTANTE ***
  // Estoy usando un potenciómetro para simular un sensor con salida entre 0 y 5V = 5000mV.

  // Leemos el valor del sensor.
  int rawValue = analogRead(PIN_ANALOGICO_MEDIDA);

  // El valor leído lo remapearemos al rango 0-5000mV;
  float value_mV = map(rawValue, 0, 1023, 0, 5000);
  float value_V = value_mV / 1000.0;

#ifdef DEBUG
  Serial.println(rawValue);
  Serial.print(value_mV);
  Serial.print(" mV = ");
  Serial.print(value_V);
  Serial.println(+ " V");
#endif

  // Mostramos el valor en la pantalla OLED.
  ShowSensorValue(value_V);
  
  // Esperamos medio segundo.
  delay(500);
}
