const uint8_t pin_led = 2;
#define pin_temp 19
#define pin_water_count 4
#define pin_switch_1 18
#define pin_switch_2 5
#define pin_switch_3 17
#define pin_switch_4 16
String var; 


//===WiFI============================== 
const char* ssid = "";  
const char* password = "";


//====Telegram=========================
String BOTtoken = "";
String CHAT_ID = "";
int botRequestDelay = 1000; //задержка для проверки новых сообщений


//=====Biblio==================
#include <DallasTemperature.h>
#include <OneWire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>


//=====Server=============== 
AsyncWebServer server(80);

//=====Sensors===============
OneWire oneWireBus(pin_temp);
DallasTemperature sensors(&oneWireBus);
//======Adress sensors======================
DeviceAddress sensor1 = { 0x28, 0xA2, 0xA4, 0x76, 0xE0, 0x1, 0x3C, 0xBF };
DeviceAddress sensor2 = { 0x28, 0x6, 0xB4, 0x43, 0xD4, 0xB1, 0x68, 0x72 };
// DeviceAddress sensor3= { 0x28, 0xFF, 0xA0, 0x11, 0x33, 0x17, 0x3, 0x96 };


//=========Start Telegram==========
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
unsigned long lastTimeBotRan;
//==================================
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
 
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following command to get current readings.\n\n";
      welcome += "/readings \n";
      bot.sendMessage(chat_id, welcome, "");
    }
//здесь задается команда которую будет слушать контроллер и при прочтении ее отправит значения температуры и влажности
    if (text == "/temp1") {
      String readings = "Температура в комнате: " + String(sensors.getTempC(sensor1)) + " ºC \n";
      bot.sendMessage(chat_id, readings, "");
    }
     if (text == "/temp2") {
      String readings = "Температура в комнате: " + String(sensors.getTempC(sensor2)) + " ºC \n";
      bot.sendMessage(chat_id, readings, "");
    }  
     /*if (text == "/temp3") {
      String readings = "Температура в комнате: " + String(sensors.getTempC(sensor3)) + " ºC \n";
      bot.sendMessage(chat_id, readings, "");
    } */   
  }
}

//==========================
// Задаем статический IP-адрес:
IPAddress local_IP(192, 168, 1, 111);
// Задаем IP-адрес сетевого шлюза:
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // опционально
IPAddress secondaryDNS(8, 8, 4, 4); // опционально

//=============================

 String processor(const String& var){ //Функция которая передает переменные на сервер
   if (var == "TEMP1"){
   return String(sensors.getTempC(sensor1));
  }  
 if (var == "TEMP2"){
   return String(sensors.getTempC(sensor2));
  }  

 

 return String();
}

 

//===========================
void setup() {
  Serial.begin(115200);
  pinMode(pin_led, OUTPUT);
  pinMode(pin_temp, INPUT);
  pinMode(pin_water_count, INPUT);
  pinMode(pin_switch_1, OUTPUT);
  
  sensors.begin();
  
  if(!SPIFFS.begin(true)){
  Serial.println("An Error has occurred while mounting SPIFFS");
               //  "При монтировании SPIFFS произошла ошибка"
   return;
  }
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
               //  "Не удалось задать статический IP-адрес"
  } 
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  bot.sendMessage(CHAT_ID, "Bot Started", "");

  //URL для файла «style.css»:
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/style.css", "text/css");
  });
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ //Достаем index.html и отправляем на сервер
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

   server.on("/temp1", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(200, "text/html", String(sensors.getTempC(sensor1)));
  });
    server.on("/temp2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", String(sensors.getTempC(sensor2)));
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/temperature.html", String(), false);
  });
  
  server.begin();
   
 }
//========================================================

int LoopCounting = 0; //Задавание переменной для подсчета количества проходок лупа

void loop() {
  sensors.requestTemperatures();
//========Messages Telegram=================================
if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  //=========================================код для подсчета количества лупов
  //ВАЖНО!!!!!!! Попоробовать откладку кода для выявления багов
    if (LoopCounting >= 10) //Проверка количества проходок функции
    {
      //ЗДЕСЬ ДОЛЖЕН БЫТЬ КОД ДЛЯ ПРОВЕРКИ ПОКАЗАНИЯ ТЕМПЕРАТУРНЫХ ДАТЧИКОВ
      LoopCounting = 0; //Сброс количества пройденных циклов лупа
    }
    LoopCounting++; //Добавление проходки
  }
//===================================================
 
  
  Serial.print("Sensor 1: ");
  Serial.print(sensors.getTempC(sensor1));
  Serial.print("\n");
 
  Serial.print("Sensor 2: ");
  Serial.print(sensors.getTempC(sensor2));
  Serial.print("\n");
 
  Serial.print("Got IP: ");  Serial.print(WiFi.localIP());
  Serial.print("\n");
  
  delay (2000);

}

//======================================




//=======================================
