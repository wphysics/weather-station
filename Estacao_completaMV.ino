#include "Ubidots.h"
#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <math.h>

Adafruit_AHTX0 aht;
Adafruit_BMP085 bmp;

//-------------------------------------------------------------  Pluviometro/anemometro/sensores
int Estado;
#define Zero 0
#define Um 1
#define T0 2
#define T1 3

long int Tempo;
long int Medida;
long int Espera;
long int Timeout = 2000;  // 3 s
long int IntervaloMedida = 240000; // 30 s 

long int Tpluvio;
long int out_pluvio = 1000;
long int cnt_pluvio;
int Epluvio;
#define PinoPlu 12

float Fmax,Fmin;
int meiavolta;
float a; 
float b;
float TE;
#define PinoAne 14

#include <RunningAverage.h>
const int numSamples = 2; // Número de amostras para calcular a média móvel
RunningAverage avgFmed(numSamples); // Objeto para calcular a média móvel de Fmed
RunningAverage avgFmin(numSamples); // Objeto para calcular a média móvel de Fmin
RunningAverage avgFmax(numSamples); // Objeto para calcular a média móvel de Fmax

//------------------------------------------------------------- 
 
const char* UBIDOTS_TOKEN = "BBFF-Gr2pgwQ4Eowpa4vDcDkbUmVtMCqxkL";  // Put here your Ubidots TOKEN
const char* WIFI_SSID = "TP-LINK_F7C892";      // Put here your Wi-Fi SSID
const char* WIFI_PASS = "espectrometro";          // Put here your Wi-Fi password 

Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);

void setup() {
  
  aht.begin();
  bmp.begin();
  Serial.begin(115200);
  ubidots.wifiConnect(WIFI_SSID, WIFI_PASS);   // ubidots.setDebug(true);  // Uncomment this line for printing debug  messages
  pinMode(PinoAne, INPUT);
  Estado = (digitalRead(PinoAne) == HIGH)? Um:Zero;
  Tempo = millis();
  Medida = millis();
  Espera = 0.0;
  Fmax = 0.0;
  Fmin = 50.0;
  meiavolta = 0;
  Tpluvio= millis();
  cnt_pluvio = 0;
  Epluvio =(digitalRead(PinoPlu) == HIGH)? Um:Zero;
  //Serial.println(Epluvio);
}

void loop() {

  switch ( Epluvio ) {
    case Um: if  ( digitalRead(PinoPlu)== LOW ) {
                    Epluvio = Zero;
                    cnt_pluvio ++;
                    Tpluvio = millis();
                }  
                break;
    case Zero: if ( (millis() > Tpluvio + out_pluvio) & digitalRead(PinoPlu)== HIGH) {
                    Epluvio = Um;
          
                } 
                break;
             
  }
       
  switch ( Estado ) {
    case Zero: if ( millis() > Tempo + Timeout) {
                    Estado = T0;
                }
                else  if  ( digitalRead(PinoAne)== HIGH) {
                    Estado = Um;
                    float f = 1/(((millis()-Tempo)/1000.0)*2.0);      // x2 porque o período medido corresponde a 1/2 volta
                    if ( f > Fmax) Fmax = f;
                    if ( f < Fmin) Fmin = f;
                    //Serial.print("Freq = ");
                    //Serial.print(f);
                    //Serial.println("Hz");
                    Tempo = millis();
                }  
                break;
    case Um: if ( millis() > Tempo + Timeout) {
                    Estado = T1;
                }
                else  if  ( digitalRead(PinoAne)== LOW) {
                    Estado = Zero;
                    meiavolta += 1;
                    float f = 1/(((millis()-Tempo)/1000.0)*2.0);      // x2 porque o período medido corresponde a 1/2 volta
                    if ( f > Fmax) Fmax = f;
                    if ( f < Fmin) Fmin = f;
                    //Serial.print("Freq = ");
                    //Serial.print(f);
                    //Serial.println("hz");
                    Tempo = millis();
                }  
              break;
    case T0: if ( digitalRead(PinoAne) == HIGH) {
                Espera += (millis() - Tempo);
                //Serial.print("espera 0 ");
                //Serial.println(Espera);
                Tempo = millis();
                Estado = Um;
              }
              break;
    case T1: if ( digitalRead(PinoAne) == LOW) {
                Espera += (millis() - Tempo);
                //Serial.print("espera 1 ");
                //Serial.println(Espera);
                Tempo = millis();
                Estado = Zero;
              }
              break;                      
  }
  //------------------------------------------------------------- Envio de dados dos Sensores
  if ( millis() > Medida + IntervaloMedida ) {
    if ( Estado == T0 || Estado == T1 )
      Espera += (millis() - Tempo);
      
    float indice_pluv = cnt_pluvio*0.63;     
    float t = (IntervaloMedida - Espera)/1000.0;
    int volta = meiavolta/2;
    float Fmed = volta/t;
    if (Fmin > 49.0){Fmin = 0.0;}
    //float velocidade_vento_med = 2*3.1416*0.1*Fmed;
    //float velocidade_vento_max = 2*3.1416*0.1*Fmax;
    //float velocidade_vento_min = 2*3.1416*0.1*Fmin;

    // Adicionar as amostras aos objetos de média móvel
    avgFmed.addValue(Fmed);
    avgFmin.addValue(Fmin);
    avgFmax.addValue(Fmax);

    // Obter a média móvel de Fmed
    float movingAverageFmed = avgFmed.getAverage();
    // Obter a média móvel de Fmin
    float movingAverageFmin = avgFmin.getAverage();
    // Obter a média móvel de Fmax
    float movingAverageFmax = avgFmax.getAverage();

    float velocidade_vento_med = 2*3.1416*0.1*movingAverageFmed;
    float velocidade_vento_max = 2*3.1416*0.1*movingAverageFmax;
    float velocidade_vento_min = 2*3.1416*0.1*movingAverageFmin;
    /*Serial.print("Fmax ");
    Serial.print(Fmax);
    Serial.print(" Hz Fmed ");
    Serial.print(fmed);
    Serial.print(" Hz Fmin ");
    Serial.print(Fmin);
    Serial.print(" Hz voltas ");
    Serial.print(volta);
    Serial.print(" espera ");
    Serial.println(Espera);*/
    Estado = (digitalRead(PinoAne) == HIGH)? Um:Zero;
    Espera = 0;
    Tempo = millis();
    Medida = millis();
    Fmax = 0.0;
    Fmin = 50.0;
    meiavolta = 0;
    
    //------------------------------------------------------------- LDR
    int sensorValue = analogRead(A0);                   // Ler o pino Analógico A0 onde está o LDR
    float voltageLDR = sensorValue * (3.3 / 1024.0);          // Converter a leitura analógica (que vai de 0 - 1023) para uma voltagem (0 - 3.3V), quanto de acordo com a intensidade de luz no LDR a voltagem diminui.
    //------------------------------------------------------------- AHT10
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    float temp_AHT10 = temp.temperature;
    float umid_AHT10 = humidity.relative_humidity;

    //------------------------------------------------------------- BMP180
    //tempbmp180 = bmp.readTemperature();          // BMP180 sensor get temperature at graus C
    float pres = bmp.readPressure();                   // BMP180 sensor get pressure at Pascal
    //------------------------------------------------------------- Cálculo do Ponto de Orvalho.
     
    if (temp_AHT10 >= 0 && temp_AHT10 <= 50.00) {
    // constantes utilizadas em material fornecido pela(o): Journal of Applied Meteorology and Climatology
    //  Buck, A. L. (1981), "New equations for computing vapor pressure and enhancement factor", J. Appl. Meteorol. 20: 1527–1532
       a = 17.368;
       b = 238.88;
      }
   
      /*
    else if (temp >= -40.00 && temp <= 0) {
      // constantes utilizadas em material fornecido pela(o): Journal of Applied Meteorology and Climatology
      //  Buck, A. L. (1981), "New equations for computing vapor pressure and enhancement factor", J. Appl. Meteorol. 20: 1527–1532
      a = 17.966;
      b = 247.15;
    }
    else {
      // constantes utilizadas em material fornecido pela(o): Sonntag (1990)
      // SHTxx Application Note Dew-point Calculation
      // http://irtfweb.ifa.hawaii.edu/~tcs3/tcs3/Misc/Dewpoint_Calculation_Humidity_Sensor_E.pdf
      a = 17.62;
      b = 243.12;
    }
    */
    float funcao1 = (log(umid_AHT10 / 100) + (a * temp_AHT10) / (b + temp_AHT10));        // cálculo do Ponto de Orvalho parcial
    float temp_pontoOrvalho = (b * funcao1) / (a - funcao1);                  // cálculo do Ponto de Orvalho finalizado
    //------------------------------------------------------------- cálculo sensação térmica
    //---------------------------------------------------------DOI: 10.11606/rdg.v0ispe.140606

    if (velocidade_vento_med <= 0.2) {
      TE = temp_AHT10 - 0.4*(temp_AHT10 - 10)*(1 - 0.01*umid_AHT10);
    }
    
    if (velocidade_vento_med > 0.2) {
      TE = 37 - ((37 - temp_AHT10)/(0.68 - 0.0014*umid_AHT10 + (1/(1.76 + 1.4*(pow(velocidade_vento_med, 0.75)))))) - 0.29*temp_AHT10*(1 - umid_AHT10/100);
    }     
   /*
    Serial.print("Pluviometro ");
    Serial.print(cnt_pluvio);
    Serial.print(", ");
    Serial.println(indice_pluv);
    Serial.print("velocidade_vento_med ");
    Serial.println(velocidade_vento_med);
    Serial.print("velocidade_vento_min ");
    Serial.println(velocidade_vento_min);
    Serial.print("velocidade_vento_max ");
    Serial.println(velocidade_vento_max);
    Serial.print("voltageLDR ");
    Serial.println(voltageLDR);
    Serial.print("temp_AHT10 ");
    Serial.println(temp_AHT10);
    Serial.print("umid_AHT10 ");
    Serial.println(umid_AHT10);
    Serial.print("presão ");
    Serial.println(pres);
    Serial.print("temp_pontoOrvalho ");
    Serial.println(temp_pontoOrvalho);
    Serial.print("TE ");
    Serial.println(TE);
    Serial.println(" ");
    Serial.println(" ");
  //------------------------------------------------------------- Envio dos dados para ubidots
  */
   ubidots.add("Pluviometro", indice_pluv);
   ubidots.add("velocidade_vento_med", velocidade_vento_med);
   ubidots.add("velocidade_vento_min", velocidade_vento_min);
   ubidots.add("velocidade_vento_max", velocidade_vento_max);
   ubidots.add("voltageLDR", voltageLDR);
   ubidots.add("temp_AHT10", temp_AHT10);
   ubidots.add("umid_AHT10", umid_AHT10);
   ubidots.add("pres", pres);
   ubidots.add("temp_pontoOrvalho", temp_pontoOrvalho);
   ubidots.add("TE", TE);
   
   bool bufferSent = false;
   bufferSent = ubidots.send();                        // Will send data to a device label that matches the device Id
}
}
