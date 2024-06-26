/*
 MQTT Publishing in the callback
  - connects to an MQTT server, providing username and password
  - subscribes to the topic "inTopic"
  - when a message is received, republishes it to "outTopic"

  This example shows how to publish messages within the
  callback function. The callback function header needs to
  be declared before the PubSubClient constructor and the
  actual callback defined afterwards.
  This ensures the client reference in the callback function
  is valid.

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-mqtt-upload-listen/
 */

#include <WiFi.h>
#include <PubSubClient.h>

char ssid[] = "Network_SSID";       // your network SSID (name)
char pass[] = "Password";           // your network password
int status = WL_IDLE_STATUS;        // Indicater of Wifi status

char mqttServer[]     = "mqttgo.io"; 
char clientId[]       = "amebaClient";
char clientUser[]     = "testuser";
char clientPass[]     = "testpass";
char publishTopic[]   = "NIUclass/";
char publishPayload[] = "hello world";
char subscribeTopic[] = "NIUHome/";
uint32_t MQTTLastPublishTime=0;    
uint32_t MQTTPublishInterval = 1000;

enum activity{
  CONNECT_STATUS = '0',
  READ_TEMP,
  READ_HUMIDITY,
  READ_LIGHT

};

float t = 25;
float h = 80;
float lux = 1000;
String msgStr = "{";


void callback(char* topic, byte* payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(mqttServer, 1883, callback, wifiClient);


void composeJson(String name,float data,int decimal){
  if (msgStr.length() > 1) { // 1 因為一開始msgStr已經有一個 '{'
    // 如果不是第一個項目，加入逗號分隔符 json data
    msgStr += ",";
  }
  msgStr += "\"" + name + "\":" + String(data, decimal);
}

void pubPayload(){
  msgStr += "}"; // 加入 JSON 結尾標記 '}'
  byte arrSize = msgStr.length() + 1; // 計算消息長度 + 空字符 '\0'
  char json[arrSize];  
  msgStr.toCharArray(json, arrSize);  // 將String轉換為字符數組
  client.publish(publishTopic, json); // 發布MQTT消息
  msgStr = "{";// 清空消息字符串
}

void CLI(int cmd){
  switch (cmd){
    case CONNECT_STATUS:
        Serial.println("Hello NIU");
        break;
    case READ_TEMP:
        // t = dht.readTemperature();
        Serial.print(F("Temperature: "));
        Serial.print(t);
        Serial.println(F("°C"));
        composeJson("temp",t,1);
        break;
    case READ_HUMIDITY:
        // h = dht.readHumidity();
        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.println(F("%"));
        composeJson("hum",h,1);
        break;    
    case READ_LIGHT:
        // 從光度計讀取光照強度（單位為Lux）
        // lux = lightMeter.readLightLevel();
        Serial.print("Light: ");
        Serial.print(lux);
        Serial.println("lux");
        break; 
    default:
        Serial.println("No CMD or CMD not exist");
        break;
  }
}
// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println();

    // In order to republish this payload, a copy must be made
    // as the orignal payload buffer will be overwritten whilst
    // constructing the PUBLISH packet.

    // Allocate the correct amount of memory for the payload copy
    byte* p = (byte*)(malloc(length));
    if (p == nullptr) {
        Serial.println("Failed to allocate memory");
        return;
    }
    // Copy the payload to the new buffer
    memcpy(p, payload, length);
    client.publish(publishTopic, p, length);
    // 打印 p 的内容
    for (unsigned int i = 0; i < length; i++) {
      Serial.println(p[i]);
      CLI(p[i]);
    }
    pubPayload();
    // Free the memory
    free(p);
}

void reconnect() {
    // Loop until we're reconnected
    while (!(client.connected())) {
        Serial.print("\r\nAttempting MQTT connection...");
        // Attempt to connect
        if (client.connect(clientId)) {
            Serial.println("connected");
            //Once connected, publish an announcement and resubscribe
            client.publish(publishTopic, publishPayload);
            client.subscribe(subscribeTopic);
        } else {
            Serial.println("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            //Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    // wait for serial port to connect.
    while (!Serial) {
        ;
    }

    //Attempt to connect to WiFi network
    while (status != WL_CONNECTED) {
        Serial.print("\r\nAttempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    if (client.connect(clientId, clientUser, clientPass)) {
        client.publish(publishTopic, publishPayload);
        client.subscribe(subscribeTopic);
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    if ((millis() - MQTTLastPublishTime) >= MQTTPublishInterval) {

      if (client.publish(publishTopic, publishPayload)) {
              Serial.println("Message published successfully");
          } else {
              Serial.println("Message publishing failed");
          }

          MQTTLastPublishTime = millis(); //更新最後傳輸時間
    }


}
