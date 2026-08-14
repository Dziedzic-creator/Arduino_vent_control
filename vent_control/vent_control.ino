#include <Stepper.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
//definicja ilości kroków w pełnym obrocie
const int STEPS = 2048;
//definicja portów RX i TX
SoftwareSerial bt(A0, A1); 

//zmienna globalna zajmująca sie odbieraną wiadomością przez HC-05
char command; 

//stany początkowe
int state = 0;  
int state1 = 0; 
int VentState= 0;
int EngineState=0;
//inicjalizacja silników z definicją ich portów służących do kontroli
Stepper stepper(STEPS, 10, 12, 11, 13);
Stepper stepper1(STEPS, 6, 8, 7, 9);    


//funkcja pomocnicza
void powerDownMotors() {
  for (int pin = 9; pin <= 13; pin++) {
    digitalWrite(pin, LOW);
  }
  // Also power down the Analog-used pins
}
void PowerUpVent() {
  if(VentState==0)
    digitalWrite(A4,HIGH);//niski bieg
  else if(VentState==1)
    digitalWrite(A5,HIGH);//wysoki bieg
}
void PowerDownVent() {
  digitalWrite(A4,LOW);
  digitalWrite(A5,LOW);
}
//inicjalizacja całego systemu i rozpoczęcie obioru i wysyłu
void setup() {
  stepper.setSpeed(5); //prędość silników
  stepper1.setSpeed(5);
  bt.begin(9600);
  Serial.begin(9600);
}

//główna pętla programu
void loop() {

  if (bt.available()) {//dba o brak działania w razie braku podłączonego urządzenia
    command = bt.read();
    if (command == '\n' || command == '\r') return; 

    if (command == '?') {
      bt.print(state);
      bt.print(state1);
      bt.print(VentState);
      return;
    }

    switch (command) {
      case '0': // Kabina
        PowerDownVent();
        stepper.step(STEPS / 4); //obrót o 90*
        delay(10);
        state = 0; 
        powerDownMotors(); // Release power after move
        break;
        
      case '1': // Rack
        PowerDownVent();
        stepper.step(-STEPS / 4);//obrót o -90*
        delay(10);
        state = 1;
        powerDownMotors(); // Release power after move
        break; 
        
      case '2': // Wentylacja
        PowerDownVent();
        stepper1.step(STEPS / 4);
        delay(10);
        state1 = 0;
        powerDownMotors(); // Release power after move
        break;
        
      case '3': // Klimatyzacja
        PowerDownVent() ;
        stepper1.step(-STEPS / 4);//obrót o -90*
        delay(10);
        state1 = 1;
        powerDownMotors(); // Release power after move
        break;
        
      case '4':
        VentState=1;
        PowerUpVent();
        break;
      case '5':
        VentState=0;
        PowerUpVent();
        break;
      case '6':
        PowerUpVent();
        break;
        case '7':
        PowerDownVent();
        break;
    }
  }
}