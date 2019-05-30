// Descomentar para activar modo depuración.
#define DEBUG

// Establecemos los pines que usaremos para conectar el módulo Bluetooth HC-05.
#define PIN_RX 10 // pin Arduino a conectar con el pin TX del HC-05
#define PIN_TX 11 // pin Arduino a conectar con el pin RX del HC-05

// Establecemos la velocidad de conexión. Usaremos la que previamente se configuró
// en los módulos.
#define BAUD 9600

// Incluímos la librería para poder interactuar con el módulo Bluetooth a través
// un puerto serie software.
#include <SoftwareSerial.h>

// Creamos el objeto que representa el módulo Bluetooth.
SoftwareSerial BT(PIN_RX,PIN_TX);

// Creamos el String que actuará como buffer de recepción.
String data;

void setup()
{
  BT.begin(BAUD);
#ifdef DEBUG
  Serial.begin(9600);
#endif
}

void loop()
{
  // Procesamos los datos recibidos desde el módulo Bluetooth.
  // Leemos caracter a carácter hasta un salto de línea. Entonces
  // tendremos el valor completo.
  if(BT.available())
  {
    // Se reciben datos carácter a carácter.
    char c = BT.read();

    // Hemos recibido el salto de línea, así que el contenido
    // recibido hasta el momento es el total de los datos.
    if (c == '\n')
    {
      int val = atoi(data.c_str());

#ifdef DEBUG
      Serial.print("TEMPERATURE IS ");
      Serial.print(val);
      Serial.println(" C ");
#endif

      data = "";
    }
    else
      data += c;
  }
}
