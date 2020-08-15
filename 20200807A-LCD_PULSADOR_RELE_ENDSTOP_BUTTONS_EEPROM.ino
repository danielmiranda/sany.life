 /*
  Capitulo 35 de Arduino desde cero en Español.
  Simple programa que muestra en la pantalla de un modulo LCD 1602A mediante adaptador
  LCD I2C el tiempo transcurrido desde la ejecucion del mismo. Adaptador basado en
  circuito integrado PCF8574 y libreria LiquidCrystal_I2C descargada desde:
  https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/

  Autor: bitwiseAr  

*/

#include <Wire.h>     // libreria de comunicacion por I2C
#include <LCD.h>      // libreria para funciones de LCD
#include <LiquidCrystal_I2C.h>    // libreria para LCD por I2C

#include <EEPROM.h>
 
int eeAddress = 0;

LiquidCrystal_I2C lcd (0x27, 2, 1, 0, 4, 5, 6, 7); // DIR, E, RW, RS, D4, D5, D6, D7

long valor;
int valorNormalizado = 16;
long timestamp;

const int inputPinUp = 2; //pulsador
const int inputPinDown = 3; //pulsador

const int inputPin = 8; //pulsador
const int stopPin = 6; //endStop
int valorPulsador = 0;
int valorEndStop = 0;
int valorUp = 0;
int valorDown = 0;
int valorEncoder = 0; //valor del potenciometro
int valorNormalizadoOld = 0;
int steeper = 3; //valor de cambios de los pasos del pulsador
long now;
const int relePin = 7; //rele
bool pausa = false;
bool ejecutando = false;
bool ejecutandoGlobal = true;
long contador = 0; // en centesimas de segundo
long tick = 0; //contador de tiempo
long preMillis = 0;
int encoderPin = A0; //encoder

bool listo = false; //

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(inputPinUp, INPUT); //pin del boton de subir
    pinMode(inputPinDown, INPUT); //pin del boton de bajar    
    pinMode(inputPin, INPUT); //el pin que lee el pulsador es INPUT
    pinMode(stopPin, INPUT); //el pin que lee el endStop es INPUT    
    pinMode(relePin, OUTPUT);
    digitalWrite(relePin, HIGH);
    pinMode(encoderPin,INPUT);
    
    Serial.begin(9600);

    lcd.setBacklightPin(3,POSITIVE);  // puerto P3 de PCF8574 como positivo
    lcd.setBacklight(LOW);   // habilita iluminacion posterior de LCD
    lcd.begin(16, 2);     // 16 columnas por 2 lineas para LCD 1602A
    lcd.clear();      // limpia pantalla
    
    lcd.setCursor(0, 0);    // ubica cursor en columna 0 y linea 0
    lcd.print("-- SANY.LIFE --");  // escribe el texto
    delay(1000); //detemos por 1 segundo
    timestamp = millis();// + (100*100); //usamos un timestamp de 100 segundos en el futuro
    iniciarPantalla();
    preMillis = millis();

    digitalWrite(relePin, HIGH); //apagamos el RELE

    EEPROM.get( eeAddress, valorNormalizadoOld );
    Serial.println("valorNormalizadoOld");
    Serial.println(valorNormalizadoOld);
    if(valorNormalizadoOld > 0){
      valorNormalizado = valorNormalizadoOld;
    } else {
      Serial.println("valorNormalizado");
      Serial.println(valorNormalizado);
      EEPROM.put(eeAddress, valorNormalizado);
    }
}

void loop()
{
    delay(100); //ralentizamos el script para que no parpadee la pantalla

    // Leemos el final de carrera endStop
    valorEndStop = digitalRead(stopPin);  //lectura del endStop
    // Si no esta presionado el EndStop vamos cortar el RELE y saltar el LOOP
    if(valorEndStop == LOW)
    {
      //if(listo) {
      //  listo = false;
      //  lcd.setCursor(0,1);
      //  lcd.print("                 "); // Borramos la segunda linea
      //}
      isListo();
      
      lcd.setCursor(9,1);
      lcd.print("ABIERTA  ");
      
      //lcd.setCursor(5,1);
      //lcd.print("    ABIERTA  ");
      //ejecutando = false;
      //ejecutandoGlobal = false; //***********
      preMillis = millis();
      if(!pausa){
        digitalWrite(relePin, HIGH);
        delay(200);
      }

      return;
      
    } else {
      if(!ejecutando && !listo) {
        
        lcd.setCursor(0,1);
        lcd.print("                 "); // Borramos la segunda linea
      }
    }


    
    // Leemos el potenciometro
    //valor = analogRead(A0); //leemos el pin del potenciometro
    //valor = analogRead(encoderPin); //leemos el pin del potenciometro
    //valorEncoder = map(valor, 5, 1010, 1, 20)*3; //ponemos el valor del potenciometro entre 1 y 12 y multiplicamos + 5
    
    //if(valorEncoder != valorEncoderOld){
    //  valorNormalizado = valorEncoder;
    //  valorEncoderOld = valorEncoder;
    //}
    
    //valorEncoderOld = valorEncoder;
    
    now = (millis()-timestamp) / 1000;

    //valorNormalizado
    /* Leemos los pulsadores */
    valorUp = digitalRead(inputPinUp);
    valorDown = digitalRead(inputPinDown);

    /* Aqui obtenemos el valor de los pulsadores para aumentar o disminuir los segundos */
    if(valorUp == HIGH) {
      if(valorNormalizado+steeper <= 100) {
        valorNormalizado = valorNormalizado + steeper;
      } else {
        valorNormalizado = 100;
      }
      isListo();
    } 
    
    if(valorDown == HIGH) {
      if(valorNormalizado-steeper >= 3){
        valorNormalizado = valorNormalizado - steeper;
      } else {
        valorNormalizado = 3;
      }
      isListo();
    } 
    
    mostrarTiempoPantalla(valorNormalizado); //mostramos los segundos elegidos en la pantalla

    // Guardamos el dato en la EEPROM
    if(valorNormalizadoOld != valorNormalizado){
      //Serial.println("2.valorNormalizado");
      //Serial.println(valorNormalizado);
      //Serial.println("2.valorNormalizadoOld");
      //Serial.println(valorNormalizadoOld);
      EEPROM.update(eeAddress, valorNormalizado);
      valorNormalizadoOld = valorNormalizado;
    }
    
    if(ejecutando && !pausa){
      tick = millis() - preMillis;
      
      contador = contador + tick;
    }
    preMillis = millis();
    //console(contador);

    valorPulsador = digitalRead(inputPin);  //lectura del pulsador
    
    //Serial.println(valorEndStop);
    
    if (valorPulsador == HIGH) {
      
        if(ejecutando) { //Si se esta ejecutando un ciclo
            pausa = !pausa; //Lo pongo en pausa o saco la pausa
            delay(500);
        }
        
        if(!ejecutando) {
          iniciarPantalla();
          ejecutando = true; //activamos la ejecucion del reloj
          pausa = false;
          delay(500);
          timestamp = millis();
          contador = 0;
        }

    }
    


    if(ejecutando) {//si se ejecuta 
      if(!pausa){ //y no esta en pausa
          mostrarTiempoPantalla(valorNormalizado); //mostramos los segundos elegidos en la pantalla
          
          lcd.setCursor(0, 1);    // ubica cursor en columna 0 y linea 1
      
          // Ejecutamos esto mientras este corriendo el tiempo
          if((contador/1000) <=  valorNormalizado){
            lcd.print(valorNormalizado  - (contador/1000));   // funcion millis() / 1000 para segundos transcurridos
            lcd.print(" seg.");     // escribe seg.
            digitalWrite(relePin, LOW);
          } else {
            lcd.print("      LISTO    ");
            listo = true;
            digitalWrite(relePin, HIGH);
            ejecutando = false;
            pausa = false;
          }
          
          lcd.print("                 "); //borramos el resto de la linea con puntos
      
      } else { //en caso de estar en pausa
          //lcd.clear();
          lcd.setCursor(0,0);
          //lcd.print("      PAUSA     ");
          lcd.setCursor(9,1);
          lcd.print(" PAUSA    ");
          digitalWrite(relePin, HIGH);
          delay(100);
      }
    }
}

void iniciarPantalla() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    INICIANDO   ");
}

void mostrarTiempoPantalla(float tiempo){
   // Vamos a escribir el tiempo configurado con el potenciometro
  lcd.setCursor(0,0);
  lcd.print("      ");
  lcd.setCursor(2,0);
  lcd.print((int)tiempo); //Mostramos en la pantalla el valor del potenciometro -> hay que poner un rango
  lcd.setCursor(5,0);
  lcd.print(" Segundos        ");
}

void console(float tiempo){
   // Vamos a escribir el tiempo configurado con el potenciometro
  lcd.setCursor(10,1);
  lcd.print((int)tiempo); //Mostramos en la pantalla el valor del potenciometro -> hay que poner un rango
}

void isListo() {
    if(listo) {
      listo = false;
      lcd.setCursor(0,1);
      lcd.print("                 "); // Borramos la segunda linea
    }  
}
