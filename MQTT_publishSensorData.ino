// 引入所需的函式庫
#include <WiFi.h>            // 用來處理WiFi連線的函式庫
#include <Wire.h>            // 用來處理I2C通訊的函式庫
#include <BH1750.h>          // 用於BH1750光照強度感測器的函式庫
#include <PubSubClient.h>    // 處理MQTT通訊的函式庫
#include "DHT.h"             // 用於DHT溫濕度感測器的函式庫

// 設定連接感測器的腳位
#define DHTPIN 2            // DHT感測器的腳位
#define DHTTYPE DHT11       // 使用的DHT型號是DHT22

// 設定WiFi參數
char ssid[] = "SSIDname";        // WiFi名稱
char pass[] = "wifipassword";   // WiFi密碼
int status = WL_IDLE_STATUS; // WiFi連線狀態初始值

// 設定MQTT參數
char mqttServer[] = "mqttgo.io";           // MQTT服務器位址
char clientId[] = "";                      // MQTT客戶端ID
char publishTopic[] = "niubmte/iotclass/"; // MQTT發布主題
char HumTopic[] = "YourTopic/class/Hum";   // MQTT發布濕度的主題
char publishPayload[] = "TEST";            // MQTT發布的消息內容
char subscribeTopic[] = "niubmte/iotclass/";// MQTT訂閱的主題

String msgStr = "";                        // 暫存MQTT消息的字符串

// 設定間隔時間
unsigned long prevMillis = 0;              // 上次發送資料的時間戳記
const long interval = 5000;                // 發送資料的時間間隔，設為5秒

// 初始化客戶端物件
WiFiClient wifiClient;
PubSubClient client(wifiClient);
DHT dht(DHTPIN, DHTTYPE);                  // 初始化DHT感測器
BH1750 lightMeter;                         // 初始化光照強度感測器

void reconnect() {
  // 檢查是否連接到MQTT，若未連接則嘗試重連
  while (!client.connected()) {
      Serial.print("嘗試連接MQTT服務...");
      if (client.connect(clientId)) {      // 嘗試連接MQTT服務器
          Serial.println("已連接");
          client.publish(publishTopic, publishPayload);  // 發布消息
          client.subscribe(subscribeTopic);              // 訂閱主題
      } else {
          Serial.println("連接失敗，錯誤碼=");
          Serial.print(client.state());
          Serial.println(" 5秒後重試");
          delay(5000);  // 等待5秒後重試
      }
  }
}

void setup() {
    Serial.begin(115200);                  // 啟動串列通訊，設定波特率為115200
    dht.begin();                           // 啟動DHT感測器
    Wire.begin();                          // 啟動I2C通訊
    lightMeter.begin();                    // 啟動光照強度感測器
   
    while (!Serial) { ; }                  // 等待串列埠連接

    while (status != WL_CONNECTED) {       // 當WiFi未連接時嘗試連接
        Serial.print("\n正在嘗試連線至SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);   // 連接到WiFi網絡
        delay(10000);                      // 等待10秒
    }

    client.setServer(mqttServer, 1883);    // 設定MQTT服務器及端口
    delay(1500);                           // 延遲1.5秒，等待硬件準備好
}

void loop() {
    if (!client.connected()) {
        reconnect();                      // 確保MQTT連接正常
    }

    if (millis() - prevMillis > interval) { // 檢查是否達到發送間隔
      prevMillis = millis();              // 更新最後發送時間
      
      float hum = dht.readHumidity();       // 讀取濕度，浮點數
      float temp = dht.readTemperature();   // 讀取溫度，浮點數
      float lux = lightMeter.readLightLevel(); // 讀取光照強度
      float umol = lux * 0.0171 + 0.1008; // 將勒克斯轉換成微摩爾值

      
      // 建立要發布的MQTT消息，使用JSON格式
      String msgStr = "{\"temp\":" + String(temp,1) + ",\"humid\":" + String(hum,1) + ",\"light\":" + String(umol, 2) + "}";
      byte arrSize = msgStr.length() + 1; // 計算消息長度
      char json[arrSize];                 // 創建字符數組以存儲消息
      msgStr.toCharArray(json, arrSize);  // 將String轉換為字符數組
      client.publish(publishTopic, json); // 發布MQTT消息
      Serial.println(msgStr);             // 在串列監視器中顯示消息
      msgStr = "";                        // 清空消息字符串以便下一次使用
    }
}
