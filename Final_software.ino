/*                
             
           To Do
- TIMER:
  - Helligkeitseinbruch nach Vibrationsstart beheben

- LERNMODUS

- AKKU-ANZEIGE:
  - das Überschreiben beheben, nur ein Mal in der Minute aktualisieren

- DATUMS-ANZEIGE:
  - das korrekte Datum anzeigen
*/




#include "config.h"
#include <Wire.h>



TTGOClass *ttgo;
TFT_eSPI *tft;
BMA *sensor;
AXP20X_Class *power;

int16_t x, y;

bool irq = false;
uint32_t targetTime = 0;       // for next 1 second timeout
byte omm = 99;
boolean initial = 1;
byte xcolon = 0;
unsigned int colour = 0;
static uint8_t conv2d(const char *p)
{
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time


char buf[128];
bool rtcIrq = false;

//Flags
bool flag1 = false;
bool flag2 = false;
bool flag3 = false;
bool flag4 = false;
bool flag5 = false;
bool flag6 = true;
bool flag7 = true;
bool flag8 = false;
bool flag9 = false;
bool flag10 = false;
bool flag11 = false;
bool flag12 = false;

bool startfenster_aktiviert = true;
bool lauffenster_aktiviert = false;
bool sudokufenster_aktiviert = false;
bool timerfenster_aktiviert = false;

//Bildschirm-settings
bool bildschirm_touched = false;
bool display_sleep = false;
bool display_wokeup = true;
unsigned long zeitpunkt_touching;
unsigned long kontrollzeitpunkt;

//Laufmodus-Settings
unsigned long laufzeit;
unsigned long startzeit;
unsigned long endzeit;
int gewicht = 55; //Default-Gewicht in kg
int vorherige_schritte = 0;
float cal;
bool sos_aktiviert = false;
unsigned long schritte_kontroll_millis = 0;
unsigned long schritte_actual_millis;


//Timer-Settings
unsigned long start_millis;
unsigned long actual_millis;
unsigned long timer_starten;
unsigned long timer_stoppen;
unsigned long verbleibende_millis;
unsigned long verbleibende_millis_2 = 0;
unsigned long verbleibende_millis_3 = 0;
unsigned long verbleibende_millis_4 = 0;
unsigned long timer_anzeige_millis;
bool timer_gestartet = false;
bool timer_gestoppt = false;
int timer_zeit_minuten = 0;
int timer_zeit_zehnerminuten = 0;
int timer_zeit_stunden = 0;
int timer_zeit_zehnerstunden = 0;
int timer_milliseconds = 0;


bool timer_zehnerstunden_aktiviert = false;
bool timer_zehnerstunden_minus_aktiviert = false;
bool timer_einerstunden_aktiviert = false;
bool timer_einerstunden_minus_aktiviert = false;
bool timer_zehnerminuten_aktiviert = false;
bool timer_zehnerminuten_minus_aktiviert = false;
bool timer_einerminuten_aktiviert = false;
bool timer_einerminuten_minus_aktiviert = false;

bool timer_stunden_aktiviert = false;
bool timer_zehnerminuten = false;
bool timer_minuten = false;

bool motor_started = false;

//Memory-Settings
bool sudoku_eingabe_aktiviert = true;
bool level_chosen = false;
bool game_started = false;
int level;
int cycles = 0;
int reihenfolge[50];
long memoryfield;
long checkfield;
int rounds = 1;
bool play = true;
bool reading = false;

void setup()
{
  ttgo = TTGOClass::getWatch();
  ttgo->begin();
  ttgo->openBL();
  tft = ttgo->tft;
  sensor = ttgo->bma;
  power = ttgo->power;

  // Accel parameter structure
  Acfg cfg;
  /*!
      Output data rate in Hz, Optional parameters:
          - BMA4_OUTPUT_DATA_RATE_0_78HZ
          - BMA4_OUTPUT_DATA_RATE_1_56HZ
          - BMA4_OUTPUT_DATA_RATE_3_12HZ
          - BMA4_OUTPUT_DATA_RATE_6_25HZ
          - BMA4_OUTPUT_DATA_RATE_12_5HZ
          - BMA4_OUTPUT_DATA_RATE_25HZ
          - BMA4_OUTPUT_DATA_RATE_50HZ
          - BMA4_OUTPUT_DATA_RATE_100HZ
          - BMA4_OUTPUT_DATA_RATE_200HZ
          - BMA4_OUTPUT_DATA_RATE_400HZ
          - BMA4_OUTPUT_DATA_RATE_800HZ
          - BMA4_OUTPUT_DATA_RATE_1600HZ
  */
  cfg.odr = BMA4_OUTPUT_DATA_RATE_50HZ;
  /*!
      G-range, Optional parameters:
          - BMA4_ACCEL_RANGE_2G
          - BMA4_ACCEL_RANGE_4G
          - BMA4_ACCEL_RANGE_8G
          - BMA4_ACCEL_RANGE_16G
  */
  cfg.range = BMA4_ACCEL_RANGE_2G;
  /*!
      Bandwidth parameter, determines filter configuration, Optional parameters:
          - BMA4_ACCEL_OSR4_AVG1
          - BMA4_ACCEL_OSR2_AVG2
          - BMA4_ACCEL_NORMAL_AVG4
          - BMA4_ACCEL_CIC_AVG8
          - BMA4_ACCEL_RES_AVG16
          - BMA4_ACCEL_RES_AVG32
          - BMA4_ACCEL_RES_AVG64
          - BMA4_ACCEL_RES_AVG128
  */
  cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

  /*! Filter performance mode , Optional parameters:
      - BMA4_CIC_AVG_MODE
      - BMA4_CONTINUOUS_MODE
  */
  cfg.perf_mode = BMA4_CONTINUOUS_MODE;

  // Configure the BMA423 accelerometer
  sensor->accelConfig(cfg);

  // Enable BMA423 accelerometer
  // Warning : Need to use steps, you must first enable the accelerometer
  // Warning : Need to use steps, you must first enable the accelerometer
  // Warning : Need to use steps, you must first enable the accelerometer
  sensor->enableAccel();

  pinMode(BMA423_INT1, INPUT);
  attachInterrupt(BMA423_INT1, [] {
    // Set interrupt to set irq value to 1
    irq = 1;
  }, RISING); //It must be a rising edge

  // Enable BMA423 step count feature
  sensor->enableFeature(BMA423_STEP_CNTR, true);

  // Reset steps
  sensor->resetStepCounter();

  // Turn on step interrupt
  sensor->enableStepCountInterrupt();


  Serial.begin(115200);
}

void loop()
{
  //******************************* STARTFENSTER *********************************//
  while (startfenster_aktiviert == true) {
    tft->setTextFont(2);
    tft->setTextColor(TFT_WHITE);
    tft->setCursor(5,5);
    tft->print(power->getBattPercentage());
    tft->println(" %");
    schlafmodus();
    if (flag1 == false) {
      flag1 = true;
      tft->fillScreen(TFT_BLACK);
      //Lauf-Modus-Ellipse//
      tft->fillEllipse(170, 170, 40, 20, TFT_WHITE);
      tft->drawEllipse(170, 170, 40, 20, TFT_WHITE);
      tft->setTextColor(TFT_BLACK);
      tft->drawString("Joggen", 145, 163, 2);

      //Timer-Ellipse//
      tft->fillEllipse(70, 170, 40, 20, TFT_WHITE);
      tft->drawEllipse(70, 170, 40, 20, TFT_WHITE);
      tft->setTextColor(TFT_BLACK);
      tft->drawString("Timer", 55, 163, 2);

      //Memory-Ellipse//
      tft->fillEllipse(120, 210, 40, 20, TFT_WHITE);
      tft->drawEllipse(120, 210, 40, 20, TFT_WHITE);
      tft->setTextColor(TFT_BLACK);
      tft->drawString("Memory", 98, 203, 2);

      //Clock
      targetTime = millis() + 1000;
      //ttgo->rtc->setDateTime(2020, 22, 8, 17, 19, 00);  Kalibrating the Real Time Clock
    }

    //Digital-Uhr
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%s", ttgo->rtc->formatDateTime());
    tft->drawString(buf, 5, 70, 7);
    ttgo->tft->setCursor (90, 133);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    ttgo->tft->print(__DATE__); // This uses the standard ADAFruit small font
    tft->setTextColor(TFT_BLACK);


    //Laufmodus-Aktivierung//
    if (ttgo->getTouch(x, y)) {
      if (x > 130 && x < 210 && y > 150 && y < 190) {
        tft->fillEllipse(170, 170, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillEllipse(170, 170, 40, 20, TFT_WHITE);
        tft->setTextColor(TFT_BLACK);
        tft->drawString("Joggen", 145, 163, 2);
        startfenster_aktiviert = false;
        lauffenster_aktiviert = true;
        break;
      }
    }

    //Timer-Aktivierung//
    if (ttgo->getTouch(x, y)) {
      if (x > 30 && x < 110 && y > 150 && y < 190) {
        tft->fillEllipse(70, 170, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillEllipse(70, 170, 40, 20, TFT_WHITE);
        tft->drawString("Timer", 55, 163, 2);
        startfenster_aktiviert = false;
        timerfenster_aktiviert = true;
        break;
      }
    }

    
    //Sudoku-Aktivierung//
    if (ttgo->getTouch(x, y)) {
      if (x > 80 && x < 160 && y > 190 && y < 230) {
        tft->fillEllipse(120, 210, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillEllipse(120, 210, 40, 20, TFT_WHITE);
        tft->setTextColor(TFT_BLACK);
        tft->drawString("Lernen", 98, 203, 2);
        startfenster_aktiviert = false;
        sudokufenster_aktiviert = true;
        break;
      }
    } 
  }


  //******************************* LAUFFENSTER *********************************//
  while (lauffenster_aktiviert == true) {
    schritte_actual_millis = millis();
    uint32_t step = sensor->getCounter();
    schlafmodus();
    if (flag2 == false) {
      flag2 = true;
      flag1 = false;

      //Laufstart-Button//
      tft->fillScreen(TFT_BLACK);
      tft->fillEllipse(70, 70, 50, 50, TFT_WHITE);
      tft->drawEllipse(70, 70, 50, 50, TFT_WHITE);
      tft->setTextColor(TFT_BLACK);
      tft->drawString("START", 34, 60, 4);

      //Main-Button//
      tft->drawRect(197, 8, 40, 20, TFT_YELLOW);
      tft->setTextColor(TFT_WHITE);
      tft->drawString("Main", 204, 10, 2);

      /*//SOS-Button//
      tft->fillEllipse(190, 70, 20, 20, TFT_RED);
      tft->drawEllipse(190, 70, 20, 20, TFT_WHITE);
      tft->setTextColor(TFT_WHITE);
      tft->drawString("SOS", 179, 63, 2);*/

      //Striche
      tft->fillRect(0, 5, 150, 3, TFT_WHITE);
      tft->fillRect(150, 5, 3, 150, TFT_WHITE);
      tft->fillRect(142, 5, 3, 150, TFT_WHITE);
      tft->fillRect(0, 152, 250, 3, TFT_WHITE);
      tft->fillRect(150,50,100,3,TFT_WHITE);
      tft->fillRect(150,70,100,3,TFT_WHITE);
      tft->setTextColor(TFT_WHITE);
      tft->drawString("SCHRITTE", 167, 55, 2);
    }

    //Zurück zum Startfenster
    if (ttgo->getTouch(x, y)) {
      if (x > 160 && x < 240 && y < 25) {
        tft->fillRect(197, 8, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillRect(197, 8, 40, 20, TFT_BLACK);
        tft->setTextColor(TFT_WHITE);
        tft->drawString("Main", 204, 10, 2);
        lauffenster_aktiviert = false;
        startfenster_aktiviert = true;
        flag2 = false;
        break;
      }
    }

    //Laufstart
    if (ttgo->getTouch(x, y)) {  //Wenn Display überhaupt berührt...
      if (x > 20 && x < 120 && y > 20 && y < 120 && flag5 == false && display_wokeup == true) { //.. und wenn dabei "Start" gedrückt und der Display auch an...
        flag5 = true;
        flag6 = false;
        tft->fillEllipse(70, 70, 50, 50, TFT_GREEN);
        delay(150);
        tft->fillEllipse(70, 70, 50, 50, TFT_WHITE);
        tft->setTextColor(TFT_BLACK);
        tft->drawString("STOP", 36, 60, 4);
        sensor->resetStepCounter();   //Fange jetzt an Schritte zu zählen
        startzeit = millis();
      }
    }

    if (flag5 == true && schritte_actual_millis - schritte_kontroll_millis >= 5000){   //Wenn der Start-Knopt gedrückt wurde und 5 Sekunden vergangen sind...
      schritte_kontroll_millis = schritte_actual_millis;
      tft->fillRect(160, 100, 70, 50, TFT_BLACK);
      tft->setTextFont(4);
      tft->setTextColor(TFT_WHITE);
      tft->setCursor(160, 100);
      tft->print(step);
      Serial.println("Step: ");
      Serial.print(step);
    }

    //Laufstop
    if (ttgo->getTouch(x, y)) {    //Wenn der Display berührt wurde...
      if (x > 20 && x < 120 && y > 20 && y < 120 && flag6 == false && display_wokeup == true) { //...wenn dabei "Stop" gedrückt und Display an
        flag6 = true;
        flag5 = false;
        tft->fillEllipse(70, 70, 50, 50, TFT_GREEN);
        delay(150);
        tft->fillEllipse(70, 70, 50, 50, TFT_WHITE);
        tft->setTextColor(TFT_BLACK);
        tft->drawString("START", 34, 60, 4);
        //uint32_t step = sensor->getCounter();
        tft->drawRect(10, 162, 220, 73, TFT_WHITE);
        tft->setTextColor(TFT_WHITE);

        endzeit = millis();
        laufzeit = endzeit - startzeit;

        //Display-Ausgaben
        tft->setTextFont(2);
        tft->setCursor(14, 170);
        tft->print("GELAUFEN: ");
        tft->print(step * 0.9);
        tft->print(" m = ");
        tft->print((step * 0.9) / 1000);
        tft->print(" km");
        tft->setCursor(14, 185);
        tft->print("LAUFZEIT: ");
        tft->print(laufzeit / 1000);
        tft->print(" s = ");
        tft->print((laufzeit / 1000) / 60);
        tft->print(" h");
        tft->setCursor(14, 200);
        tft->print("SPEED: ");
        tft->print((step * 0.9) / (laufzeit / 1000));
        tft->print(" m/s = ");
        tft->print(1/(((step * 0.9) / (laufzeit / 1000)) * 3.6)*60);
        tft->print(" min/km");
        tft->setCursor(14, 215);
        tft->print("UMSATZ: ");
        cal = gewicht * (step * 0.9);
        tft->print(cal / 1000);
        tft->print(" kcal");
      }
    }    
  }



  //******************************* TIMERFENSTER *********************************//
  while (timerfenster_aktiviert == true) {
    if (flag3 == false) {
      flag3 = true;
      flag1 = false;

      tft->fillScreen(TFT_BLACK);

      //Main-Button//
      tft->drawRect(197, 8, 40, 20, TFT_YELLOW);
      tft->setTextColor(TFT_WHITE);
      tft->drawString("Main", 204, 10, 2);

      //Timer-Rechteck//
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);

      //Start-Button//
      tft->fillEllipse(25,25,22,20, TFT_WHITE);
      tft->setTextFont(2);
      tft->setTextColor(TFT_BLACK);
      tft->setCursor(7, 18);
      tft->print("START");
      tft->setTextColor(TFT_WHITE);

      //Timer-Buttons//
      tft->fillEllipse(46, 90, 15, 15, TFT_RED); //Plus-Knopf-10er Stunden
      tft->fillRect(37,90, 20, 2, TFT_WHITE);
      tft->fillRect(46,82, 2, 18, TFT_WHITE);

      tft->fillEllipse(46, 190, 15, 15, TFT_RED); //Minus-Knopf-10er Stunden
      tft->fillRect(37,190, 20, 2, TFT_WHITE);

      tft->fillEllipse(86, 90, 15, 15, TFT_RED); //Plus-Knopf-1er Stunden
      tft->fillRect(77,90, 20, 2, TFT_WHITE);
      tft->fillRect(86,82, 2, 18, TFT_WHITE);

      tft->fillEllipse(86, 190, 15, 15, TFT_RED); //Minus-Knopf-1er Stunden
      tft->fillRect(77,190, 20, 2, TFT_WHITE);

      tft->fillEllipse(151, 90, 15, 15, TFT_RED); //Plus-Knopf-10er Minuten
      tft->fillRect(142,90, 20, 2, TFT_WHITE);
      tft->fillRect(151,82, 2, 18, TFT_WHITE);

      tft->fillEllipse(151, 190, 15, 15, TFT_RED); //Minus-Knopf-10er Minuten
      tft->fillRect(142,190, 20, 2, TFT_WHITE);

      tft->fillEllipse(191, 90, 15, 15, TFT_RED); //Plus-Knopf-1er Minuten
      tft->fillRect(182,90, 20, 2, TFT_WHITE);
      tft->fillRect(191,82, 2, 18, TFT_WHITE);

      tft->fillEllipse(191, 190, 15, 15, TFT_RED); //Minus-Knopf-1er Minuten
      tft->fillRect(182,190, 20, 2, TFT_WHITE);

      //Timer-Anzeige
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten);
    }

    //Timer-Einstellung
    if (ttgo->getTouch(x, y)) {  //Aktivierung Zehnerstunden-Plus-Knopf
      if (x > 11 && x < 61 && y > 75 && y < 105 && display_wokeup == true && timer_zehnerstunden_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_zehnerstunden_aktiviert = true;
      tft->fillEllipse(46, 90, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(46, 90, 15, 15, TFT_RED);
      tft->fillRect(37,90, 20, 2, TFT_WHITE);
      tft->fillRect(46,82, 2, 18, TFT_WHITE);
      if (timer_zeit_zehnerstunden<=9){
        timer_zeit_zehnerstunden+=1;
      }
      if (timer_zeit_zehnerstunden == 10){
        timer_zeit_zehnerstunden = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_zehnerstunden_aktiviert = false;
    }

    if (ttgo->getTouch(x, y)) {  //Aktivierung Zehnerstunden-Minus-Knopf
      if (x > 11 && x < 61 && y > 175 && y < 205 && display_wokeup == true && timer_zehnerstunden_minus_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_zehnerstunden_minus_aktiviert = true;      
      tft->fillEllipse(46, 190, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(46, 190, 15, 15, TFT_RED);
      tft->fillRect(37,190, 20, 2, TFT_WHITE);
      if (timer_zeit_zehnerstunden>=0){
        timer_zeit_zehnerstunden-=1;
      }
      if (timer_zeit_zehnerstunden < 0){
        timer_zeit_zehnerstunden = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_zehnerstunden_minus_aktiviert = false;
    }

    if (ttgo->getTouch(x, y)) {  //Aktivierung Einerstunden-Plus-Knopf
      if (x > 71 && x < 101 && y > 75 && y < 105 && display_wokeup == true && timer_einerstunden_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_einerstunden_aktiviert = true;
      tft->fillEllipse(86, 90, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(86, 90, 15, 15, TFT_RED);
      tft->fillRect(77,90, 20, 2, TFT_WHITE);
      tft->fillRect(86,82, 2, 18, TFT_WHITE);
      if (timer_zeit_stunden<=9){
        timer_zeit_stunden+=1;
      }
      if (timer_zeit_stunden == 10){
        timer_zeit_stunden = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_einerstunden_aktiviert = false;
    }    


    if (ttgo->getTouch(x, y)) {  //Aktivierung Einerstunden-Minus-Knopf
      if (x > 71 && x < 101 && y > 175 && y < 205 && display_wokeup == true && timer_einerstunden_minus_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_einerstunden_minus_aktiviert = true;
      tft->fillEllipse(86, 190, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(86, 190, 15, 15, TFT_RED);
      tft->fillRect(77,190, 20, 2, TFT_WHITE);
      if (timer_zeit_stunden>=0){
        timer_zeit_stunden-=1;
      }
      if (timer_zeit_stunden < 0){
        timer_zeit_stunden = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_einerstunden_minus_aktiviert = false;
    }


    if (ttgo->getTouch(x, y)) {  //Aktivierung Zehnerminuten-Plus-Knopf
      if (x > 136 && x < 166 && y > 75 && y < 105 && display_wokeup == true && timer_zehnerminuten_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_zehnerminuten_aktiviert = true;
      tft->fillEllipse(151, 90, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(151, 90, 15, 15, TFT_RED);
      tft->fillRect(142,90, 20, 2, TFT_WHITE);
      tft->fillRect(151,82, 2, 18, TFT_WHITE);
      if (timer_zeit_zehnerminuten<=9){
        timer_zeit_zehnerminuten+=1;
      }
      if (timer_zeit_zehnerminuten == 10){
        timer_zeit_zehnerminuten = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_zehnerminuten_aktiviert = false;

    }

    if (ttgo->getTouch(x, y)) {  //Aktivierung Zehnerminuten-Minus-Knopf
      if (x > 136 && x < 166 && y > 175 && y < 205 && display_wokeup == true && timer_zehnerminuten_minus_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_zehnerminuten_minus_aktiviert = true;
      tft->fillEllipse(151, 190, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(151, 190, 15, 15, TFT_RED);
      tft->fillRect(142,190, 20, 2, TFT_WHITE);
      if (timer_zeit_zehnerminuten>=0){
        timer_zeit_zehnerminuten-=1;
      }
      if (timer_zeit_zehnerminuten < 0){
        timer_zeit_zehnerminuten = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_zehnerminuten_minus_aktiviert = false;
    }

    if (ttgo->getTouch(x, y)) {  //Aktivierung Einerminuten-Plus-Knopf
      if (x > 176 && x < 206 && y > 75 && y < 105 && display_wokeup == true && timer_einerminuten_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_einerminuten_aktiviert = true;
      tft->fillEllipse(191, 90, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(191, 90, 15, 15, TFT_RED);
      tft->fillRect(182,90, 20, 2, TFT_WHITE);
      tft->fillRect(191,82, 2, 18, TFT_WHITE);
      if (timer_zeit_minuten<=9){
        timer_zeit_minuten+=1;
      }
      if (timer_zeit_minuten == 10){
        timer_zeit_minuten = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_einerminuten_aktiviert = false;
    }
      
    if (ttgo->getTouch(x, y)) {  //Aktivierung Einerminuten-Minus-Knopf
      if (x > 176 && x < 206 && y > 175 && y < 205 && display_wokeup == true && timer_einerminuten_minus_aktiviert == false) { //.. und wenn dabei " 10er-Stunden-+ " gedrückt und der Display auch an...
      timer_einerminuten_minus_aktiviert = true;
      tft->fillEllipse(191, 190, 15, 15, TFT_GREEN);
      delay(150);
      tft->fillEllipse(191, 190, 15, 15, TFT_RED);
      tft->fillRect(182,190, 20, 2, TFT_WHITE);
      if (timer_zeit_minuten>=0){
        timer_zeit_minuten-=1;
      }
      if (timer_zeit_minuten < 0){
        timer_zeit_minuten = 0;
      }
      }
      //Timer-Anzeige
      tft->fillRect(32, 111, 176, 59, TFT_BLACK);
      tft->drawRect(31, 110, 177, 60, TFT_WHITE);
      tft->setTextFont(6);
      tft->setCursor(35, 120);
      tft->print(timer_zeit_zehnerstunden);
      tft->print(" ");
      tft->print(timer_zeit_stunden);
      tft->print(" ");
      tft->print(":");
      tft->print(" ");
      tft->print(timer_zeit_zehnerminuten);
      tft->print(" ");
      tft->print(timer_zeit_minuten); 
    }
    else{
      timer_einerminuten_minus_aktiviert = false;
    }
    //Ab hier nachträgliche Umstellung: Minuten = Sekunden und Stunden = Minuten
    timer_milliseconds = timer_zeit_minuten*60000/60 + timer_zeit_zehnerminuten*10*60000/60 + timer_zeit_stunden*60*60000/60 + timer_zeit_zehnerstunden*10*60*60000/60;   //Millisekunden, die insgesamt heruntergezählt werden soll
    Serial.println("Timer_milliseconds: ");
    Serial.print(timer_milliseconds);
    //Aktivierung des Timers - Start des Herunterzählens//
    if (ttgo->getTouch(x, y)) {
      if (x > 0 && x < 50 && y > 0 && y < 50) {
        tft->fillEllipse(25,25,22,20, TFT_GREEN);
        delay(150);
        tft->fillEllipse(25,25,22,20, TFT_WHITE);
        tft->setTextFont(2);
        tft->setTextColor(TFT_BLACK);
        tft->setCursor(10, 18);
        tft->print("Reset");
        tft->setTextColor(TFT_WHITE);
        flag7 = true;
        while(flag7 == true){   //flag7 = false, wenn Abbruch des Herunterzählens oder Pausierung
          if (flag8 == false){
            timer_starten = millis();
            timer_stoppen = timer_starten + timer_milliseconds;
            timer_anzeige_millis = millis();
            flag8 = true;
          }
          if (flag10 == false){
              flag10 = true;
              start_millis = millis();
            }
          actual_millis = millis();
          if (actual_millis < timer_stoppen){          //Herunterzählen
            timer_anzeige_millis = actual_millis;
            verbleibende_millis = timer_stoppen - actual_millis;
            timer_zeit_zehnerstunden = verbleibende_millis/(36000000/60);
            verbleibende_millis_2 = verbleibende_millis - timer_zeit_zehnerstunden*36000000/60;
            timer_zeit_stunden = verbleibende_millis_2/(3600000/60);
            verbleibende_millis_3 = verbleibende_millis_2 - timer_zeit_stunden*3600000/60;
            timer_zeit_zehnerminuten = verbleibende_millis_3/(600000/60);
            verbleibende_millis_4 = verbleibende_millis_3 - timer_zeit_zehnerminuten*600000/60;
            timer_zeit_minuten = verbleibende_millis_4 / (60000/60);
            Serial.println(timer_zeit_zehnerstunden);
            Serial.print(" ");
            Serial.print(timer_zeit_stunden);
            Serial.print(" : ");
            Serial.print(timer_zeit_zehnerminuten);
            Serial.print(" ");
            Serial.print(timer_zeit_minuten);
            if((actual_millis-start_millis)>= 1000){
              start_millis = actual_millis;
              tft->fillRect(32, 111, 176, 59, TFT_BLACK);
              tft->drawRect(31, 110, 177, 60, TFT_WHITE);
              tft->setTextFont(6);
              tft->setCursor(35, 120);
              tft->print(timer_zeit_zehnerstunden);
              tft->print(" ");
              tft->print(timer_zeit_stunden);
              tft->print(" ");
              tft->print(":");
              tft->print(" ");
              tft->print(timer_zeit_zehnerminuten);
              tft->print(" ");
              tft->print(timer_zeit_minuten); 
              }
          }
         if (actual_millis >= timer_stoppen && flag9 == false){    //Vibrationsmotor starten
            flag9 = true;
            motor_started = true;
            tft->fillRect(32, 111, 176, 59, TFT_BLACK);
            tft->drawRect(31, 110, 177, 60, TFT_WHITE);
            tft->setTextFont(4);
            tft->fillEllipse(25,25,22,20, TFT_RED);
            tft->setCursor(37, 130);
            tft->setTextColor(TFT_RED);
            tft->print("ABGELAUFEN");
            tft->setTextFont(2);
            tft->setTextColor(TFT_WHITE);
            tft->setCursor(12, 18);
            tft->print("Done");
            ttgo->motor_begin();
          }

          //Stoppen des Vibrationsmotors//
  
          if (motor_started == true){
            if (actual_millis%300 == 0){
              ttgo->motor->onec();
            }
            if (ttgo->getTouch(x, y)) {
              if (x > 0 && x < 50 && y > 0 && y < 50) {
                tft->fillEllipse(25,25,22,20, TFT_GREEN);
                delay(150);
                tft->fillEllipse(25,25,22,20, TFT_WHITE);
                tft->setTextFont(2);
                tft->setTextColor(TFT_BLACK);
                tft->setCursor(10, 18);
                tft->print("Start");
                verbleibende_millis = 0;
                verbleibende_millis_2 = 0;
                verbleibende_millis_3 = 0;
                verbleibende_millis_4 = 0;
                timer_anzeige_millis = 0;
                timer_zeit_minuten = 0;
                timer_zeit_zehnerminuten = 0;
                timer_zeit_stunden = 0;
                timer_zeit_zehnerstunden = 0;
                timer_milliseconds = 0; 
                tft->fillRect(32, 111, 176, 59, TFT_BLACK);
                tft->drawRect(31, 110, 177, 60, TFT_WHITE);
                tft->setTextFont(6);
                tft->setTextColor(TFT_WHITE);
                tft->setCursor(35, 120);
                tft->print(timer_zeit_zehnerstunden);
                tft->print(" ");
                tft->print(timer_zeit_stunden);
                tft->print(" ");
                tft->print(":");
                tft->print(" ");
                tft->print(timer_zeit_zehnerminuten);
                tft->print(" ");
                tft->print(timer_zeit_minuten);
                motor_started = false;
                flag7 = false;
                flag8 = false;
                flag9 = false;
                flag10 = false;
               }
            } 
          }
          
         //Reset
         if (ttgo->getTouch(x, y)) {
          if (x > 0 && x < 50 && y > 0 && y < 50) {
            tft->fillEllipse(25,25,22,20, TFT_GREEN);
            delay(150);
            tft->fillEllipse(25,25,22,20, TFT_WHITE);
            tft->setTextFont(2);
            tft->setTextColor(TFT_BLACK);
            tft->setCursor(10, 18);
            tft->print("Start");
            verbleibende_millis = 0;
            verbleibende_millis_2 = 0;
            verbleibende_millis_3 = 0;
            verbleibende_millis_4 = 0;
            timer_anzeige_millis = 0;
            timer_zeit_minuten = 0;
            timer_zeit_zehnerminuten = 0;
            timer_zeit_stunden = 0;
            timer_zeit_zehnerstunden = 0;
            timer_milliseconds = 0; 
            tft->fillRect(32, 111, 176, 59, TFT_BLACK);
            tft->drawRect(31, 110, 177, 60, TFT_WHITE);
            tft->setTextFont(6);
            tft->setTextColor(TFT_WHITE);
            tft->setCursor(35, 120);
            tft->print(timer_zeit_zehnerstunden);
            tft->print(" ");
            tft->print(timer_zeit_stunden);
            tft->print(" ");
            tft->print(":");
            tft->print(" ");
            tft->print(timer_zeit_zehnerminuten);
            tft->print(" ");
            tft->print(timer_zeit_minuten);
            flag7 = false;
            flag8 = false;
            flag9 = false;
            flag10 = false;
            motor_started = false;
            break;        
            }
           }
         
         //Zurück zum Startfenster 
         if (ttgo->getTouch(x, y)) {
          if (x > 160 && x < 240 && y < 25) {
            tft->fillRect(197, 8, 40, 20, TFT_GREEN);
            delay(150);
            tft->fillRect(197, 8, 40, 20, TFT_BLACK);
            tft->setTextColor(TFT_WHITE);
            tft->drawString("Main", 204, 10, 2);
            timer_zeit_minuten = 0;
            timer_zeit_zehnerminuten = 0;
            timer_zeit_stunden = 0;
            timer_zeit_zehnerstunden = 0;
            timerfenster_aktiviert = false;
            startfenster_aktiviert = true;
            flag3 = false;
            flag8 = false;
            flag7 = true;
            flag9 = false;
            flag10 = false;
            motor_started = false;
            break;
            }
          }
        schlafmodus();
        }
      }
    }
    //Zurück zum Startfenster
    if (ttgo->getTouch(x, y)) {
      if (x > 160 && x < 240 && y < 25) {
        tft->fillRect(197, 8, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillRect(197, 8, 40, 20, TFT_BLACK);
        tft->setTextColor(TFT_WHITE);
        tft->drawString("Main", 204, 10, 2);
        timer_zeit_minuten = 0;
        timer_zeit_zehnerminuten = 0;
        timer_zeit_stunden = 0;
        timer_zeit_zehnerstunden = 0;
        timerfenster_aktiviert = false;
        startfenster_aktiviert = true;
        flag3 = false;
        flag8 = false;
        flag7 = true;
        break;
      }
    }

    schlafmodus();
  }

  //******************************* SUDOKUFENSTER *********************************//
  while (sudokufenster_aktiviert == true) {
    if (flag4 == false) {
      flag4 = true;
      flag1 = false;
      tft->fillScreen(TFT_BLACK);
      
      //Main-Button//
      tft->drawRect(197, 8, 40, 20, TFT_YELLOW);
      tft->setTextColor(TFT_WHITE);
      tft->drawString("Main", 204, 10, 2);
      
      //Schwierigkeitsauswahl//
      tft->fillRect(65, 70, 120, 4, TFT_WHITE);
      tft->drawString("LEVEL", 87, 40, 4);
      tft->drawEllipse(50, 120, 20, 20, TFT_WHITE);
      tft->drawEllipse(120, 120, 20, 20, TFT_WHITE);
      tft->drawEllipse(190, 120, 20, 20, TFT_WHITE);
      tft->drawString("1", 43, 111, 4);
      tft->drawString("2", 113, 111, 4);
      tft->drawString("3", 183, 111, 4);

      //Spielstart//
      tft->fillEllipse(120, 200, 60, 30, TFT_WHITE);
      tft->setTextColor(TFT_BLACK);
      tft->drawString("START", 82, 191, 4);
    }
        
    //Zurück zum Startfenster//
    if (ttgo->getTouch(x, y)) {
      if (x > 160 && x < 240 && y < 25) {
        tft->fillRect(197, 8, 40, 20, TFT_GREEN);
        delay(150);
        tft->fillRect(197, 8, 40, 20, TFT_BLACK);
        tft->setTextColor(TFT_WHITE);
        tft->drawString("Main", 204, 10, 2);
        sudokufenster_aktiviert = false;
        startfenster_aktiviert = true;
        flag4 = false;
        level_chosen = false;
        game_started = false;
        break;
      }
    }

    //Level-Auswahl//
    if (level_chosen == false){
      if (ttgo->getTouch(x, y)) {
        if (x > 30 && x < 70 && y < 140 && y > 100) {
          tft->fillEllipse(50, 120, 20, 20, TFT_GREEN);
          tft->setTextColor(TFT_WHITE);
          tft->drawString("1", 43, 111, 4);
          level = 1;
          level_chosen = true;
        }

        else if (x > 100 && x < 140 && y < 140 && y > 100) {
          tft->fillEllipse(120, 120, 20, 20, TFT_GREEN);
          tft->setTextColor(TFT_WHITE);
          tft->drawString("2", 113, 111, 4);
          level = 2;
          level_chosen = true;
        }

        else if (x > 170 && x < 210 && y < 140 && y > 100) {
          tft->fillEllipse(190, 120, 20, 20, TFT_GREEN);
          tft->setTextColor(TFT_WHITE);
          tft->drawString("3", 183, 111, 4);
          level = 3;
          level_chosen = true;
        }
      }

    }

    //Spielstart//
    if (game_started == false && level_chosen == true){
      if (ttgo->getTouch(x, y)) {
        if (x > 60 && x < 180 && y < 230 && y > 170) {
          tft->fillEllipse(120, 200, 60, 30, TFT_GREEN);
          delay(150);
          tft->fillEllipse(120, 200, 60, 30, TFT_WHITE);
          tft->setTextColor(TFT_BLACK);
          tft->drawString("START", 82, 191, 4);
          game_started = true;
        }
      }
    }

    while(game_started==true){
      if (flag12 == false){
        flag12 = true;
        reihenfolge[0]=random(1,5);
        
        //Spielbrett
        tft->fillScreen(TFT_BLACK);
        tft->drawRect(10,10,80,80, TFT_WHITE);
        tft->drawRect(90,90,80,80, TFT_WHITE);
        tft->drawRect(90,10,80,80, TFT_WHITE);
        tft->drawRect(10,90,80,80, TFT_WHITE);

        //Main-Button//
        tft->drawRect(197, 8, 40, 20, TFT_YELLOW);
        tft->setTextColor(TFT_WHITE);
        tft->drawString("Main", 204, 10, 2);
        delay(2000);}

        if (play == true){
          for (int i = 0; i<=rounds;){
            memoryfield = reihenfolge[i];
            reihenfolge[i+1] = random(1,5);
            Serial.println(memoryfield);
            if (memoryfield == 1){
              tft->fillRect(10,10,80,80, TFT_GREEN);
              delay(1000/level);
              tft->fillRect(10,10,80,80, TFT_BLACK);
              tft->drawRect(10,10,80,80, TFT_WHITE);
              delay(1000/level);
              i+=1;
              }
            if (memoryfield == 3){
              tft->fillRect(90,90,80,80, TFT_GREEN);
              delay(1000/level);
              tft->fillRect(90,90,80,80, TFT_BLACK);
              tft->drawRect(90,90,80,80, TFT_WHITE);
              delay(1000/level);
              i+=1;
              }
            if (memoryfield == 2){
              tft->fillRect(90,10,80,80, TFT_GREEN);
              delay(1000/level);
              tft->fillRect(90,10,80,80, TFT_BLACK);
              tft->drawRect(90,10,80,80, TFT_WHITE);
              delay(1000/level);
              i+=1;
              }
            if (memoryfield == 4){
              tft->fillRect(10,90,80,80, TFT_GREEN);
              delay(1000/level);
              tft->fillRect(10,90,80,80, TFT_BLACK);
              tft->drawRect(10,90,80,80, TFT_WHITE);
              delay(1000/level);
              i+=1;
              }
          }
          play = false;
          reading = true;
        }
         if (reading == true){
          for (int i = 0; i<=rounds;){
            if (ttgo->getTouch(x, y)) {
              if (x < 80 && y < 80) {
                checkfield = 1;
                Serial.println(checkfield);
                tft->fillRect(10,10,80,80, TFT_GREEN);
                delay(200);
                tft->fillRect(10,10,80,80, TFT_BLACK);
                tft->drawRect(10,10,80,80, TFT_WHITE);
                delay(200);
                if (reihenfolge[i] != checkfield){
                  tft->fillScreen(TFT_RED);
                  tft->setTextColor(TFT_WHITE);
                  tft->drawString("FALSCH :(", 100, 100, 4);
                  delay(1000);
                  flag4 = false;
                  level_chosen = false;
                  game_started = false;
                  flag12 = false;
                  sudokufenster_aktiviert = true;
                  rounds = 1;
                  break;}
                i+=1;              
              }
              if (x < 170 && x>80 && y < 80) {
                checkfield = 2;
                Serial.println(checkfield);
                tft->fillRect(90,10,80,80, TFT_GREEN);
                delay(200);
                tft->fillRect(90,10,80,80, TFT_BLACK);
                tft->drawRect(90,10,80,80, TFT_WHITE);
                delay(200);
                if (reihenfolge[i] != checkfield){
                  tft->fillScreen(TFT_RED);
                  tft->setTextColor(TFT_WHITE);
                  tft->drawString("FALSCH :(", 100, 100, 4);
                  delay(1000);
                  flag4 = false;
                  level_chosen = false;
                  game_started = false;
                  flag12 = false;
                  sudokufenster_aktiviert = true;
                  rounds = 1;
                  break;}
                i+=1;
              
              }
              if (x > 80 && y > 80) {
                checkfield = 3;
                Serial.println(checkfield);
                tft->fillRect(90,90,80,80, TFT_GREEN);
                delay(200);
                tft->fillRect(90,90,80,80, TFT_BLACK);
                tft->drawRect(90,90,80,80, TFT_WHITE);
                delay(200);
                if (reihenfolge[i] != checkfield){
                  tft->fillScreen(TFT_RED);
                  tft->setTextColor(TFT_WHITE);
                  tft->drawString("FALSCH :(", 100, 100, 4);
                  delay(1000);
                  flag4 = false;
                  level_chosen = false;
                  game_started = false;
                  flag12 = false;
                  sudokufenster_aktiviert = true;
                  rounds = 1;
                  break;}
                  i+=1;
              
              }
              if (x > 10 && x < 90 && y > 90) {
                checkfield = 4;
                Serial.println(checkfield);
                tft->fillRect(10,90,80,80, TFT_GREEN);
                delay(200);
                tft->fillRect(10,90,80,80, TFT_BLACK);
                tft->drawRect(10,90,80,80, TFT_WHITE);
                delay(200);
                if (reihenfolge[i] != checkfield){
                  tft->fillScreen(TFT_RED);
                  tft->setTextColor(TFT_WHITE);
                  tft->drawString("FALSCH :(", 100, 100, 4);
                  delay(1000);
                  flag4 = false;
                  level_chosen = false;
                  game_started = false;
                  play = true;
                  reading = false;
                  flag12 = false;
                  sudokufenster_aktiviert = true;
                  rounds = 1;
                  break;}
                i+=1;
              }
              
            }
          }
          delay(1000);
          reading = false;
          play = true;
          rounds+=1;
         }
         

      //Zurück zum Startfenster//
      if (ttgo->getTouch(x, y)) {
        if (x > 160 && x < 240 && y < 25) {
          tft->fillRect(197, 8, 40, 20, TFT_GREEN);
          delay(150);
          tft->fillRect(197, 8, 40, 20, TFT_BLACK);
          tft->setTextColor(TFT_WHITE);
          tft->drawString("Main", 204, 10, 2);
          sudokufenster_aktiviert = false;
          startfenster_aktiviert = true;
          flag4 = false;
          level_chosen = false;
          game_started = false;
          flag12 = false;
          rounds = 1;
          break;
        }
      }
    }
  }

  //******************************* SCHLAFMODUS *************************************//
  schlafmodus();

}



//############################################### FUNKTIONEN ######################################################//
void schlafmodus() {
  if (ttgo->getTouch(x, y)) {
    bildschirm_touched = true;
    zeitpunkt_touching = millis();
    //Serial.println("Bildschirm berührt!");
  }
  else {
    bildschirm_touched = false;
    //Serial.println("Bildschirm nicht berührt");
  }

  if (bildschirm_touched == false) {
    kontrollzeitpunkt = millis();
    if ((kontrollzeitpunkt - zeitpunkt_touching) >= 12000 && (kontrollzeitpunkt - zeitpunkt_touching) < 12200) {
      display_sleep = true;
      display_wokeup = false;
      ttgo->setBrightness(0);
      ttgo->displaySleep();
      //Serial.println("Display off");
    }
  }
  if (ttgo->getTouch(x, y) && display_sleep == true) {
    bildschirm_touched = true;
    display_sleep = false;
    zeitpunkt_touching = millis();
    display_wokeup = true;
    ttgo->displayWakeup();
    ttgo->setBrightness(200);
  }
}
