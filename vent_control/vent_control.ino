#include <Stepper.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <EEPROM.h>

#define seaLevelPressure_hPa 1013.25

Adafruit_BMP085 bmp;
const int STEPS = 2048;

SoftwareSerial bt(A0, A1); 

char command; 
int state = 0;  
int state1 = 0; 
int VentState = 0;   // 0 = Low Gear, 1 = High Gear
int EngineState = 0; // 0 = OFF, 1 = ON

// SAFETY CHECK: This tells us if it's safe to read the sensor
bool bmpWorking = false; 

Stepper stepper(STEPS, 10, 12, 11, 13);
Stepper stepper1(STEPS, 6, 8, 7, 9);    

void powerDownMotors() {
  for (int pin = 6; pin <= 13; pin++) { 
    digitalWrite(pin, LOW);
  }
}

void PowerUpVent() {
  if (EngineState == 1) { 
    // Fan is ON, check which gear to use
    if (VentState == 0) {
      digitalWrite(A3, LOW);  // Safety: ensure high gear is OFF
      delay(50);              // CRITICAL: Wait for SSR 2 to physically stop conducting
      digitalWrite(A2, HIGH); // Turn ON low gear
    } else if (VentState == 1) {
      digitalWrite(A2, LOW);  // Safety: ensure low gear is OFF
      delay(50);              // CRITICAL: Wait for SSR 1 to physically stop conducting
      digitalWrite(A3, HIGH); // Turn ON high gear
    }
  } else {
    // If EngineState is 0, ensure fan is OFF
    PowerDownVent();
  }
}

// Completely cuts power to the fan
void PowerDownVent() {
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  delay(10);
}

// Temporarily cuts power to prevent spikes while stepper motors start up
void PowerDownVentOnSwitch() {
  PowerDownVent();
  delay(50); // Give the relay time to actually shut off
}
void saveStateToMemory() {
  EEPROM.update(0, state);
  EEPROM.update(1, state1);
  EEPROM.update(2, VentState);
  EEPROM.update(3, EngineState);
}
void setup() {
  stepper.setSpeed(5); 
  stepper1.setSpeed(5);
  bt.begin(9600);
  Serial.begin(9600);
  
  pinMode(A3, OUTPUT);
  pinMode(A2, OUTPUT);
  
  if (!bmp.begin()) {
    Serial.println("BMP085 failed. Motors will still work.");
    bmpWorking = false; // Sensor failed, do not try to read it later!
  } else {
    bmpWorking = true;  // Sensor works!
  }

  // --- NEW: READ EEPROM ON BOOT ---
  // If EEPROM is uninitialized, it returns 255. 
  // If it's not 255, it means we have saved a state previously.
  if (EEPROM.read(0) != 255) {
    state       = EEPROM.read(0);
    state1      = EEPROM.read(1);
    VentState   = EEPROM.read(2);
    EngineState = EEPROM.read(3);
    
    // Apply the saved hardware state to the Fan automatically
    PowerUpVent(); 
  }
}

void loop() {
  if (bt.available()) {
    command = bt.read();
    if (command == '\n' || command == '\r') return; 

    if (command == '?') {
      bt.print(state);
      bt.print(state1);
      bt.print(VentState);
      bt.print(EngineState);
      
      if (bmpWorking) {
        bt.print(bmp.readTemperature());
      }
      
      return; 
    }        
  
    switch (command) {
      case '0': // Kabina
        PowerDownVentOnSwitch();
        stepper.step(STEPS / 4); 
        delay(10);
        state = 0; 
        powerDownMotors(); 
        PowerUpVent();
        saveStateToMemory();
        break;
        
      case '1': // Rack
        PowerDownVentOnSwitch();
        stepper.step(-STEPS / 4);
        delay(10);
        state = 1;
        powerDownMotors();
        PowerUpVent(); 
        saveStateToMemory();
        break; 
        
      case '2': // Wentylacja
        PowerDownVentOnSwitch();
        stepper1.step(-STEPS / 4); //znak został zamieniony na - ze względu na zły sposób odwracania się silnika wentylacyjnego (silnik został również zamieniony miejscami)
        delay(10);
        state1 = 0;
        powerDownMotors();
        PowerUpVent(); 
                saveStateToMemory();
        break;
        
      case '3': // Klimatyzacja
        PowerDownVentOnSwitch();
        stepper1.step(STEPS / 4); //znak został zamieniony na + ze względu na zły sposób odwracania się silnika wentylacyjnego (silnik został również zamieniony miejscami)
        delay(10);
        state1 = 1;
        powerDownMotors();
        PowerUpVent(); 
                saveStateToMemory();
        break;
        
      case '4': // Set High Gear and turn ON
        VentState = 1;
        PowerUpVent();
                saveStateToMemory();
        break;
        
      case '5': // Set Low Gear and turn ON
        VentState = 0;
        PowerUpVent();
                saveStateToMemory();
        break;
        
      case '6': // Turn Engine ON (leaves it in whatever gear was last used)
        EngineState = 1;
        PowerUpVent();
                saveStateToMemory();
        break;
        
      case '7': // Turn Engine OFF
        EngineState = 0;
        PowerDownVent();
                saveStateToMemory();
        break;

      // --- NEW: SAVE TO EEPROM COMMAND ---
      case '8': 
        // .update() only writes to memory if the value is different than what is already saved, saving EEPROM lifespan.
        EEPROM.update(0, state);
        EEPROM.update(1, state1);
        EEPROM.update(2, VentState);
        EEPROM.update(3, EngineState);
        break;
    }
  }
}
