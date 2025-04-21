/*
Copyright 2025 Gustavs Muižnieks
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Akvārija vadības sistēmas programma
Autors: Gustavs Muižnieks
Trešās puses bibliotēkas, kuras izmantotas šajā projektā:
- Wire.h – Arduino komanda, GNU GPL v2 vai vēlāka versija
- EEPROM.h – Arduino komanda, GNU GPL v2 vai vēlāka versija
- RTClib.h – Adafruit, MIT licence
- LiquidCrystal_I2C.h – Frank de Brabander (un citi), MIT licence
- Adafruit_Sensor.h – Adafruit, Apache licence
- DHT.h – Rob Tillaart, MIT licence
- OneWire.h – Jim Studt, Paul Stoffregen, GNU GPL v3
- DallasTemperature.h – Miles Burton, GNU GPL v3
- Servo.h – Arduino komanda, GNU GPL v2 vai vēlāka versija
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>                 // for RTC
//#include "DHT_Async.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

#define  ParametruVersija  3

//#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHTTYPE    DHT11     // DHT 11

#define GaraisMazais_ee 1
#define GaraisMazais_aa B11100001
#define Latviesu_sh 2
#define CelsijaGrads 0xDF
#define BultaAtpakal 0x7F
#define BultaUzPrieksu 0x7E
#define Kvadratsakne B11101000
#define Garais_uu B11110101
#define Zivs 0
#define Garais_aa B11100001

// DallasTemperature Datu vads - Arduino digitalais pins # 4
#define DallasTemperatureSensors 4
// Konfigurē viena vada interfeisu DallasTemperature instrumentiem
OneWire oneWire(DallasTemperatureSensors);
DallasTemperature sensors(&oneWire);


static const int Enkoders_CLK = 2;
static const int Enkoders_DT = 3;
static const int Enkoders_SW = 5;

static const int RelejaPins_Gaisma1 = 6;
static const int RelejaPins_Gaisma2 = 7;
static const int RelejaPins_CO = 8;
static const int RelejaPins_Silditajs = 9;

static const int ServoPins = 12;


//#define DHT_SENSOR_TYPE DHT_TYPE_21
//#define DHT_SENSOR_TYPE DHT_TYPE_22

static const int DHT_SENSOR_PIN = 11;// uz 10 nestradā
//DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

DHT_Unified dht(DHT_SENSOR_PIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display - 4 rindu lielakām ekranam būs 20,4
RTC_DS3231 pulkstenis;                     // create rtc for the DS3231 RTC module, address is fixed at 0x68. Svarīgi lai I2C adreses nesakrīt
//#define  pulkstenaAtmina_I2C 0x57; // Pulksteņa 32 bitu atmiņu pašlaik nelietoju, bet lai nav pēctam jāmeklē adrese sablabāju.

int counter = 0;
int currentStateCLK;
int lastStateCLK;
unsigned long barosanaOn = 0;

float temperature;
float humidity; 
float tempAkvarija;
bool tempTrauksme=false;
float temperaturaIstaba=0;
float mitrumsIstaba=0;
int pedejaBarosana = 0;
bool co2 = false;
int dienaaktsDala=0;
bool silditajs=false;
unsigned long gaismaEkranam=0;
bool fonaGaisma=0;
bool fonaGaismaUzstaditaVertiba=false;
bool trauksme=0;

Servo myservo;

DateTime PedejasBarosanasDatums;
DateTime now;

//**********************************************************     EEPROM atminā sablabājamā parametru struktūra        *********************************************************************************************//
struct Parametri{
  uint8_t Saullekts_Stundas =  10;
  uint8_t Saullekts_Minutes =  0;

  uint8_t Diena_Stundas =  11;
  uint8_t Diena_Minutes =  0;

  uint8_t Saulriets_Stundas =  20;
  uint8_t Saulriets_Minutes =  0;

  uint8_t Nakts_Stundas =  22;
  uint8_t Nakts_Minutes =  0;


  uint8_t BaroNedelasDienas=127;
  uint8_t Barosana_Stundas =  12;
  uint8_t Barosana_Minutes =  0; 
  
  uint8_t CO2On_Stundas =  10;  
  uint8_t CO2On_Minutes =  30; 

  uint8_t CO2Off_Stundas =  19;
  uint8_t CO2Off_Minutes =  30; 

  bool CO2Izmatosana=1;  
  float Temp=27;  
  float TempTrauksme=4;
  uint32_t versija=ParametruVersija;
};

Parametri parametri;


//*******************************************  Lietotāja izvēlnes stāvokļu mašīnas soļi ******************************************************//
enum Menu_Izvelnes{
  Galvena=0,
  Menu=5,
  DatumsLaiks=10,
  Saullekts=20,
  Diena=30,
  Saulriets=40,
  Nakts=50,
  CO2=60,
  Tempratura=70,
  Barosana=80,
  SaktBarot=90
};
#define  ZimejamieMenuci 10 // Darbību skaits izvēlnēs

Menu_Izvelnes Menu_AktivaIzvelne=Galvena;
Menu_Izvelnes Menu_IeprieksejaIzvelne=-1;
int Menu_KursoraPozicija=0;
int Menu_MinVertiba=0;
int Menu_MaxVertiba=0;

void Ekrans(){

  switch(Menu_AktivaIzvelne){
    case Galvena:
       ZimePamatlogu();
       break;
    case Menu:
       ZimeMenu();
       break;
     case DatumsLaiks:
       Zime_DatumsUnLaiks();
       break;
    case Saullekts:
       Zime_Saullekts();
       break;
    case Nakts:
       Zime_Nakts();
       break;
    case Diena:
       Zime_Diena();
       break;
    case Saulriets:
       Zime_Saulriets();
       break;
    case Barosana:
      Zime_Barosana();
      break;
    case SaktBarot:
      Zime_SaktBarot();
      break;  
    case CO2:
      Zime_CO2();
      break;
    case Tempratura:
      Zime_Temperaturu();
      break;

    default:
      break;

  }

}
bool co2on;
int pozicija;
bool laboPoziciju;
int pozicijaVeca;
int skaitlis1Vecais;
int skaitlis2Vecais;
int skaitlis3Vecais;
int skaitlis4Vecais;
int skaitlis5Vecais;


int skaitlis1;
int skaitlis2;
int skaitlis3;
int skaitlis4;
int skaitlis5;
uint8_t BaroNedelasDienas;
uint8_t BaroNedelasDienasVecas;

int aktivaisLogs;
int aktivaisLogsVec;

void NepareizasVertibas(){
      lcd.setCursor(0,0);               
      lcd.print("Nepareizas vērt.");  
      lcd.setCursor(12,0);          
      lcd.write(byte(GaraisMazais_ee));
      lcd.setCursor(0,0); 
      delay(5000); //5 sekunžu pauze
      Menu_IeprieksejaIzvelne=-10;
}
void DatiSaglabatiVeiksmigi(){
      lcd.setCursor(0,0);               
      lcd.print("Dati saglabati. ");  
      lcd.setCursor(11,0);          
      lcd.write(byte(GaraisMazais_aa));
      lcd.setCursor(0,0); 
      delay(2000); //2 sekundes 
      Menu_AktivaIzvelne=Galvena;
}
void MainaKursoraPozicijuLoga(int lidz ){
   //Pārbauda vai rullītis maina pozīciju
      if(counter!=0){
        if(counter>0){
          pozicija++;
        } else {
          pozicija--;
        }
        counter=0;     
      }
      if(pozicija<0){
        pozicija=0;
      }  
      if (pozicija>lidz)
      {
        pozicija=lidz;
      }
}

void LaikaIevadesStundasOnClick(){
  if(laboPoziciju){
    pozicija++; //Ejam tālāk labot minūtes
  } else {
    laboPoziciju=true;
    lcd.blink();
  }          
  counter=0;      
}

void LaikaIevadesMinutesOnClick(){
  laboPoziciju=!laboPoziciju;
  counter=0;
    if(laboPoziciju){
      lcd.blink();
    } else {
      lcd.noBlink();
    }
}

void  Zime_Laiku(String virsraksts, Menu_Izvelnes ativaizvelne, int stundas, int minutes){
    if(Menu_IeprieksejaIzvelne!=ativaizvelne){
        Menu_IeprieksejaIzvelne=ativaizvelne;
        counter=0;   
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.write(byte(BultaAtpakal));
        lcd.print(virsraksts);               
        lcd.setCursor(15,1);
        lcd.write(byte(Zivs));        
        skaitlis1=stundas;
        skaitlis2=minutes;
        skaitlis1Vecais=-500;
        skaitlis2Vecais=-500;
        pozicija=0;
        pozicijaVeca=0;
        laboPoziciju=false;

        lcd.cursor(); 
        lcd.noBlink();       
      }
      
    if(laboPoziciju && counter!=0){
        switch(pozicija){       
          case 1: //Stundas
            skaitlis1+=counter;
            counter=0;
            if(skaitlis1<0) {
              
              skaitlis1=23;
            }
            if(skaitlis1>23) {
              skaitlis1=0;
            }
            break;
          case 2://Minūtes
            skaitlis2+=counter;
            counter=0;
            
            if(skaitlis2<0) {
              skaitlis2=59;
            }
            if(skaitlis2>59) {
              skaitlis2=0;
            }
            break;
        
          default:     
            break;
        }

    } else {
      MainaKursoraPozicijuLoga(3);
    }

if (
    skaitlis1Vecais!=skaitlis1 || 
    skaitlis2Vecais!=skaitlis2 ||
    pozicijaVeca!=pozicija
    ) {
      //Zīmējam ja ir izmaiņas

    skaitlis1Vecais=skaitlis1;
    skaitlis2Vecais=skaitlis2;
    pozicijaVeca=pozicija;

      lcd.setCursor(1,1);
      if(skaitlis1>9){
        lcd.print(skaitlis1);
      } else {
        lcd.print("0");
        lcd.print(skaitlis1);
      }
      lcd.print(":");
      if(skaitlis2>9){
        lcd.print(skaitlis2);
      } else {
        lcd.print("0");
        lcd.print(skaitlis2);
      }    
      
      
      switch(pozicija){
        case 0: 
          lcd.setCursor(0,0);
          break;
        case 1:
         lcd.setCursor(2,1);
          break;
        case 2:
          lcd.setCursor(5,1);
          break;
        case 3:
          lcd.setCursor(15,1);
          break;      

        default:
          //Vērtība pāri robežām
          pozicija=0;
          break;
      }
    } 

}

/******************************************* Sakt barot *****************************************************/
bool zimeBarotKeksi;
bool zimeBarotKeksiVec;
void  SaktBarot_OnClick(){
    uint8_t bitsDienasMainai=0;
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Ķeksis         
          //Maina ķekša vērtību uz pretējo
         zimeBarotKeksi=!zimeBarotKeksi;     
          break;
           
          
        case 2: // Saglabā ja ir OK
          laboPoziciju=false;
          if( zimeBarotKeksi ){            
              SaktBarotFunkcija();
              //DatiSaglabatiVeiksmigi();
               Menu_AktivaIzvelne=Galvena;
          }
          else {
             
               Menu_AktivaIzvelne=Galvena;
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
 
}

 String   MenesuSaisinajumi[12]={
      ".Jan ",
      ".Feb ",
      ".Mar ", 
      ".Apr ", 
      ".Maj ",  
      ".Jun ", 
      ".Jul ",
      ".Aug ",
      ".Sep ",
      ".Okt ",
      ".Nov ",
      ".Dec "} ; 

void  Zime_SaktBarot(){
    if(Menu_IeprieksejaIzvelne!=SaktBarot || aktivaisLogs!=aktivaisLogsVec){
        if(Menu_IeprieksejaIzvelne!=SaktBarot){
   
            pozicija=0;
            pozicijaVeca=0;
            laboPoziciju=false;   
            zimeBarotKeksi = false;
            zimeBarotKeksiVec = true;     
        }
       
        Menu_IeprieksejaIzvelne=SaktBarot;
   
        counter=0;   
 
        lcd.cursor(); 
        lcd.noBlink();   

          lcd.setCursor(0,0);
          lcd.write(byte(BultaAtpakal));
          
          if(PedejasBarosanasDatums.year()<=2000){
            lcd.print(" Nav barots!         ");  
          } else {
            lcd.print(" ");  
            lcd.print(PedejasBarosanasDatums.day());
            lcd.print(MenesuSaisinajumi[PedejasBarosanasDatums.month()-1]);
            
            if(PedejasBarosanasDatums.hour()<10){
              lcd.print("0");
            }
            lcd.print(PedejasBarosanasDatums.hour());
            if(PedejasBarosanasDatums.minute()<10){
              lcd.print(":0");
            } else {
              lcd.print(":");
            }
            lcd.print(PedejasBarosanasDatums.minute());            
            lcd.print("     ");  
          }
          lcd.setCursor(0,1);
          lcd.print("Sakt barot [ ]      ");  
          lcd.setCursor(1,1);          
          lcd.write(byte(Garais_aa));     
          lcd.setCursor(15,1);
          lcd.write(byte(Zivs));
      }  
       
 
      MainaKursoraPozicijuLoga(2);
 
  if (     
      pozicijaVeca!=pozicija ||
       zimeBarotKeksi !=  zimeBarotKeksiVec 
      ) {
        //Zīmējam ja ir izmaiņas      
         pozicijaVeca=pozicija;
        zimeBarotKeksiVec =  zimeBarotKeksi;

        if(zimeBarotKeksi){        
              lcd.setCursor(12,1);        
              lcd.write(byte(Kvadratsakne));
            } else {
              lcd.setCursor(12,1);
              lcd.print(" ");
            }

        switch(pozicija){
          case 0: 
            lcd.setCursor(0,0);
            break;
          case 1:
          lcd.setCursor(12,1);        
            break;
          case 2:
            lcd.setCursor(15,1);
            break;         

          default:                       
            break;
        }
      } 

   
}

/********************************************** Barošana ******************************************************************/
void  Barosana_OnClick(){
    uint8_t bitsDienasMainai=0;
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Stundas         

          LaikaIevadesStundasOnClick();       
          break;
        case 2: //Minuites        
          LaikaIevadesMinutesOnClick();
          break;

           case 3: //Pirmdiena
            laboPoziciju=false;
            lcd.noBlink();
            bitsDienasMainai=1;
            break;
           case 4: //O
           laboPoziciju=false;
           lcd.noBlink();
            bitsDienasMainai=2;
            break;
           case 5: //T
           laboPoziciju=false;
           lcd.noBlink();
            bitsDienasMainai=4;
           break;           
           case 6: //C
              laboPoziciju=false;
              lcd.noBlink();
              bitsDienasMainai=8;
           break;
           case 7: //P
              laboPoziciju=false;
              lcd.noBlink();
              bitsDienasMainai=16;
           break;
           case 8: //S
              laboPoziciju=false;
              lcd.noBlink();
              bitsDienasMainai=32;
           break;
           case 9: //Sv
              laboPoziciju=false;
              lcd.noBlink();
              bitsDienasMainai=64;
           break;
        case 10: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 
          ){            
                if(
                  parametri.Barosana_Stundas!=skaitlis1 ||
                  parametri.Barosana_Minutes!=skaitlis2 ||
                  parametri.BaroNedelasDienas!=BaroNedelasDienas
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Barosana_Stundas=skaitlis1;
                  parametri.Barosana_Minutes=skaitlis2;
                  parametri.BaroNedelasDienas=BaroNedelasDienas;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Barosana(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
        if(bitsDienasMainai!=0){
            //Mainītas nedēlas dienas
            //Ar XOR maina uz pretējo vērtību bitu
            BaroNedelasDienas=BaroNedelasDienas^bitsDienasMainai;
        }
}

void  Zime_Barosana(){
    if(Menu_IeprieksejaIzvelne!=Barosana || aktivaisLogs!=aktivaisLogsVec){
        if(Menu_IeprieksejaIzvelne!=Barosana){
            param_Nolasit();
            skaitlis1=parametri.Barosana_Stundas;
            skaitlis2=parametri.Barosana_Minutes;
            BaroNedelasDienas=parametri.BaroNedelasDienas;
            pozicija=0;
            pozicijaVeca=0;
            laboPoziciju=false;
            aktivaisLogs=1;
            aktivaisLogsVec=0;
        }
       
        Menu_IeprieksejaIzvelne=Barosana;
        aktivaisLogsVec=aktivaisLogs;
        counter=0;   
        lcd.clear();
        if(aktivaisLogs==1){       

          skaitlis1Vecais=-500;
          skaitlis2Vecais=-500;
        }
        else{
      
            BaroNedelasDienasVecas=256;
        }
      
        lcd.cursor(); 
        lcd.noBlink();   
            
      }
   
       
    if(laboPoziciju && counter!=0){
     
        switch(pozicija){       
          case 1: //Stundas
            skaitlis1+=counter;
            counter=0;
            if(skaitlis1<0) {              
              skaitlis1=23;
            }
            if(skaitlis1>23) {
              skaitlis1=0;
            }
            break;
          case 2://Minūtes
            skaitlis2+=counter;
            counter=0;
            
            if(skaitlis2<0) {
              skaitlis2=59;
            }
            if(skaitlis2>59) {
              skaitlis2=0;
            }
            break;
         
        
          default:     
            break;
        }
      

    } else {
      MainaKursoraPozicijuLoga(10);
     
      if(pozicija>2 && pozicija<10){
        aktivaisLogs=2;
      } else {
        aktivaisLogs=1;
      } 
    }


if(aktivaisLogs==1){
  if (
      skaitlis1Vecais!=skaitlis1 || 
      skaitlis2Vecais!=skaitlis2 ||
      pozicijaVeca!=pozicija ||
      aktivaisLogs!=aktivaisLogsVec
      ) {
        //Zīmējam ja ir izmaiņas
      aktivaisLogsVec=aktivaisLogs;
      skaitlis1Vecais=skaitlis1;
      skaitlis2Vecais=skaitlis2;
      pozicijaVeca=pozicija;


          lcd.setCursor(0,0);
          lcd.write(byte(BultaAtpakal));
          lcd.print(" Barosana:      ");  
          lcd.setCursor(6,0);          
          lcd.write(byte(Latviesu_sh));       
        

        lcd.setCursor(1,1);
        if(skaitlis1>9){
          lcd.print(skaitlis1);
        } else {
          lcd.print("0");
          lcd.print(skaitlis1);
        }
        lcd.print(":");
        if(skaitlis2>9){
          lcd.print(skaitlis2);
        } else {
          lcd.print("0");
          lcd.print(skaitlis2);
        }      
        //Dzes lidz rindas beigam 
        lcd.print("           ");
        lcd.setCursor(15,1);
        lcd.write(byte(Zivs));
        
        switch(pozicija){
          case 0: 
            lcd.setCursor(0,0);
            break;
          case 1:
          lcd.setCursor(2,1);
            break;
          case 2:
            lcd.setCursor(5,1);
            break;
          case 10:
            lcd.setCursor(15,1);
            break;      

          default:           
            break;
        }
      } 

  } else {
    if (
      BaroNedelasDienasVecas!=BaroNedelasDienas ||       
      aktivaisLogs!=aktivaisLogsVec ||
      pozicijaVeca!=pozicija
      ) {
        //Zīmējam ja ir izmaiņas
      BaroNedelasDienasVecas=BaroNedelasDienas;
      aktivaisLogsVec=aktivaisLogs;
      pozicijaVeca=pozicija;
        lcd.setCursor(0,0);
        lcd.print(" P O T C P S Sv "); 
      //Dzesam otro rindu
      lcd.setCursor(0 ,1);
      lcd.print("                 ");

      uint8_t ndParbauditajs=1;
      for(uint8_t nd=0; nd<7; nd++)
      {
          lcd.setCursor(1 + 2*nd,1);
          uint8_t rez=BaroNedelasDienas & ndParbauditajs;
          if(rez!=0 ){
            lcd.write(byte(Kvadratsakne));
          } 
          ndParbauditajs=ndParbauditajs*2;          
      }    
      
      lcd.setCursor(1 + 2*(pozicija-3/*Pirmdiena ir 3*/),1);                
      } 
  }
}

/********************************************** Temperatūra ******************************************************************/
void  Temperatura_OnClick(){
     //Serial.print("pozicija: ");  Serial.println(pozicija);
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;        
        
          break;
        case 1:        
       
           if(laboPoziciju){
              pozicija++; //Ejam tālāk +/- nobīdi
            } else {
              laboPoziciju=true;
              lcd.blink();
            }          
            counter=0;          
          break;
        case 2: // +/- nobīde        
          laboPoziciju=!laboPoziciju;
          counter=0;
            if(laboPoziciju){
              lcd.blink();
            } else {
              lcd.noBlink();
            }
          break;   
        case 3: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=100 && skaitlis1<350 && 
              skaitlis2>=5 && skaitlis2<=100 
          ){     
              float desmit=10.0;
                if(
                  parametri.Temp!=(skaitlis1/desmit) ||
                  parametri.TempTrauksme!=(skaitlis2/desmit)
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Temp=skaitlis1/desmit;
                  parametri.TempTrauksme=skaitlis2/desmit;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Temperaturu(); //Pārzīmē ekrānu
          }        

          break;

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }      
}
//bookmark2
void  Zime_Temperaturu(){
    if(Menu_IeprieksejaIzvelne!=Tempratura ){    
            
            param_Nolasit();
            /*
            Skitļus glabā kā float daļkaitļus, bet labojot labo kā int veselus skaitļus, reizinot ar 10, lai varētu labot līdz grāda desmitdaļām. 
              float Temp=27.933;  int skaitlis1 =279;
              float TempTrauksme=4;  int skaitlis2 =40;
            */
            skaitlis1=parametri.Temp*10;
            skaitlis2=parametri.TempTrauksme*10;

              //Pārbauda vai zivīm temeratūra ir nomālā diapazonā
            if(skaitlis1>350 ||skaitlis1<100 ){
              skaitlis1 = 270;//noklusētā vērtība 27 grādi
            }
            if(skaitlis2>100 || skaitlis2<5){
              skaitlis2 = 5;//noklusētā vērtība 0.5 grāsdi
            }
            
            pozicija=0;
            pozicijaVeca=0;
            laboPoziciju=false;      
       
            Menu_IeprieksejaIzvelne=Tempratura;
            counter=0;   
            lcd.clear();
      
            skaitlis1Vecais=-500;
        
            lcd.setCursor(0,0);
            lcd.write(byte(BultaAtpakal));
            String ugarais=" Temperatura   ";
            ugarais[9]=Garais_uu;          
            lcd.print(ugarais);   

            lcd.setCursor(15,0);
            lcd.write(byte(Zivs));        

            lcd.cursor(); 
            lcd.noBlink();
    }
    if(pozicija>4 && pozicija<0){
        pozicija=0;
      }
       
    if(laboPoziciju && counter!=0){
     
        switch(pozicija){       
          case 1: //Stundas
            skaitlis1+=counter;
            counter=0;
            if(skaitlis1<100) {              
              skaitlis1=100;
            }
            if(skaitlis1>350) {
              skaitlis1=350;
            }
            break;
          case 2://Minūtes
            skaitlis2+=counter;
            counter=0;
            
            if(skaitlis2< 5) {
              skaitlis2=5;
            }
            if(skaitlis2>100) {
              skaitlis2=100;
            }
            break;
        
          default:     
            break;
        }
      

    } else {
      MainaKursoraPozicijuLoga(4);    
    }
 

if (
      skaitlis1Vecais!=skaitlis1 || 
      skaitlis2Vecais!=skaitlis2 ||      
      pozicijaVeca!=pozicija  
      ) {
        //Zīmējam ja ir izmaiņas
      skaitlis1Vecais=skaitlis1;
      skaitlis2Vecais=skaitlis2; 
      pozicijaVeca=pozicija;        

        lcd.setCursor(0,1);
        lcd.print("      +/-         ");
        lcd.setCursor(1,1);
        
        lcd.print(skaitlis1/10);
        lcd.print(".");
        lcd.print(skaitlis1 % 10);
        
        lcd.setCursor(10,1);
        
        lcd.print(skaitlis2/10);
        lcd.print(".");
        lcd.print(skaitlis2 % 10);
        
        switch(pozicija){
          case 0: 
            lcd.setCursor(0,0);
            break;
          case 1:
          lcd.setCursor(4,1);
            break;
          case 2:
            lcd.setCursor(12,1);
            break;
          case 3:
            lcd.setCursor(15,0);
            break;  
          
          default:           
            break;
        }
      }   
}

/********************************************** CO2 ******************************************************************/
void  CO2_OnClick(){
   //Serial.print("pozicija: ");  Serial.println(pozicija);
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1:
          laboPoziciju=false;
           lcd.noBlink();
          co2on=!co2on;
         //Serial.print("co2: ");
         //Serial.println(co2on);
          break;
        case 2: //Stundas
        case 3: //Minuites  
        case 4: //Stundas         
           if(laboPoziciju){
              pozicija++; //Ejam tālāk labot nākamo lauku
            } else {
              laboPoziciju=true;
              lcd.blink();
            }          
            counter=0;          
          break;
        case 5: //Minuites    
          //Pēdējais labojamais lauks, tāpēc ja tagad jau labo tad beidz labot uz klikšķa    
          laboPoziciju=!laboPoziciju;
          counter=0;
            if(laboPoziciju){
              lcd.blink();
            } else {
              lcd.noBlink();
            }
          break;   
        case 6: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 &&
              skaitlis3>=0 && skaitlis3<24 &&
              skaitlis4>=0 && skaitlis4<60
          ){            
                if(
                  parametri.CO2On_Stundas!=skaitlis1 ||
                  parametri.CO2On_Minutes!=skaitlis2 ||
                  parametri.CO2Off_Stundas!=skaitlis3 ||
                  parametri.CO2Off_Minutes!=skaitlis4 ||

                  parametri.CO2Izmatosana!=co2on
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.CO2On_Stundas=skaitlis1;
                  parametri.CO2On_Minutes=skaitlis2;
                  parametri.CO2Off_Stundas=skaitlis3;
                  parametri.CO2Off_Minutes=skaitlis4;
                  parametri.CO2Izmatosana=co2on;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_CO2(); //Pārzīmē ekrānu
          }       

          break;         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }      
}

bool co2onVec;
void  Zime_CO2(){
    if(Menu_IeprieksejaIzvelne!=CO2 ){
    
            param_Nolasit();
            /*  
            Datu struktūras fragments ar noklusētajām vērtībām:
            int8_t CO2On_Stundas =  10;  
            uint8_t CO2On_Minutes =  30; 

            uint8_t CO2Off_Stundas =  19;
            uint8_t CO2Off_Minutes =  30; 

            bool CO2Izmatosana=1;  */
            skaitlis1=parametri.CO2On_Stundas;
            skaitlis2=parametri.CO2On_Minutes;
            skaitlis3=parametri.CO2Off_Stundas;
            skaitlis4=parametri.CO2Off_Minutes;
            if(parametri.CO2Izmatosana==0){
              co2on = false;
            } else {
              co2on = true;
            }
            
            pozicija=0;
            pozicijaVeca=0;
            laboPoziciju=false;      
       
            Menu_IeprieksejaIzvelne=CO2;
            counter=0;   
            lcd.clear();
      
            skaitlis1Vecais=-500;
        
            lcd.setCursor(0,0);
            lcd.write(byte(BultaAtpakal));
            lcd.print(" CO2: [ ]       ");         
            lcd.cursor(); 
            lcd.noBlink(); 
      }
    if(pozicija>6 && pozicija<0){
        pozicija=0;
      }
       
    if(laboPoziciju && counter!=0){
     
        switch(pozicija){       
          case 2: //Stundas
            skaitlis1+=counter;
            counter=0;
            if(skaitlis1<0) {              
              skaitlis1=23;
            }
            if(skaitlis1>23) {
              skaitlis1=0;
            }
            break;
          case 3://Minūtes
            skaitlis2+=counter;
            counter=0;
            
            if(skaitlis2<0) {
              skaitlis2=59;
            }
            if(skaitlis2>59) {
              skaitlis2=0;
            }
            break;
          case 4: //Stundas
            skaitlis3+=counter;
            counter=0;
            if(skaitlis3<0) {              
              skaitlis3=23;
            }
            if(skaitlis3>23) {
              skaitlis3=0;
            }
            break;
          case 5://Minūtes
            skaitlis4+=counter;
            counter=0;
            
            if(skaitlis4<0) {
              skaitlis4=59;
            }
            if(skaitlis4>59) {
              skaitlis4=0;
            }
            break;
        
          default:     
            break;
        }
      

    } else {
      MainaKursoraPozicijuLoga(6);    
    }
 

if (
      skaitlis1Vecais!=skaitlis1 || 
      skaitlis2Vecais!=skaitlis2 ||
      skaitlis3Vecais!=skaitlis3 || 
      skaitlis4Vecais!=skaitlis4 ||
      pozicijaVeca!=pozicija ||
      co2onVec!=co2on
      ) {
        //Zīmējam ja ir izmaiņas
      skaitlis1Vecais=skaitlis1;
      skaitlis2Vecais=skaitlis2;
      skaitlis3Vecais=skaitlis3;
      skaitlis4Vecais=skaitlis4;
      co2onVec=co2on;
      pozicijaVeca=pozicija;

        lcd.setCursor(1,1);
        if(skaitlis1>9){
          lcd.print(skaitlis1);
        } else {
          lcd.print("0");
          lcd.print(skaitlis1);
        }
        lcd.print(":");
        if(skaitlis2>9){
          lcd.print(skaitlis2);
        } else {
          lcd.print("0");
          lcd.print(skaitlis2);
        }     
        lcd.print("  ");
        if(skaitlis3>9){
          lcd.print(skaitlis3);
        } else {
          lcd.print("0");
          lcd.print(skaitlis3);
        }
        lcd.print(":");
        if(skaitlis4>9){
          lcd.print(skaitlis4);
        } else {
          lcd.print("0");
          lcd.print(skaitlis4);
        } 

        //Dzes lidz rindas beigam 
        lcd.print("      ");
        lcd.setCursor(15,1);
        lcd.write(byte(Zivs));

        lcd.setCursor(8,0);
        if(co2on){        
          lcd.write(byte(Kvadratsakne));
        } else {
          lcd.print(" ");
        }
        
        switch(pozicija){
          case 0: 
            lcd.setCursor(0,0);
            break;
          case 1:
          lcd.setCursor(8,0);
            break;
          case 2:
            lcd.setCursor(2,1);
            break;
          case 3:
            lcd.setCursor(5,1);
            break;  
          case 4:
            lcd.setCursor(9,1);
            break;
          case 5:
            lcd.setCursor(12,1);
            break;  
          case 6:
            lcd.setCursor(15,1);
            break;   

          default:           
            break;
        }
      } 
  
}


/********************************************** Saullēkts ******************************************************************/
void  Saullekts_OnClick(){
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Stundas 
          LaikaIevadesStundasOnClick();      
          break;
        case 2: //Minutes         
          LaikaIevadesMinutesOnClick();
          break;
        case 3: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 
          ){            
                if(
                  parametri.Saullekts_Stundas!=skaitlis1 ||
                  parametri.Saullekts_Minutes!=skaitlis2 
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Saullekts_Stundas=skaitlis1;
                  parametri.Saullekts_Minutes=skaitlis2;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Saullekts(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
}

void  Zime_Saullekts(){
     if(Menu_IeprieksejaIzvelne!=Saullekts){
       param_Nolasit();
     }
    String x=" Saullekts: ";
    x[6]=GaraisMazais_ee;
    Zime_Laiku(x, Saullekts, parametri.Saullekts_Stundas,parametri.Saullekts_Minutes);

}

/********************************************** Saulriets ******************************************************************/
void  Saulriets_OnClick(){
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Stundas 
          LaikaIevadesStundasOnClick();        
          break;
        case 2: //Minutes         
          LaikaIevadesMinutesOnClick();
          break;
        case 3: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 
          ){            
                if(
                  parametri.Saulriets_Stundas!=skaitlis1 ||
                  parametri.Saulriets_Minutes!=skaitlis2 
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Saulriets_Stundas=skaitlis1;
                  parametri.Saulriets_Minutes=skaitlis2;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Saulriets(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
}

void  Zime_Saulriets(){
       if(Menu_IeprieksejaIzvelne!=Saulriets){
       param_Nolasit();
     }
    Zime_Laiku(" Saulriets: ", Saulriets, parametri.Saulriets_Stundas,parametri.Saulriets_Minutes);

}

/********************************************** Nakts ******************************************************************/
void  Nakts_OnClick(){
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Stundas 
          LaikaIevadesStundasOnClick();        
          break;
        case 2: //Minutes         
          LaikaIevadesMinutesOnClick();
          break;
        case 3: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 
          ){            
                if(
                  parametri.Nakts_Stundas!=skaitlis1 ||
                  parametri.Nakts_Minutes!=skaitlis2 
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Nakts_Stundas=skaitlis1;
                  parametri.Nakts_Minutes=skaitlis2;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Nakts(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
}

void  Zime_Nakts(){
       if(Menu_IeprieksejaIzvelne!=Nakts){
       param_Nolasit();
     }
    Zime_Laiku(" Nakts: ", Nakts, parametri.Nakts_Stundas,parametri.Nakts_Minutes);

}

/********************************************** Diena ******************************************************************/
void  Diena_OnClick(){
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Stundas 
          LaikaIevadesStundasOnClick();        
          break;
        case 2: //Minutes         
          LaikaIevadesMinutesOnClick();
          break;
        case 3: // Saglabā ja ir OK
          laboPoziciju=false;
          if( skaitlis1>=0 && skaitlis1<24 && 
              skaitlis2>=0 && skaitlis2<60 
          ){            
                if(
                  parametri.Diena_Stundas!=skaitlis1 ||
                  parametri.Diena_Minutes!=skaitlis2 
                ) {
                  //Sagalbā ja dati tiešam mainīti, jo ir ierobežots maksimālais ierastīšanas reižu saits
                  parametri.Diena_Stundas=skaitlis1;
                  parametri.Diena_Minutes=skaitlis2;
                  param_Saglabat();
                }
              DatiSaglabatiVeiksmigi();
          }
          else {
               NepareizasVertibas();
                Zime_Diena(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
}

void  Zime_Diena(){
       if(Menu_IeprieksejaIzvelne!=Diena){
       param_Nolasit();
     }
    Zime_Laiku(" Diena: ", Diena, parametri.Diena_Stundas,parametri.Diena_Minutes);

}

/********************************************** Datums un laiks ******************************************************************/

void  DatumsUnLaiks_OnClick(){
   switch(pozicija){
        case 0:  //Atgriežamies neko nesaglabājot
          Menu_AktivaIzvelne=Galvena;
          break;
        case 1: //Gads 
        case 2: //Mēnesis
        case 3: //Diena
        case 4: //Stundas        
          LaikaIevadesStundasOnClick();         
          break;
        case 5: //Minutes         
          LaikaIevadesMinutesOnClick();
          break;
        case 6: // Saglabā ja ir OK
          laboPoziciju=false;
          if( 
              skaitlis1>2024 && skaitlis1<2100 && 
              skaitlis2>0 && skaitlis2<13 && 
              skaitlis3>0 && 
              skaitlis3<=31 && 
              
              (skaitlis3<29 || skaitlis2!= 2 /*februāris*/) &&
              (skaitlis3<31 || !(skaitlis2== 4 || skaitlis2==6 || skaitlis2==9 || skaitlis2==11) /*īsie mēneši*/) &&
              
              skaitlis4>=0 && skaitlis4<24 && 
              skaitlis5>=0 && skaitlis5<60 

          ){        
               
             pulkstenis.adjust(DateTime(skaitlis1,skaitlis2,skaitlis3,skaitlis4,skaitlis5,0));  
            now = pulkstenis.now(); //Nolasa reāli uzstādīto laiku no pulksteņa, tads kaāds ir saglabāts
            if( 
              skaitlis1!=now.year() ||
              skaitlis2!=now.month() ||
              skaitlis3!=now.day() ||
              skaitlis4!=now.hour() ||
              skaitlis5!=now.minute() 
            ) {   
             
              NepareizasVertibas(); 
                Zime_DatumsUnLaiks(); //Pārzīmē ekrānu
                break;
            }

               DatiSaglabatiVeiksmigi();
          }
          else {
           
                NepareizasVertibas(); 
                Zime_DatumsUnLaiks(); //Pārzīmē ekrānu
          }        

          break;
         

        default:
          //Vērtība pāri robezām
          pozicija=0;
          laboPoziciju=false;
          break;
      }
}

void  Zime_DatumsUnLaiks(){
    if(Menu_IeprieksejaIzvelne!=DatumsLaiks){
          Menu_IeprieksejaIzvelne=DatumsLaiks;
          counter=0;   
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.write(byte(BultaAtpakal));
          lcd.print("Datums, laiks  ");         
          lcd.setCursor(15,0);
          lcd.write(byte(Zivs));
          
          skaitlis1=now.year();
          skaitlis2=now.month();
          skaitlis3=now.day();
          skaitlis4=now.hour();
          skaitlis5=now.minute();
          skaitlis1Vecais=-500;
          pozicija=0;
          pozicijaVeca=0;
          laboPoziciju=false;

          lcd.cursor(); 
          lcd.noBlink();       
      }
      
    if(laboPoziciju && counter!=0){
        switch(pozicija){       
          case 1: //Gadi
            skaitlis1+=counter;
            counter=0;
            if(skaitlis1<2025) {
              
              skaitlis1=2025;
            }
            if(skaitlis1>2100) {
              skaitlis1=2100;
            }
            break;
          case 2://Mēneši
            skaitlis2+=counter;
            counter=0;
            
            if(skaitlis2<0) {
              skaitlis2=0;
            }
            if(skaitlis2>12) {
              skaitlis2=12;
            }
            break;

          case 3://Dienas
            skaitlis3+=counter;
            counter=0;
            
            if(skaitlis3<0) {
              skaitlis3=0;
            }
            if(skaitlis3>31) {
              skaitlis3=31;
            }
            break;

          case 4: //Stundas
            skaitlis4+=counter;
            counter=0;
            if(skaitlis4<0) {
              
              skaitlis4=23;
            }
            if(skaitlis4>23) {
              skaitlis4=0;
            }
            break;
          case 5://Minūtes
            skaitlis5+=counter;
            counter=0;
            
            if(skaitlis5<0) {
              skaitlis5=59;
            }
            if(skaitlis5>59) {
              skaitlis5=0;
            }
            break;
        
          default:     
            break;
        }

    } else {
      MainaKursoraPozicijuLoga(6);
    }



  if (
      skaitlis1Vecais!=skaitlis1 || 
      skaitlis2Vecais!=skaitlis2 ||
      skaitlis3Vecais!=skaitlis3 ||
      skaitlis4Vecais!=skaitlis4 ||
      skaitlis5Vecais!=skaitlis5 ||
      pozicijaVeca!=pozicija
      ) {
        //Zīmējam ja ir izmaiņas

      skaitlis1Vecais=skaitlis1;
      skaitlis2Vecais=skaitlis2;
      skaitlis3Vecais=skaitlis3;
      skaitlis4Vecais=skaitlis4;
      skaitlis5Vecais=skaitlis5;
      pozicijaVeca=pozicija;

        lcd.setCursor(0,1);
        lcd.print(skaitlis1);
        lcd.print(".");

        if(skaitlis2>9){
          lcd.print(skaitlis2);
        } else {
          lcd.print("0");
          lcd.print(skaitlis2);
        }
        lcd.print(".");

        if(skaitlis3>9){
          lcd.print(skaitlis3);
        } else {
          lcd.print("0");
          lcd.print(skaitlis3);
        }
        lcd.print(" ");

        if(skaitlis4>9){
          lcd.print(skaitlis4);
        } else {
          lcd.print("0");
          lcd.print(skaitlis4);
        }
        lcd.print(":");

        if(skaitlis5>9){
          lcd.print(skaitlis5);
        } else {
          lcd.print("0");
          lcd.print(skaitlis5);
        }
        
        
        
        switch(pozicija){
          case 0: 
            lcd.setCursor(0,0);
            break;
          case 1:
          lcd.setCursor(3,1);
            break;
          case 2:
            lcd.setCursor(6,1);
            break;
          case 3:
            lcd.setCursor(9,1);
            break;
          case 4:
            lcd.setCursor(12,1);
            break;
          case 5:
            lcd.setCursor(15,1);
            break;
          case 6:
            lcd.setCursor(15,0);
            break;      

          default:
            //Vērtība pāri robezām
            pozicija=0;
            break;
        }
      } 

    }

/*****************************************************  Galvanais logs  ******************************************************************/

unsigned long displajaLaikaApdeitsVeiks=0; 

void  ZimePamatlogu(){
  if(Menu_AktivaIzvelne!=Menu_IeprieksejaIzvelne){
    Menu_IeprieksejaIzvelne=Menu_AktivaIzvelne;
    counter=0;   
    lcd.clear();
    lcd.noCursor();
    lcd.noBlink();
   }

  unsigned long  mi=millis();
  if(displajaLaikaApdeitsVeiks +500>mi){
    // Nav jēga pārzīmēt pamatekrānu biežāk kā reizi 0.5 sekundēs, jo tur nekas nebūs mainijies
    return;
  }
  displajaLaikaApdeitsVeiks=mi;  

// Fona gaismas uztādīšana
if(gaismaEkranam> mi || tempTrauksme){
  //Nav pienāci laiks aptumšot ekrānu vai trauksme
  fonaGaisma=true;
  } else {
    fonaGaisma=false;
  }

  //Zīmē laiku
  uint16_t gads=now.year()-2000;
  uint8_t menesis=now.month();
  uint8_t diena=now.day();
  uint8_t stundas=now.hour();
  uint8_t minutes=now.minute();
  uint8_t sekundes=now.second(); 

//Izdrukā datumu un laiku, pieliekot 0 priekšā, ja skaitlis ir viencipara
  lcd.setCursor(0,0);
  lcd.print(gads);
  lcd.print('.');
  /*
  Jāietaupa 1 simbola vieta
  if(menesis<10){
    lcd.print("0");  
  }*/
  lcd.print(menesis);
  lcd.print('.');
  if(diena<10){
    lcd.print("0"); 
  }
  lcd.print(diena);
  lcd.print(' ');
  if(stundas<10){
    lcd.print("0"); 
  }
  lcd.print(stundas);
  lcd.print(':');
  if(minutes<10){
    lcd.print("0"); 
  }
  lcd.print(minutes);
 // Ja mēnesis ir 2 ciparu, tad ekrānā nav vietas sekundēm 
  if(menesis<10){
      lcd.print(':');
      if(sekundes<10){
        lcd.print("0"); 
      }
      lcd.print(sekundes);
   }
  lcd.print("     ");  

  //Attēlo vides parametrus
  lcd.setCursor(0,1);

  
// Reizi 10 sekundēs maina to, ko attēlo 2. rindiņā
  if( ((sekundes/10)%2==0) || tempTrauksme  || temperaturaIstaba==0 /*Ja THC sensors nedarbojas, tad vienmēr rāda 1. variantu */){
    // katras 10 sekundes daījums būs pāra skaitlis, lidz ar to tas dalīsies ar 2 bez atlikuma, kamēr nomainīsies nakamais sekunžu desmits  
      lcd.print("T=");
      lcd.print(tempAkvarija, 1);
      lcd.print("C            ");
     
    if (pedejaBarosana == now.day()){
       lcd.setCursor(7,1);
       lcd.write(byte(Zivs));   //Akvarija T
    }

      if(co2){
        lcd.setCursor(9,1);
        lcd.print("CO");
        lcd.write(byte(3));   //Simboli_CO2_2
      }

      lcd.setCursor(13,1);
      switch(dienaaktsDala){
        case 0:
        case 4:
         lcd.write(byte(7));   //Simboli_Nakts
         return;
        case 1://Rīts
         lcd.write(byte(4));   //Simboli_Saulekts
         return;
        case 2://Diena
         lcd.write(byte(5));   //Simboli_Saule
         return;
        case 3://Vakars
         lcd.write(byte(6));   //Simboli_Saulriets
         return;
      } 

  } else {
        //Otrās rindiņas otrs variants
        lcd.print("T");
        lcd.write(byte(3));   //Simboli_CO2_2
        lcd.print("=");
        lcd.print(temperaturaIstaba, 1);
        lcd.print("C, H=");
        lcd.print(mitrumsIstaba, 1);
        lcd.println("%   ");
  }
}
//*********************************************************  Poga_OnClick *******************************************************************************//
//Vadības pogas klikška apstrāda atbilstoši iekārtas stāvoklim
void Poga_OnClick(){
  gaismaEkranam= millis() + 60*1000; //Ieslēdzam ekrāna gaismu uz 60 sekundēm

  switch(Menu_AktivaIzvelne){
    case Galvena:
       Menu_AktivaIzvelne=Menu;
       return;
    case DatumsLaiks:
        DatumsUnLaiks_OnClick();
        return;
    case Saullekts:
       Saullekts_OnClick();
       return;
    case Saulriets:
       Saulriets_OnClick();
       return;
    case Nakts:
       Nakts_OnClick();
       return;
    case Diena:
       Diena_OnClick();
       return;
    case Barosana:
       Barosana_OnClick();
        return;
    case  SaktBarot:
        SaktBarot_OnClick();
        return;
    case CO2:
      CO2_OnClick();
      return;
    case Tempratura:
      Temperatura_OnClick();
      return;
    case Menu:
       switch (Menu_KursoraPozicija){
          case 0:
          case 10:
            Menu_AktivaIzvelne=Galvena;
            return;
          case 1:
            Menu_AktivaIzvelne=DatumsLaiks;
            return;
          case 2:
            Menu_AktivaIzvelne=Saullekts;
            return;
          case 3:
            Menu_AktivaIzvelne=Diena;
            return;
          case 4:
            Menu_AktivaIzvelne=Saulriets;
            return;
          case 5:
            Menu_AktivaIzvelne=Nakts;
            return;
          case 6:
            Menu_AktivaIzvelne=CO2;
            return;
          case 7:
            Menu_AktivaIzvelne=Tempratura;
            return;
          case 8:
            Menu_AktivaIzvelne=Barosana;
            return;
          case 9:
            Menu_AktivaIzvelne=SaktBarot;
            return;
          
        default:
          Menu_AktivaIzvelne=Galvena;
          break;
       }
       
       return;

    default:
      break;

  }

}

//***************************************************** Galvenā izvēlne  ^^^^^^^^^^  ZimeMenu ****************************************//
int ZimeMenu_Nobide;
int ZimeMenu_VecaisLogs;
int ZimeMenu_NobideVeca;
 String   menuPunkti[11]={
      "<--Atgriesties  ",
      "Datums un laiks ",
      "Saullekts       ", 
      "Diena           ", 
      "Saulriets       ",  
      "Nakts           ", 
      "CO2             ",
      "Temperatura     ",
      "Barosana        ",
      "Sakt barot      ",
      "<--Atgriesties  "} ; 

void ZimeMenuArGarumzimem(int rinda, int menuPunkts){
    lcd.setCursor(0,rinda);
    lcd.print(menuPunkti[menuPunkts]);

  
       switch (menuPunkts){        

          case 2: //Saullekts 
            lcd.setCursor(5,rinda);
            lcd.write(byte(GaraisMazais_ee));
            return;
          case 7: //Temperatura
             lcd.setCursor(8,rinda);
             lcd.write(byte(Garais_uu));
            return;
          case 8://Barosana
             lcd.setCursor(4,rinda);
             lcd.write(byte(Latviesu_sh));
            return;        
          case 9://Sākt barot 
             lcd.setCursor(1,rinda);
             lcd.write(byte(Garais_aa));
            return;          
          
        default:        
          break;
       }
}


void  ZimeMenu(){
       
   Menu_MinVertiba=0;
   Menu_MaxVertiba=ZimejamieMenuci;
   int x=counter /2;

   if(Menu_AktivaIzvelne!=Menu_IeprieksejaIzvelne){
      Menu_IeprieksejaIzvelne=Menu_AktivaIzvelne;
      counter=Menu_MinVertiba;
      x=1;
      counter=3;
      ZimeMenu_Nobide=-1;
      ZimeMenu_VecaisLogs=-10;
      ZimeMenu_NobideVeca=-10;
      lcd.clear();     
      lcd.cursor();
      lcd.blink();
   }
   
   if(x<0){
      x=0;
      counter=0;
   }

  if(x>Menu_MaxVertiba){
      x=Menu_MaxVertiba;
      counter=Menu_MaxVertiba*2 ;
   }
   
  if(x==Menu_MaxVertiba){
     ZimeMenu_Nobide=-1;      
   }
   if(x==0){
    ZimeMenu_Nobide=0;
   }
   int logs=x+ZimeMenu_Nobide;

   if(ZimeMenu_VecaisLogs!=logs) {
      ZimeMenu_VecaisLogs=logs;
      ZimeMenuArGarumzimem(0,logs);
      ZimeMenuArGarumzimem(1,logs+1);
      ZimeMenu_NobideVeca=-10;
   }

  if(ZimeMenu_NobideVeca!=ZimeMenu_Nobide){
    ZimeMenu_NobideVeca=ZimeMenu_Nobide;
    if(ZimeMenu_Nobide==0) 
        {        
          lcd.setCursor(0,0); 
        } else {
          lcd.setCursor(0,1); 
        }
   }
    
  Menu_KursoraPozicija=x;
  
  Serial.print("X=");
  Serial.println(x);

 
   Serial.print("ZimeMenu_Nobide=");
   Serial.println(ZimeMenu_Nobide);
}



//********************************************************  Paštaisīto burtu zīmēšna ***********************************************//

uint8_t Simboli_Zivs[8]  = {0x0,0x8,0x16,0x1f,0x16,0x4,0x0,0x0};
/*uint8_t Simboli_CO2_1[8]  = {B000000,
                     B000011,
                     B000100,
                     B000100,
                     B000100,
                     B000011,
                     B000000,
                     B000000};
                     */

uint8_t Simboli_Sh[8]  = {B10010,
                     B01100,
                     B01110,
                     B10000,
                     B01110,
                     B00001,
                     B01110,
                     B00000};

uint8_t Simboli_ee[8]  = {B01110,
                     B00000,
                     B01110,
                     B10001,
                     B11111,
                     B10000,
                     B01110,
                     B00000};


/*uint8_t Simboli_CO2_2[8]  = {B00000,
                     B01100,
                     B10010,
                     B10010,
                     B01100,
                     B00011,
                     B00011,
                     B00000};*/

uint8_t Simboli_CO2_2[8]  = {B00000,
                     B00000,
                     B00000,
                     B01100,
                     B10010,
                     B00100,                     
                     B11110,
                     B00000};

uint8_t Simboli_Saulekts[8]  = {B00000,
                     B00000,
                     B00100,
                     B10101,
                     B00000,
                     B01110,
                     B11111,
                     B00000};
uint8_t Simboli_Saule[8]  = {B00000,
                     B01110,
                     B11111,
                     B11111,
                     B11111,
                     B01110,
                     B00000,
                     B00000};

uint8_t Simboli_Saulriets[8]  = {B00000,
                     B01110,
                     B11111,
                     B00000,
                     B01110,
                     B00000,
                     B00100,
                     B00000};

uint8_t Simboli_Nakts[8]  = {B00110,
                     B01100,
                     B11000,
                     B11000,
                     B01111,
                     B00110,
                     B00000,
                     B00000};



// ===========================================================================  Parametri un to saglabāšana EEPROM atmiņā

void param_UzstadaNoklusetos(){
  Parametri parametriTemp;
  //Kopē visus struktūras datus atmiņā
  memcpy(&parametriTemp, &parametri, sizeof parametri );
}

void param_Nolasit(){
  //No programmas atmiņas vai pastāvīgo mainīgo atmiņas nolasa parametrus
   EEPROM.get(0, parametri);
   //Pārbauda vai parametri nav mainījušies
   if(parametri.versija!=ParametruVersija){
     param_UzstadaNoklusetos();
   }
}

void param_Saglabat(){
  // Pārbauda vai kaut kas ir mainijusies struktūrā
   Parametri parametriTemp;
  EEPROM.get(0, parametriTemp);
  if(    
      parametriTemp.Saullekts_Stundas == parametri.Saullekts_Stundas &&
      parametriTemp.Saullekts_Minutes == parametri.Saullekts_Minutes &&

      parametriTemp.Diena_Stundas ==  parametri.Diena_Stundas &&
      parametriTemp.Diena_Minutes ==  parametri.Diena_Minutes &&

      parametriTemp.Saulriets_Stundas ==  parametri.Saulriets_Stundas &&
      parametriTemp.Saulriets_Minutes == parametri.Saulriets_Minutes &&

      parametriTemp.Nakts_Stundas == parametri.Nakts_Stundas &&
      parametriTemp.Nakts_Minutes == parametri.Nakts_Minutes &&

      parametriTemp.BaroNedelasDienas == parametri.BaroNedelasDienas &&
      parametriTemp.Barosana_Stundas == parametri.Barosana_Stundas &&
      parametriTemp.Barosana_Minutes == parametri.Barosana_Minutes &&
      
      parametriTemp.CO2On_Stundas == parametri.CO2On_Stundas &&
      parametriTemp.CO2On_Minutes == parametri.CO2On_Minutes &&

      parametriTemp.CO2Off_Stundas == parametri.CO2Off_Stundas &&
      parametriTemp.CO2Off_Minutes == parametri.CO2Off_Minutes &&

      parametriTemp.CO2Izmatosana==parametri.CO2Izmatosana &&
      parametriTemp.Temp==parametri.Temp &&
      parametriTemp.TempTrauksme==parametri.TempTrauksme &&       

      parametriTemp.versija==parametri.versija 
  ){
    //Dati nav mainīt
    return;
  }
  
  //Saglabā parametrus - max 10 000 reizes progammas atmiņā vai 100 000 EEPROM
   EEPROM.put(0, parametri);  
}


 
 //****************************************************************************** DHT izstabas mitruma un temperatūras sensors ******************************************//
/*
DHT sensora inicialzēšana lai notestētu tā darebību
Iespējams DHT sensora fizisks bojājums, tāpēc atstāju oficiālo debuga kodu didliotekai, lai varētu notestēt, kad būs iespējams nomainīt sensoru

uint32_t delayMS;
void DHT_setup() {
 // Serial.begin(9600);
  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
   delayMS = sensor.min_delay / 1000;
 Serial.print  (F("Sensor delay: ")); Serial.println(delayMS);
   delay(delayMS*5);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
   
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }
}
*/
static bool measure_environment() {
  //https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHT_Unified_Sensor/DHT_Unified_Sensor.ino
    static unsigned long measurement_timestamp = millis();

    /* Measure once every four seconds. */
    if (millis() - measurement_timestamp > 8000ul) {
   
        sensors_event_t event;
        dht.temperature().getEvent(&event);

        if (isnan(event.temperature) ) {       
            temperaturaIstaba=0; 
        }
        else {
              temperaturaIstaba=event.temperature;
        }
        dht.humidity().getEvent(&event);
        if ( isnan(event.relative_humidity)) {              
              mitrumsIstaba=0; 
        }
        else {       
              mitrumsIstaba=event.relative_humidity;            
        }

   }
}
    
//***************************************************************   Vadības poga *************************************************//

 int btnStateLast=0;  
 unsigned long lastButtonPress = 0;

 bool Enkoders () {
  // Izmantos paraugs no https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
    // Read the button state
    int btnState = digitalRead(Enkoders_SW);

    //If we detect LOW signal, button is pressed
    if (btnState == LOW && btnStateLast != btnState) {     
    
        //if 50ms have passed since last LOW pulse, it means that the
        //button has been pressed, released and pressed again
        if (millis() - lastButtonPress > 50) {
            lastButtonPress = millis();
            btnStateLast=btnState;    
            return true;
        }

        // Remember last button press event
        lastButtonPress = millis();
        btnStateLast=btnState;
        return false;
	  }

  btnStateLast=btnState;
	// Put in a slight delay to help debounce the reading
	delay(1);
  return false;
}

void updateEncoder()
{
	// Paraugs no https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
  // Read the current state of CLK
	currentStateCLK = digitalRead(Enkoders_CLK);

	// If last and current state of CLK are different, then pulse occurred
	// React to only 1 state change to avoid double count
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

		// If the DT state is different than the CLK state then
		// the encoder is rotating CCW so decrement
		if (digitalRead(Enkoders_DT) != currentStateCLK) {
			counter --;    
		} else {
			// Encoder is rotating CW so increment
			counter ++;
			//currentDir ="CW";
		} 	 
	}

	// Remember last CLK state
	lastStateCLK = currentStateCLK;
}


/*********************************************************************************************************************************************************************************
                                                                        Barosana
*********************************************************************************************************************************************************************************/

void SaktBarotFunkcija() 
{
  PedejasBarosanasDatums=now;
  pedejaBarosana = now.day();
  if(barosanaOn == 0){
    barosanaOn = millis();
  }
}

static const unsigned long BarosanasCiklaVienaGradaIlgums = 50; //decimalsekundes
static const unsigned long BarosanasMotoraMaxPagriezienaLenkis = 180;
static const unsigned long BarosanasMotoraMaxPagriezienaLenkis_X_2 = BarosanasMotoraMaxPagriezienaLenkis*2;

void BarosanasVeiksana()
{
  if(barosanaOn == 0){
    return;
  }
  unsigned long mils=millis();
  if( barosanaOn >mils){
    //Notikusi milisekunšu nobīde - overflow, tāpēc sakam no sākuma
    barosanaOn=mils;
    return;    
  }
  unsigned long  t1 = mils- barosanaOn; // delata milisekundes
  unsigned long  x = t1 / BarosanasCiklaVienaGradaIlgums; 

  Serial.print("Motora lenkis:");
  Serial.println(x);
  
  if(x <= BarosanasMotoraMaxPagriezienaLenkis) {
    myservo.write(x);
  }
  else {
    if (x < BarosanasMotoraMaxPagriezienaLenkis_X_2){
    x = BarosanasMotoraMaxPagriezienaLenkis_X_2 - x; //atpakaļ ceļš
    myservo.write(x);
    }
    else {
      barosanaOn = 0; //barošana beidzas
      myservo.write(0);
    }
  }
}

/*********************************************************************************************************************************************************************************
                                                                        DarbibuIeslegsana
*********************************************************************************************************************************************************************************/



void DarbibuIeslegsana() 
{
  
    int minutesNoDienasSakuma = now.hour()*60+now.minute();
    //pārbauda vai jābaro
     if (pedejaBarosana != now.day()) {
          //" P O T C P S Sv 
          uint8_t nedelasDiena=now.dayOfTheWeek()-1;
          if(nedelasDiena=0){
            //Angļu svētdiena
            nedelasDiena=6;
          } else {
            nedelasDiena--;        
          }                   
          uint8_t ndParbauditajs=1;
          for(uint8_t nd=0; nd<7; nd++)
          {         
              uint8_t rez=BaroNedelasDienas & ndParbauditajs;
              if(rez!=0  && nd==nedelasDiena) {
                if( parametri.Barosana_Stundas*60 +  parametri.Barosana_Minutes< minutesNoDienasSakuma ){
                    SaktBarotFunkcija();
                 
                  }
              } 
              ndParbauditajs=ndParbauditajs*2;          
          } 
    }

    //giasma
    bool mazaGaisma = false;
    bool lielaGaisma = false;
    dienaaktsDala=0;

    if( parametri.Saullekts_Stundas*60 +  parametri.Saullekts_Minutes< minutesNoDienasSakuma ){  
      mazaGaisma=true;
      dienaaktsDala++;
    }

    if( parametri.Diena_Stundas*60 +  parametri.Diena_Minutes< minutesNoDienasSakuma ){
      lielaGaisma=true;
      dienaaktsDala++;
    }

    if( parametri.Saulriets_Stundas*60 +  parametri.Saulriets_Minutes< minutesNoDienasSakuma ){
      lielaGaisma=false;
      dienaaktsDala++;
    }

    if( parametri.Nakts_Stundas*60 +  parametri.Nakts_Minutes< minutesNoDienasSakuma ){
      mazaGaisma=false;
      dienaaktsDala=0;
    }

    if( mazaGaisma){
      digitalWrite(RelejaPins_Gaisma1, LOW );
    } else {
      digitalWrite(RelejaPins_Gaisma1, HIGH);
    }

    if( lielaGaisma){
      digitalWrite(RelejaPins_Gaisma2, LOW);
    } else {
      digitalWrite(RelejaPins_Gaisma2, HIGH);
    }
    
    //CO2
    co2=false;
    if(parametri.CO2Izmatosana){
        if( parametri.CO2On_Stundas*60 +  parametri.CO2On_Minutes< minutesNoDienasSakuma ){
          co2=true;
        }

        if( parametri.CO2Off_Stundas*60 +  parametri.CO2Off_Minutes< minutesNoDienasSakuma ){
          co2=false;
        }
    }
    if( co2){
      digitalWrite(RelejaPins_CO, LOW);
    } else {
      digitalWrite(RelejaPins_CO, HIGH);
    }


    //Temperatūra 

    if(tempAkvarija< parametri.Temp +parametri.TempTrauksme-2){
      digitalWrite(RelejaPins_Silditajs, LOW);
      silditajs=true;
    } 
    if(tempAkvarija> parametri.Temp +parametri.TempTrauksme){ {
      digitalWrite(RelejaPins_Silditajs, HIGH);
      silditajs=false;
    }

    if(
      tempAkvarija< parametri.Temp - parametri.TempTrauksme
      ||
      tempAkvarija> parametri.Temp + parametri.TempTrauksme
      ){
        tempTrauksme=true;
      }  else 
      {
        tempTrauksme=false;
      }
  }
}

/*********************************************************************************************************************************************************************************
                                                                        SETUPS
*********************************************************************************************************************************************************************************/

void setup()
{
  Serial.begin(9600);
  Serial.println("Advarija debuga informācija");
//Definē arduion kontaktu pielietojumu un uzstāda sākotnējās vērtības
    pinMode(Enkoders_SW,INPUT_PULLUP);
    pinMode(Enkoders_DT,INPUT);
    pinMode(Enkoders_CLK,INPUT);
    
    pinMode(RelejaPins_Gaisma1,OUTPUT);
    pinMode(RelejaPins_Gaisma2,OUTPUT);
    pinMode(RelejaPins_CO,OUTPUT);
    pinMode(RelejaPins_Silditajs,OUTPUT);
   
    digitalWrite(RelejaPins_Gaisma1, HIGH);
    digitalWrite(RelejaPins_Gaisma2, HIGH);
    digitalWrite(RelejaPins_CO, HIGH);
    digitalWrite(RelejaPins_Silditajs, HIGH);
 

    lastStateCLK = digitalRead(Enkoders_CLK);
    btnStateLast=digitalRead(Enkoders_SW);

  // Ievades pogas enkodera piesaiste pārtraukumu (interrupt) apstrādei izsaucot updateEncoder()
	// pārtraukums Nr 0  priekš pin 2
  // pārtraukums Nr 1 priekš  pin 3
	attachInterrupt(0, updateEncoder, CHANGE);
	attachInterrupt(1, updateEncoder, CHANGE);

 //LCD ekrana sagatavoošana
  lcd.init();   
  // Paštaisīto simbolu inicializēšana  
  lcd.createChar(Zivs,Simboli_Zivs);
  lcd.createChar(GaraisMazais_ee, Simboli_ee);
  lcd.createChar(Latviesu_sh, Simboli_Sh);
  lcd.createChar(3, Simboli_CO2_2);
  lcd.createChar(4, Simboli_Saulekts);
  lcd.createChar(5, Simboli_Saule);
  lcd.createChar(6, Simboli_Saulriets);
  lcd.createChar(7, Simboli_Nakts);
  
  //Ieslēdz ekrāna gaismu
  lcd.backlight();
  
  pulkstenis.begin();       // initialize rtc
 /*  Vienmreiz uzstāda un tad caur programmu, 
    pulkstenis.adjust(DateTime(2025,1,9,22,1,0)); 
 */
//Saglabāto parametru nolašišana
param_Nolasit();
 // DallasTemperature sensoru palaišana
sensors.begin();
//Servo motors 
myservo.attach(ServoPins);
myservo.write(0);

//Sākuma stavokļi mainīgajiem
tempAkvarija=0;
PedejasBarosanasDatums=new DateTime();
fonaGaisma=false;
fonaGaismaUzstaditaVertiba=true;

 
 // DHT nedarbojas, iespējams sensora bojājuma dēļ, tādēļ izkomentēts
 //DHT_setup();
 dht.begin();
}

/*********************************************************************************************************************************************************************************
                                                                        Cikls
*********************************************************************************************************************************************************************************/



void loop()
{
    
 // DHT nedarbijas, iespējams sensora bojājuma dēļ, tādēļ izkomentēts
 //measure_environment();
 
//Mēra temp akvārijā
sensors.requestTemperatures();     
tempAkvarija=sensors.getTempCByIndex(0);   

// nolasa pulksteni       
now = pulkstenis.now();

DarbibuIeslegsana();
Ekrans();
 if (Enkoders()){
    Poga_OnClick();
 }  
BarosanasVeiksana();
delay(2);   //Nogaida 2 milisekundes katrā ciklā, lai paspēj kontakti nodzirksteļot un nomierināties

/*
//  Izkoomentēts, lai prezentējot prototipu neizslēgtos ekrana apgasmojums
if(fonaGaismaUzstaditaVertiba!=fonaGaisma){
 //Maina fona gaismu, ja tā nav uztādīta atbilstoši vajadzībai
  if(fonaGaisma){
      lcd.backlight();
  } else {
     // lcd.noBacklight();
  }
  fonaGaismaUzstaditaVertiba=fonaGaisma;
}
*/
 
}
