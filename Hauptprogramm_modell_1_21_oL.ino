// Version 21
// Software ohne Lichtschrankenabfrage!!!
// Software Version in #define Software_version [XX] anpassen!
// Programm E-mail senden Funktion auskommentiert.
// LibreOffice speichern ist auskommentiert.
// Neu! LibreOffice wird vor dem Starten der Messung mit [strg + F3]
// in den Vordergrund geholt.
// 18.12.2018   -   22 Uhr
// Zeitenangabe erfolgt mit Zeitausgabe() in d,h,m,s
// Messungsanzahl wird mit jedem Programmwechsel auf 0 zurückgestellt
// Referenzfahrt und Korrekturfahrt HIGH und LOW abfolge geändert
// pinMode(STEP_PIN, OUTPUT); eingeführt Zeile 134


#include <AccelStepper.h>
#include <EEPROM.h>
#include <Keyboard.h>

#define Software_version 21

#define Schalter_4 A4            // 
#define Schalter_3 A3
#define Schalter_2 A2
#define Schalter_1 A1
#define Taster A0

#define Hall_Sensor 3
#define LED_Taster 9

#define Netzpin 10

#define EN_PIN   8 //enable pin
#define STEP_PIN 11 //step pin
#define DIR_PIN  12 //direction pin

int Messungsanzahl = 0;

int X = 300;
int delta_Pos, P1, P2, P3, test = 0;

bool schleife = LOW;

int LED_PWM = 0, Schalter_Stellung, Schalter_Stellung_lokal;

volatile bool Taster_kurz = LOW;
volatile bool Taster_lang = LOW;

bool Startbutton_result = LOW;
bool Programmier_Menue_aufrufen = LOW;
bool Kugel_oben = LOW;
bool Kugel_holen_start = true;
bool Testbeendet = false;
bool LED_DIMM = LOW;
bool Kugel_abgeworfen = false;
bool Fehler = LOW;
bool nach_Messung_speichern = LOW;
volatile bool Hall_Schleife_Aktiviert = LOW;
volatile bool Netzbetrieb = LOW;

char strgKey = KEY_LEFT_CTRL;     // Tastatur einrichten
char escKey = KEY_ESC;
char enterKey = KEY_RETURN;
char lBracket = {'*'};            // left round bracket (
char rBracket = {'('};            // right round bracket )
char hTips = {'@'};               // this is the "
char F3 = KEY_F3;

int Korrekturzaehler = 0;

int Rohrlaenge_mm = 0;

int Fahrweg_Referenz_ab_mm = 50;       // Alle Fahrwege in mm!
int Fahrweg_Referenz_auf_mm = 400;
int Fahrweg_Abwurf_auf_mm = -8;        // Achtung!!! Zahl muss negativ angegeben werden!

int Rohrlaenge_steps;
int Fahrweg_Referenz_ab_steps;
int Fahrweg_Referenz_auf_steps;
int Fahrweg_Abwurf_auf_steps;

unsigned long Dauer1 = 0,            // Zeiten in Millisekunden angeben!
              Intervall1 = 300000;   // Keine Intervallzeiten unter 60000! wird sonst zu eng mit dem gesamten Messvorgang
unsigned long Dauer2 = 0,            // Wenn LibreOffice abgespeichert aktiviert ist -> Keine Intervallzeiten unter 180000!
              Intervall2 = 1800000;
unsigned long Dauer3 = 0,
              Intervall3 = 7200000;
unsigned long Speicher_Intervall = 3600000;

unsigned long Schalter_Stellung_1,
         Schalter_Stellung_2,
         Schalter_Stellung_3,
         Schalter_Stellung_4,
         Schalter_Time,
         Schalter_Time_old,
         delta_Schalter_Time,
         StartupDauer = 0,
         StartupIntervall = 0,
         Startupdelta_Dauer = 0,
         Startupdelta_Intervall = 0,
         Startupdelta_t = 0,
         now,
         prenow = 0,
         Dauer = 0,
         Intervall = 0,
         delta_Dauer = 0,
         delta_Intervall = 0,
         delta_t = 0,
         delta_Blink = 0,
         pretime = 0,
         delta_Speicher_Intervall = 0,
         countdown = 0;





AccelStepper stepper(1, 11, 12); // pin 11 = step, pin 12 = direction

void setup() {


  Rohrlaenge_mm = EEprom_lesen();

  Fahrweg_berechnen();

  digitalWrite(A0, HIGH);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
  digitalWrite(A3, HIGH);
  digitalWrite(A4, HIGH);

  pinMode(EN_PIN, OUTPUT);                // ACHTUNG! Diesen Befehl bei Störung DEAKTIVIEREN!!
  digitalWrite(EN_PIN, LOW);              // ACHTUNG! Diesen Befehl Bei Störung DEAKTIVIEREN!!
  pinMode(STEP_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);
  pinMode(Hall_Sensor, INPUT);
  pinMode(LED_Taster, OUTPUT);
  digitalWrite(LED_Taster, 0);
  pinMode(Netzpin, INPUT);
  Serial.begin(9600);



  while (digitalRead(Netzpin) == LOW)
  {
    Serial.println("Bitte Netzteil einstecken");
    Serial.println();
    delay(1000);
  }
  Netzbetrieb = HIGH;


  stepper.setMaxSpeed(4000);
  stepper.setSpeed(2000);
  stepper.setAcceleration(2000);
  stepper.setEnablePin(8);
  stepper.setPinsInverted(true, false, true);

  stepper.disableOutputs();


  delay(1000);
  Serial.println("Um in das Setup Programm zu gelangen,");
  Serial.println("den [Taster] fuer min. 1,5s druecken");
  Serial.print("Countdown: "); Serial.println("10s");

  prenow = millis();
  countdown = 0;
  while (delta_t < 10000) {
    now = millis();
    if (now >= prenow) delta_t = delta_t + (now - prenow);
    if (now < prenow) delta_t = delta_t + (4294967295 - prenow) + now;
    prenow = now;

    if (delta_t >= (countdown + 1000)) {
      countdown = countdown + 1000;
      Serial.print("            "); Serial.print((10000 - countdown) / 1000); Serial.print("s"); Serial.print("\n");
    }

    Tasterabfrage(1500);
    if (Taster_lang == HIGH) {
      Programmier_Menue();
      delta_t = 10000;
      Taster_lang = LOW;
    }

  }

  Serial.println();
  Serial.println("Setup beendet, Programm wird gestartet.");
  Serial.print("Die verwendete Softwareversion: "); Serial.println(Software_version);

  Referenzfahrt();

  stepper.setCurrentPosition(0);
  Schalter_Time_old = millis();
  if (Kugel_oben == LOW) Kugel_holen();
}
void loop() {

  Schalter_Time = millis();
  if (Schalter_Time >= Schalter_Time_old) delta_Schalter_Time = (Schalter_Time - Schalter_Time_old);
  if (Schalter_Time < Schalter_Time_old)  delta_Schalter_Time = (4294967295 - Schalter_Time_old) + Schalter_Time;
  if (delta_Schalter_Time > 200) delta_Schalter_Time = 200;
  Schalter_Time_old = Schalter_Time;


  if (analogRead(A1) < 400) {
    Schalter_Stellung_1 = Schalter_Stellung_1 + delta_Schalter_Time;
    Schalter_Stellung_2 = 0;
    Schalter_Stellung_3 = 0;
    Schalter_Stellung_4 = 0;
    if (Schalter_Stellung_1 > 1000) {
      Schalter_Stellung_1 = 1001;
      Schalter_Stellung = 1;
      Messungsanzahl = 0;
      Programmausgabe();
      //      Serial.println();
      //      Serial.println("Gewaehltes Programm Nr: 1");
      //      Serial.println("Manueller Start.");
      Programm_Manuell();
    }
  }

  if (analogRead(A2) < 400) {
    Schalter_Stellung_2 = Schalter_Stellung_2 + delta_Schalter_Time;
    Schalter_Stellung_1 = 0;
    Schalter_Stellung_3 = 0;
    Schalter_Stellung_4 = 0;

    if (Schalter_Stellung_2 > 1000) {
      Schalter_Stellung = 2;
      Dauer = Dauer1;
      Intervall = Intervall1;
      delta_Dauer = 0;
      delta_Intervall = 0;
      delta_Speicher_Intervall = 0;
      delta_t = 0;
      Messungsanzahl = 0;
      Programmausgabe();
      Programm_Automatik();
    }
  }

  if (analogRead(A3) < 400) {
    Schalter_Stellung_3 = Schalter_Stellung_3 + delta_Schalter_Time;
    Schalter_Stellung_1 = 0;
    Schalter_Stellung_2 = 0;
    Schalter_Stellung_4 = 0;


    if (Schalter_Stellung_3 > 1000) {
      //  Serial.println();
      //  Serial.println("Wahlschalter in Stellung 3");
      Schalter_Stellung = 3;
      Dauer = Dauer2;
      Intervall = Intervall2;
      delta_Dauer = 0;
      delta_Intervall = 0;
      delta_Speicher_Intervall = 0;
      delta_t = 0;
      Messungsanzahl = 0;
      Programmausgabe();
      Programm_Automatik();
    }
  }

  if (analogRead(A4) < 400) {

    Schalter_Stellung_4 = Schalter_Stellung_4 + delta_Schalter_Time;
    Schalter_Stellung_1 = 0;
    Schalter_Stellung_2 = 0;
    Schalter_Stellung_3 = 0;

    if (Schalter_Stellung_4 > 1000) {
      //  Serial.println();
      // Serial.println("Wahlschalter in Stellung 4");
      Schalter_Stellung = 4;
      Dauer = Dauer3;
      Intervall = Intervall3;
      delta_Dauer = 0;
      delta_Intervall = 0;
      delta_Speicher_Intervall = 0;
      delta_t = 0;
      Messungsanzahl = 0;
      Programmausgabe();
      Programm_Automatik();
    }
  }

}
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Programmier_Menue() {
  int i = 0;
  bool Status;
  digitalWrite(LED_Taster, 255);
  do { } while (analogRead(A0) < 400); // Warten bis Taster wieder gelöst;
  digitalWrite(LED_Taster, 0);
  delay(50);   //Entprellen
  Serial.println();
  Serial.println("###   Setup Modus   ###");
  Serial.println("zum verlassen -> [Taster] druecken!");
  Serial.println();
  Serial.print("Die verwendete Softwareversion: "); Serial.println(Software_version);
  Serial.print("Die gespeicherte Rohrlaenge betraegt: "); Serial.println(EEprom_lesen());
  Serial.print("bisher korrigierte Schritte: "); Serial.println(Korrekturzaehler);
  Serial.println();
  Serial.println(" - #S -> Softwareversion ausgeben -");
  Serial.println(" - #K -> Kugel holen              -");
  Serial.println(" - #A -> Kugel auswerfen          -");
  Serial.println(" - #R -> Referenzfahrt ausfuehren -");
  Serial.println();
  Serial.println("Rohrlaenge Parameter: #Lxxx  {xxx : dreistellige Zahl in [mm]}");

  while (analogRead(A0) > 800) {
    i++;
    if ((i > 1500) && (Status == HIGH)) {
      Status = LOW;
      i = 0;
    }
    if ((i > 1500) && (Status == LOW)) {
      Status = HIGH;
      i = 0;
    }
    digitalWrite(LED_Taster, Status);
    Seriell_einlesen();
  }
  do { } while (analogRead(A0) < 400); // Warten bis Taster wieder gelöst;
  delay(50);   //Entprellen
  Rohrlaenge_mm = EEprom_lesen();
  Fahrweg_berechnen();
  digitalWrite(LED_Taster, 0);
  Serial.println();
  Serial.println("###   Setup Modus verlassen -> zurueck im Programm   ###");
  Serial.println();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

void Programm_Manuell() {
  Schalter_Stellung_lokal = Schalter_Stellung;


  while (analogRead(Schalter_Stellung_lokal) < 400) {   // Schalterstellung 1: Manueller Abwurf
    if (Kugel_oben == LOW) Kugel_holen();

    Startbefehl_abfragen();
    if (analogRead(Schalter_Stellung_lokal) < 400) {      // wenn Taster gedrÃ¼ckt wird und Wahlschalter auf 1
      analogWrite(LED_Taster, 255);

      Kugel_abwerfen();

      digitalWrite(LED_Taster, 0);
    }

    digitalWrite(LED_Taster, 0);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Programm_Automatik() {
  int  i = 0;
  Schalter_Stellung_lokal = Schalter_Stellung;

  analogWrite(LED_Taster, 0);
  // Serial.println();
  // Serial.print("Die gewaehlte Intervallzeit: ");
  // Zeitausgabe(Intervall);
  // Serial.println();
  // if (Dauer == 0) Serial.println("Endlosmessung gewaehlt");
  // else {
  //   Serial.print("Die Gesamte Messdauer betraegt: ");
  //   Zeitausgabe(Dauer);
  //   Serial.println();
  // }
  delay(1000);
  //  Kugel_holen();
  if (Kugel_oben == LOW) Kugel_holen();

  Startbefehl_abfragen();

  prenow = millis();
  analogWrite(LED_Taster, 255);



  // if (analogRead(A0) < 400) {
  // Kugel_abwerfen();
  //   delay(2000);
  if (Taster_kurz == HIGH)  Kugel_abwerfen();

  //    delay(2000);
  //    Email_senden();
  // }

  digitalWrite(LED_Taster, 0);

  //  if (Intervall <= Speicher_Intervall) nach_Messung_speichern = LOW;
  //  else nach_Messung_speichern = HIGH;

  while ((delta_Dauer <= Dauer) && (analogRead(Schalter_Stellung_lokal) < 400)) {
    now = millis();
    if (now >= prenow) delta_t = (now - prenow);
    if (now < prenow)  delta_t = (4294967295 - prenow) + now;
    prenow = now;
    if (Dauer != 0) delta_Dauer = delta_Dauer + delta_t;
    delta_Intervall = delta_Intervall + delta_t;
    delta_Blink = delta_Blink + delta_t;

    //   if (nach_Messung_speichern == LOW) {
    //      delta_Speicher_Intervall = delta_Speicher_Intervall + delta_t;

    //      if (delta_Speicher_Intervall >= Speicher_Intervall) {
    //        nach_Messung_speichern = HIGH;
    //        delta_Speicher_Intervall = 0;
    //      }
    //    }
    // if (analogRead(A0) < 400) {
    Tasterabfrage(1500);
    if (Taster_lang == HIGH) {
      Programmausgabe();
      Serial.print("Zeit bis zur naechsten Messung: ");
      Zeitausgabe(Intervall - delta_Intervall);
      Serial.println();
      Serial.println();
      //Taster_lang == LOW;
    }



    if (Kugel_oben == LOW) {
      Kugel_holen();
      delta_Blink = 0;
    }

    if (delta_Blink >= 2000) {
      while (i < Schalter_Stellung_lokal - 1)
      {
        i++;
        analogWrite(LED_Taster, 255);
        delay(50);
        analogWrite(LED_Taster, 0);
        if (Schalter_Stellung_lokal > 2) delay(200);
      }
      i = 0;
      delta_Blink = 0;
    }

    if (delta_Intervall >= Intervall) {
      Serial.print("Programm Nr: ");
      Serial.println(Schalter_Stellung_lokal - 1);
      Serial.println();
      Serial.print("Intervallzeit ist abgelaufen: ");
      Zeitausgabe(Intervall);
      Serial.println();
      analogWrite(LED_Taster, 255);
      Kugel_abwerfen();
      //    Email_senden();
      delay(2000);

      //      if (nach_Messung_speichern == HIGH) {
      //        delay(30000);
      //        Libreoffice_speichern();
      //        if (Intervall <= Speicher_Intervall) nach_Messung_speichern = LOW;
      //      }

      analogWrite(LED_Taster, 0);
      delta_Intervall = 0;
      delta_Blink = 0;

    }

  }
  //if (delta_Dauer >= Dauer) Testende();
  Serial.println();
  Serial.print("Messreihe beendet: ");
  Zeitausgabe(Dauer);
  Serial.println();
  Serial.println("-----------------------------------------");
  Schalter_Stellung_1 = 0;
  Schalter_Stellung_2 = 0;
  Schalter_Stellung_3 = 0;
  Schalter_Stellung_4 = 0;

  //}
}
////////////////////////////////////////////////////////////////////////////////////////////////////

void Startbefehl_abfragen()
{
  Taster_kurz = LOW;
  Taster_lang = LOW;
  digitalWrite(LED_Taster, 0);
  LED_PWM = 0;

  do { } while (analogRead(A0) < 400); // Warten bis Taster wieder gelöst;
  delay(50);   //Entprellen

  while ((analogRead(Schalter_Stellung_lokal) < 400) && (Taster_kurz == LOW)) {    // solang Taster nicht gedrÃ¼ckt und Wahlschalter auf 1
    if (LED_DIMM == LOW) LED_PWM++;          // LED_Taster auf und ab dimmen..
    else LED_PWM--;                          //
    analogWrite(LED_Taster, LED_PWM);        //
    delay(3);                                //
    if (LED_PWM >= 255) LED_DIMM = HIGH;     //
    if (LED_PWM <= 0) LED_DIMM = LOW;        //

    Tasterabfrage(1500);
    if (Taster_lang == HIGH) {
      Programmausgabe();
    }
  }

  digitalWrite(LED_Taster, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

void Kugel_holen()
{
  int count = 0;
  /////////////////////////////??? Richtige Position???

  count++;
  stepper.setMaxSpeed(4000);
  stepper.enableOutputs();
  stepper.runToNewPosition(Rohrlaenge_steps);
  delay(500);
  stepper.runToNewPosition(6000);
  stepper.setMaxSpeed(1000);
  stepper.runToNewPosition(0);
  stepper.disableOutputs();
  delay(500);
  Serial.print("angefahrene Stempelposition: "); Serial.println(stepper.currentPosition());
  if (digitalRead(Hall_Sensor) == HIGH) Korrekturfahrt();
  Serial.print("bisher korrigierte Schritte: ");
  Serial.println(Korrekturzaehler);
  Serial.println();
  Serial.println("-------------------------------------------------");
  Serial.println();
  Kugel_oben = HIGH;
}



////////////////////////////////////////////////////////////////////////////////////////////////////

void Kugel_abwerfen()
{
  Keyboard.begin();
  delay(1000);
  Serial.println();
  Serial.println("Die Messung wurde gestartet und die Kugel wird abgeworfen.");
  Messungsanzahl++;
  Libre_Office_onTop();
  delay(1000);
  Messung_starten();
  delay(3000);
  stepper.setMaxSpeed(4000);
  stepper.enableOutputs();
  stepper.runToNewPosition(Fahrweg_Abwurf_auf_steps);
  stepper.disableOutputs();
  Serial.print("bisher durchgefuehrte Messungen: "); Serial.println(Messungsanzahl);
  delay(15000);
  Messung_speichern();
  //digitalWrite(LED_Taster, 255);
  Kugel_oben = LOW;

}

///////////////////////////////////////////////////////////////////////////////////////////////////

void do_Hall_Sensor()
{
  Hall_Schleife_Aktiviert = HIGH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void do_Netzteil_off()
{
  Netzbetrieb = LOW;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void Messung_starten()
{
  Keyboard.press(strgKey);    // Sendet Messung Starten als Keyboard
  Keyboard.press('6');        //
  delay(100);                   //
  Keyboard.releaseAll();        //
  Serial.println("Messung starten.");
}
////////////////////////////////////////////////////////////////////////////////////////////////////

void Messung_speichern()
{
  Keyboard.press(strgKey);      // Sendet Messung speichern als Keyboard
  Keyboard.press('7');          //
  delay(100);                   //
  Keyboard.releaseAll();        //
  Serial.println("Messung speichern.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Email_senden()
{
  Keyboard.press(strgKey);      // Sendet E-mail senden als Keyboard
  Keyboard.press('8');          //
  delay(100);                   //
  Keyboard.releaseAll();        //
  Serial.println("E-mail senden.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////

//void Libreoffice_speichern()
//{
//  Keyboard.press(strgKey);      // Speichert in Libre-Office mit Keyboard ab
//  Keyboard.press('s');          //
//  delay(100);                   //
//  Keyboard.releaseAll();        //
//  Serial.println("Libre-Office abgespeichert!");
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
void Fehlermeldung_bestaetigen()
{
  bool Status;
  int i = 0;
  do { } while (analogRead(A0) < 400); // Warten bis Taster wieder gelöst;
  delay(50);   //Entprellen
  digitalWrite(LED_Taster, 0);
  while (analogRead(A0) > 800) {    // solang Taster nicht gedrÃ¼ckt
    i++;
    if ((i > 500) && (Status == HIGH)) {
      Status = LOW;
      i = 0;
    }
    if ((i > 500) && (Status == LOW)) {
      Status = HIGH;
      i = 0;
    }
    digitalWrite(LED_Taster, Status);
  }
  Fehler = LOW;

}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Fehler_blockiert()
{
  Fehlermeldung_bestaetigen();
  Referenzfahrt();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

void Korrekturfahrt()
{
  int i;
  Hall_Schleife_Aktiviert = LOW;
  digitalWrite(EN_PIN, LOW);
  digitalWrite(DIR_PIN, HIGH); //set the direction: high
  i = 0;
  attachInterrupt(digitalPinToInterrupt(3), do_Hall_Sensor, LOW);
  delay(500); //wait 1000ms
  Hall_Schleife_Aktiviert = LOW;
  while (((Hall_Schleife_Aktiviert == LOW) && (i <= Fahrweg_Referenz_auf_steps)) || i > 1000) //iterate for 32000 steps
  {
    i++;
    Korrekturzaehler++;
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(400); //wait 2ms
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(400); //wait 2ms
  }
  if (i >= 1000) Fehler_blockiert();
  detachInterrupt(digitalPinToInterrupt(3));
  digitalWrite(EN_PIN, HIGH);

  stepper.setCurrentPosition(0);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void Referenzfahrt()
{
  Serial.println();
  Serial.println("Referenzfahrt wird ausgefuehrt");
  int i;
  Hall_Schleife_Aktiviert = LOW;
  digitalWrite(EN_PIN, LOW);
  digitalWrite(DIR_PIN, LOW); //set the direction: low

  for (i = 0; i < Fahrweg_Referenz_ab_steps; i++) //iterate for 3200 steps
  {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(300); //wait 2ms
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(300); //wait 2ms
  }



  digitalWrite(DIR_PIN, HIGH); //set the direction: high
  i = 0;
  attachInterrupt(digitalPinToInterrupt(3), do_Hall_Sensor, LOW);
  delay(500);
  Hall_Schleife_Aktiviert = LOW;
  while ((Hall_Schleife_Aktiviert == LOW) && (i <= Fahrweg_Referenz_auf_steps)) //iterate for 32000 steps
  {
    i++;
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(400); //wait 2ms
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(400); //wait 2ms
  }
  detachInterrupt(digitalPinToInterrupt(3));
  digitalWrite(EN_PIN, HIGH);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void Seriell_einlesen() {
  char ch;

  if (Serial.available() > 0) {
    ch = Serial.read();
    delay(2);


    if (ch == '#') {
      ch = Serial.read();
      delay(2);
      if (ch == 'L') {
        Rohrlaenge_einlesen();
      }
      if (ch == 'S') {
        Serial.println(); // Softwarenummer Seriell ausgeben
        Serial.print("Die verwendete Softwareversion: "); Serial.println(Software_version);
        Serial.println();
      }

      if (ch == 'K') Kugel_holen(); // Funktion Kugel holen auslösen
      if (ch == 'R') Referenzfahrt(); // Funktion Referenzfahrt auslösen
      if (ch == 'A') Kugel_auswerfen();


    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Rohrlaenge_einlesen() {
  char ch;
  int stelle = 0, wert = 0;
  for (int stelle = 0; stelle < 3; stelle++) {
    ch = Serial.read();
    delay(2);
    if (isDigit(ch)) wert = (wert * 10) + (ch - '0');
  }
  Serial.print("Rohrlaenge = ");
  Serial.print(wert);
  Serial.println(" mm");

  EEprom_schreiben(wert);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void EEprom_schreiben(int rohrlaenge)  //
{ //
  int wert_1, wert_2;
  int gelesen;

  gelesen = EEprom_lesen();
  Serial.println();
  Serial.print("Dieser Wert steht bereits im EEprom: "); Serial.println(gelesen);


  if (gelesen != rohrlaenge) {
    Serial.println("Dieser Wert wurde in das EEprom geschrieben: ");
    Serial.print(rohrlaenge);

    if (rohrlaenge > 255) {              //
      wert_1 = 255;                      //
      wert_2 = rohrlaenge - 255;         //
    }                                    //
    if (rohrlaenge < 256) {              //
      wert_1 = rohrlaenge;               //
      wert_2 = 0;                        //
    }                                    //
    EEPROM.write(0, wert_1);             //
    EEPROM.write(1, wert_2);             //
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////

int EEprom_lesen() {    //
  int wert_1, wert_2;
  int rohrlaenge;
  wert_1 = EEPROM.read(0);             //
  wert_2 = EEPROM.read(1);             //
  rohrlaenge = wert_1 + wert_2;
  return rohrlaenge;//
}                                      //

////////////////////////////////////////////////////////////////////////////////////////////////

void Fahrweg_berechnen()
{
  Rohrlaenge_steps = Rohrlaenge_mm * 80 + 300;
  Fahrweg_Referenz_ab_steps = Fahrweg_Referenz_ab_mm * 80;
  Fahrweg_Referenz_auf_steps = Fahrweg_Referenz_auf_mm * 80;
  Fahrweg_Abwurf_auf_steps = Fahrweg_Abwurf_auf_mm * 80;
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Libre_Office_onTop()
{
  Keyboard.press(strgKey);    // Sendet [Strg] + [F3] um LibreOffice zu starten
  Keyboard.press(F3);           // bzw. in den Vordergrund zu holen.
  delay(100);                       //
  Keyboard.releaseAll();            //
  Serial.println();
  Serial.println("LibreOffice im Vordergrund!");
}

////////////////////////////////////////////////////////////////////////////////////////////////

void Zeitausgabe(unsigned long Zeit_in_ms)
{

  long rest_nach_tagen = 0,
       tage = 0,
       rest_nach_stunden = 0,
       stunden = 0,
       rest_nach_minuten = 0,
       minuten = 0,
       rest_nach_sekunden = 0,
       sekunden = 0;

  if (Zeit_in_ms >= 86400000) {
    rest_nach_tagen = Zeit_in_ms % 86400000;
    tage = (Zeit_in_ms - rest_nach_tagen) / 86400000;
    Serial.print(tage);
    Serial.print(" Tag(e)");
    Zeit_in_ms = rest_nach_tagen;
    if (Zeit_in_ms > 0) Serial.print(", ");
  }
  if (Zeit_in_ms >= 3600000) {
    rest_nach_stunden = Zeit_in_ms % 3600000;
    stunden = (Zeit_in_ms - rest_nach_stunden) / 3600000;
    Serial.print(stunden);
    Serial.print(" h");
    Zeit_in_ms = rest_nach_stunden;
    if (Zeit_in_ms > 0) Serial.print(" : ");
  }
  if (Zeit_in_ms >= 60000) {
    rest_nach_minuten = Zeit_in_ms % 60000;
    minuten = (Zeit_in_ms - rest_nach_minuten) / 60000;
    Serial.print(minuten);
    Serial.print(" min.");
    Zeit_in_ms = rest_nach_minuten;
    if (Zeit_in_ms > 0) Serial.print(" : ");
  }
  if (Zeit_in_ms >= 1) {
    rest_nach_sekunden = Zeit_in_ms % 1000;
    sekunden = (Zeit_in_ms - rest_nach_sekunden) / 1000;
    Serial.print(sekunden);
    Serial.print(",");
    if (rest_nach_sekunden < 100) Serial.print("0");
    if (rest_nach_sekunden < 10) Serial.print("0");
    Serial.print(rest_nach_sekunden);
    Serial.print(" sec.");
  }
}

////////////////////////////////////////////////////////////////////

void Tasterabfrage(int Taster_Schaltzeit_total)

{

  unsigned long now_2 = 0, prenow_2 = 0, Taster_Schaltzeit = 0, Taster_Schaltzeit_delta_t = 0;

  Taster_kurz = LOW;
  Taster_lang = LOW;
  prenow_2 = millis();

  while (analogRead(A0) < 400)
  {
    digitalWrite(LED_Taster, 0);
    now_2 = millis();
    if (now_2 >= prenow_2) Taster_Schaltzeit_delta_t = (now_2 - prenow_2);
    if (now_2 < prenow_2)  Taster_Schaltzeit_delta_t = (4294967295 - prenow_2) + now_2;
    prenow_2 = now_2;
    Taster_Schaltzeit = Taster_Schaltzeit + Taster_Schaltzeit_delta_t;
    
    if (Taster_Schaltzeit >= Taster_Schaltzeit_total) {
      analogWrite(LED_Taster, 255);
      Taster_lang = HIGH;
    }

  }



  //do { } while (analogRead(A0) < 400); // Warten bis Taster wieder gelöst; Evtl. diesen Teil in Programmier_Menue einfügen
  //    delay(50);   //Entprellen



  //      Programmier_Menue();
  //      Taster_kurz = LOW;
  if ((Taster_Schaltzeit > 100) && (Taster_lang == LOW)) Taster_kurz = HIGH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void Programmausgabe()
{
  if (Schalter_Stellung == 1) {
    Serial.println();
    Serial.println("Gewaehltes Programm Nr: 1");
    Serial.println("Manueller Start.");
  }
  else {
    Serial.println();
    Serial.print("Gewaehltes Programm Nr: "); Serial.println(Schalter_Stellung);
    Serial.print("Die Intervallzeit: ");
    Zeitausgabe(Intervall);
    Serial.println();
    if (Dauer == 0) Serial.println("Die Messungen werden in Endlosschleife ausgefuehrt.");
    else {
      Serial.print("Die Gesamte Messdauer betraegt: ");
      Zeitausgabe(Dauer);
      Serial.println();
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void Kugel_auswerfen()

{
  stepper.setMaxSpeed(4000);
  stepper.enableOutputs();
  stepper.runToNewPosition(Fahrweg_Abwurf_auf_steps);
  stepper.disableOutputs();
}

