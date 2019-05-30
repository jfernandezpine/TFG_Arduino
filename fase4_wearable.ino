// Descomentar para activar modo depuración.
#define DEBUG

// Establecemos el pin para encender el LED.
#define PIN_LED 7

// Establecemos el pin para encender el zumbador. Nos aseguramos de
// elegir uno que soporte PWM.
#define PIN_NOISE 6

// Frecuencia del zumbador para la vibración.
#define NOISE_FREQ 1000

// Establecemos el pin para detectar el pulsador. Hay que asegurarse de
// elegir uno que permita gestionar interrupciones.
#define PIN_BUTTON 2

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

// Variable para saber si el botón ya ha sido pulsado para reconocer la
// alarma.
volatile bool AlarmAck = false;

void TurnOffNoise() {
  // Reconocemos la alarma y apagamos el zumbador.
  AlarmAck = true;
  noTone(PIN_NOISE);
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);
  BT.begin(BAUD);

  // Configuramos la interrupción asociada al flanco ascendente sobre
  // el pin digital conectado al pulsador.
  pinMode(PIN_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), TurnOffNoise, RISING);

#ifdef DEBUG
  Serial.begin(9600);
#endif
}

void UnDoSerial (String str, int& valSensor, int& valPot, bool& alarm) {
  // El entero hasta el carácter # es valSensor.
  String aux = str.substring(0, str.indexOf('#'));
  valSensor = aux.toInt();
  
  // El 0/1 después del carácter # es alarm.
  aux = str.substring(str.indexOf('#') + 1, str.length());
  alarm = aux.toInt();
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
#ifdef DEBUG
      Serial.println(data.c_str());
#endif

      int valSensor;
      int valPot;
      bool alarm;
      UnDoSerial(data, valSensor, valPot, alarm);

#ifdef DEBUG
      Serial.print("TEMPERATURE IS ");
      Serial.print(valSensor);
      Serial.println(" C ");
      if (alarm)
        Serial.println("ALARM !!!");
#endif

      if (alarm)
      {
#ifdef DEBUG
        Serial.println("Alarma!!!");
#endif

        // Encendemos el LED.
        digitalWrite(PIN_LED, HIGH);

        // El zumbador se activa sólo si no se ha reconocido la alarma.
        if (AlarmAck)
          noTone(PIN_NOISE);
        else
          tone(PIN_NOISE, NOISE_FREQ);
      }
      else // alarm == false
      {
#ifdef DEBUG
        Serial.println("Fin alarma.");
#endif

        // Apagamos el LED.
        digitalWrite(PIN_LED, LOW);

        // Apagamos el zumbador.
        noTone(PIN_NOISE);

        // Deshacemos el reconocimiento para la próxima vez
        // que se active la alarma.
        AlarmAck = false;
      }

      data = "";
    }
    else
      data += c;
  }
}
