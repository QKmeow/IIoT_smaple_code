/******引用函示庫*****/
// 引用 I2C Wire 和 BH1750 函式庫
#include <Wire.h>
#include <BH1750.h>
#include "DHT.h"

/******定義常數*****/
// DHT pin and type
#define DHTPIN 8
#define DHTTYPE DHT11    
//RGB pin
#define Red 9
#define Green 10
#define Blue 11


/******定義變數*****/
enum CMD{
  CONNECT_STATUS = '0',
  READ_TEMP,
  READ_HUMIDITY,
  READ_LIGHT,
  MONITOR_MOD
};

float lux = 0;
float t = 0;
float h = 0;
float light_upper_limit = 100.0; // 假設的光量上限
float light_lower_limit = 10.0;  // 假設的光量下限

/******初始化*****/
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();
    // 初始化 I2C 通訊
    Wire.begin();
    // 初始化光度計
    lightMeter.begin();
    //初始化LED 腳位
    pinMode(LED_Red, OUTPUT);
    pinMode(LED_Green, OUTPUT);
    pinMode(LED_Blue, OUTPUT);


    Serial.println("Hello NIU");

}

void loop() {
    // 如果有訊息傳入氣象站
    if (Serial.available()>0){
        int recVal = Serial.read(); //讀取buffer中的內容

    switch (recVal){
        case CONNECT_STATUS:
            Serial.println("Hello NIU");
            break;
        case READ_TEMP:
            t = dht.readTemperature();
            Serial.print(F("Temperature: "));
            Serial.print(t);
            Serial.println(F("°C"));
            break;
        case READ_HUMIDITY:
            h = dht.readHumidity();
            Serial.print(F("Humidity: "));
            Serial.print(h);
            Serial.println(F("%"));
            break;    
        case READ_LIGHT:
            // 從光度計讀取光照強度（單位為Lux）
            lux = lightMeter.readLightLevel();
            Serial.print("Light: ");
            Serial.print(lux);
            Serial.println("lux");
            break; 
        case MONITOR_MOD:
        //當沒有傳入指令時持續監測
            while(Serial.available() == 0){
                lux = lightMeter.readLightLevel();
                Serial.print("Light: ");
                Serial.print(lux);
                Serial.println("lux");
                
                if(lux > ligt_upper_limit){
                    digitalWrite(LED_Red, HIGH);
                    digitalWrite(LED_Blue, LOW);
                    digitalWrite(LED_Green, LOW);
                    Serial.println("Light level is above the upper limit!");
                }
                else if (lux < ligt_lower_limit){
                    digitalWrite(LED_Red, LOW);
                    digitalWrite(LED_Blue, HIGH);
                    digitalWrite(LED_Green, LOW);
                    Serial.println("Light level is below the lower limit!");
                    
                }
                else{
                    digitalWrite(LED_Red, LOW);
                    digitalWrite(LED_Blue, LOW);
                    digitalWrite(LED_Green, HIGH);
                }
                delay(2000);
            }
            break;
        default:
            Serial.println("No CMD or CMD not exist");
            
            Serial.println(String(CONNECT_STATUS) + ", CONNECT_STATUS: Check if connected");
            Serial.println(String(READ_TEMP) + ", READ_TEMP: Read temperature");
            Serial.println(String(READ_HUMIDITY) + ", READ_HUMIDITY: Read humidity");
            Serial.println(String(READ_LIGHT) + ", READ_LIGHT: Read light intensity");
            Serial.println(String(MONITOR_MOD) + ", MONITOR_MOD: Monitor mode");
            break;
    }
  }
}
