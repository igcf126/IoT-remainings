#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>   // Incluimos la librería  SoftwareSerial  
#include <avr/interrupt.h>
SoftwareSerial BT(10,11);    // Definimos los pines RX y TX del Arduino conectados al Bluetooth

#define RST_PIN 9    // Pin 9 para el reset del RC522
#define SS_PIN 53   // Pin 10 para el SS (SDA) del RC522
#define MOTION_PIN 2 // Pin del sensor de movimiento
#define LED_PIN 13   // Pin del LED
#define BUZZER_PIN 4 // Pin del buzzer
#define LIGHT_SENSOR_PIN A0 // Pin del sensor de luz
#define SERVO_PIN 3 // Pin al que está conectado el servo

Servo puertaServo; // Crear un objeto de tipo Servo

MFRC522 mfrc522(SS_PIN, RST_PIN); // Creamos el objeto para el RC522

bool abre = true;
bool cierra = !abre;
bool EnEmergencia = true; 
volatile bool suena = false; // Condición booleana
const unsigned long interval = 1000; // Intervalo de tiempo en milisegundos

bool isEmergencyActive = true; // Variable para indicar si la emergencia está activa
bool checkCardID(); // Prototipo de función
bool isNightTime(); // Prototipo de función
void activateAlarm(); // Prototipo de función
void controlPuerta(bool abrir);


void setup() {
    BT.begin(9600);       // Inicializamos el puerto serie BT (Para Modo AT 2)
  Serial.begin(9600); // Iniciamos la comunicación serial
  SPI.begin();        // Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos el MFRC522
  pinMode(MOTION_PIN, INPUT); // Configuramos el pin del sensor de movimiento como entrada con resistencia de pull-up
  pinMode(LED_PIN, OUTPUT);   // Configuramos el pin del LED como salida
  pinMode(BUZZER_PIN, OUTPUT); // Configuramos el pin del buzzer como salida
  Serial.println("Lectura del UID");
  puertaServo.attach(SERVO_PIN); // Adjuntar el pin del servo al objeto Servo


  // Configurar el timer para generar una interrupción cada segundo
  // Configurar el timer (Timer 2)
  noInterrupts(); // Deshabilitar interrupciones

  // Configurar el preescalador para generar una interrupción cada medio segundo
  TCCR2A = 0;
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); // Preescalador de 1024
  OCR2A = 15624; // Establecer el valor de comparación para generar una interrupción cada 250 ms
  TIMSK2 = (1 << OCIE2A); // Habilitar la interrupción del comparador A

  interrupts(); // Habilitar interrupciones
}

void loop() {
  // Revisamos si hay nuevas tarjetas presentes
  if (mfrc522.PICC_IsNewCardPresent()) {
    // Seleccionamos una tarjeta
    if (mfrc522.PICC_ReadCardSerial()) {
      // Enviamos seriamente su UID
      Serial.print("Card UID:");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println();
      Serial.println(isEmergencyActive);
      // Verificamos si el ID de la tarjeta coincide con el ID específico

      if (checkCardID()) {
        // Apagamos el LED si se detecta el ID específico
        digitalWrite(LED_PIN, LOW);
        if (EnEmergencia){
          EnEmergencia = false;
          suena = false;
        }else{
          controlPuerta(abre);
          delay(500);
          controlPuerta(cierra);

        }

      } else if (!checkCardID() && isNightTime() && isEmergencyActive) {
        // Encendemos el LED y activamos la alarma si se detecta movimiento en la noche
        activateAlarm();
      } else if (!EnEmergencia) {
        controlPuerta(abre);
        delay(500);
        controlPuerta(cierra);
      }
      // Terminamos la lectura de la tarjeta actual
      mfrc522.PICC_HaltA();
    }
  }

      //Serial.println(digitalRead(MOTION_PIN) );      

  // Verificamos si se detecta movimiento en el sensor y es de noche
  if (digitalRead(MOTION_PIN) == HIGH && isNightTime() && isEmergencyActive) {
    // Encendemos el LED y activamos la alarma si se detecta movimiento en la noche
      activateAlarm();
  } else {
    // Apagamos el LED si no se cumple la condición
    //digitalWrite(LED_PIN, LOW);
  }

  // verifica prende y apaga de sistema de seguridad
  if(BT.available())    // Si llega un dato por el puerto BT se envía al monitor serial
  {
    char valo = (char)BT.read();
    int valor = (int)valo;

    if(valor==49 || valor =="49"){
      //digitalWrite(LALED, HIGH);
      //Serial.println(valor);
      Serial.println("sistema de emergencia activo");      
      isEmergencyActive = true;
    }else{
      Serial.println("sistema de emergencia inactivo");
      //Serial.println(valor);
      //digitalWrite(LALED, LOW);
      isEmergencyActive = false;
    }

  }

  
}

void activateAlarm() {
  EnEmergencia = true;
  controlPuerta(cierra);
  digitalWrite(LED_PIN, HIGH);
  suena = true;
}

bool checkCardID() {
  // ID específico que deseas detectar (reemplaza con el ID de tu tarjeta)
  byte specificID[] = {0x9B, 0x48, 0x4A, 0x13};

  // Comparamos el ID de la tarjeta con el ID específico
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] != specificID[i]) {
      return false; // No coincide el ID
    }
  }
  return true; // Coincide el ID
}

bool isNightTime() {
  // Valor límite para considerar que es de noche (ajústalo según tus necesidades)
  int lightThreshold = 900;

  // Leemos el valor del sensor de luz
  int lightValue = analogRead(LIGHT_SENSOR_PIN);

  Serial.println(lightValue);
  // Comparamos el valor del sensor con el umbral para determinar si es de noche
  if (lightThreshold < lightValue) {
    return true; // Es de noche
  } else {
    return false; // No es de noche
  }
}

void emergencia(bool activar) {

  isEmergencyActive = activar; // Actualizamos el estado de la emergencia
  if (activar) {
    // Encendemos el LED y activamos el buzzer
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    // Apagamos el LED y el buzzer
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void controlPuerta(bool abrir) {
  if (abrir) {
    // Abrir la puerta
    puertaServo.write(90); // Establecer el ángulo del servo para abrir la puerta
    delay(1000); // Esperar 1 segundo para que la puerta se abra completamente (ajusta el tiempo según sea necesario)
  } else {
    // Cerrar la puerta
    puertaServo.write(0); // Establecer el ángulo del servo para cerrar la puerta
    delay(1000); // Esperar 1 segundo para que la puerta se cierre completamente (ajusta el tiempo según sea necesario)
  }
}


// Rutina de interrupción del comparador A del Timer 1
ISR(TIMER2_COMPA_vect) {
  Serial.println(suena);

  if (suena) {
    digitalWrite(BUZZER_PIN, !digitalRead(BUZZER_PIN));

    // // Función para activar la alarma (Buzzer y LED)
    // digitalWrite(BUZZER_PIN, HIGH);
    // delay(250); // Duración del sonido de la alarma
    // digitalWrite(BUZZER_PIN, LOW);

  }
}
