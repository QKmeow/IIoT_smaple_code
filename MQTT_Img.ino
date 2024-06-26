/*

 MQTT Basic example

 This sketch demonstrates the basic capabilities of the library.
  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out any messages it receives. It assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-mqtt-upload-listen/
 */

/*
https://twgo.io/xwznc
本範例建立兩個頻道
頻道1：configV，VIDEO_FHD且帶有音訊提供RTSP視訊
頻道2：configLine，640x480提供LINE影像及MQTT
*/

#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "RTSP.h"
#include <PubSubClient.h>  //請先安裝PubSubClient程式庫

// Default preset configurations for each video channel:
// Channel 0 : 1920 x 1080 30FPS H264
// Channel 1 : 1280 x 720  30FPS H264
// Channel 2 : 1280 x 720  30FPS MJPEG
VideoSetting configV(VIDEO_FHD, CAM_FPS, VIDEO_H264, 0);
VideoSetting configMQTT(800, 600, 10, VIDEO_JPEG, 1);

// Default audio preset configurations:
// 0 :  8kHz Mono Analog Mic
// 1 : 16kHz Mono Analog Mic
// 2 :  8kHz Mono Digital PDM Mic
// 3 : 16kHz Mono Digital PDM Mic
AudioSetting configA(0);

Audio audio;
AAC aac;
RTSP rtsp;
RTSP rtsp2;

StreamIO audioStreamer(1, 1);  // 1 Input Audio -> 1 Output AAC
StreamIO avMixStreamer(2, 1);  // 2 Input Video + Audio -> 1 Output RTSP

char ssid[] = "Network_SSID";       // your network SSID (name)
char pass[] = "Password";           // your network password
int status = WL_IDLE_STATUS;        // Indicater of Wifi status

char mqttServer[]     = "mqttgo.io"; 
char clientId[]       = "amebaClient";
char publishTopic[]   = "outTopic";
char publishPayload[] = "hello world";
char MQTTPubTopic1[] = "class205/pic1";  //推播主題1:即時影像
char subscribeTopic[] = "inTopic";

uint32_t MQTTLastPublishTime;                         //此變數用來記錄推播時間
uint32_t MQTTPublishInterval = 100;                   //每1秒推撥4-5次影像

uint32_t img_addr = 0;
uint32_t img_len = 0;

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)(payload[i]));
    }
    Serial.println();
}

WiFiClient wifiClient;
PubSubClient client(wifiClient);

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
    // Configure camera video channel with video format information
    // Adjust the bitrate based on your WiFi network quality
    //configV.setBitrate(2 * 1024 * 1024);     // Recommend to use 2Mbps for RTSP streaming to prevent network congestion
    Camera.configVideoChannel(0, configV);
    Camera.configVideoChannel(1, configMQTT);
    Camera.videoInit();
    // Configure audio peripheral for audio data output
    
    audio.configAudio(configA);
    audio.begin();
    // Configure AAC audio encoder
    aac.configAudio(configA);
    aac.begin();

    rtsp.configVideo(configV);
    rtsp.configAudio(configA, CODEC_AAC);
    rtsp.begin();
    // Configure StreamIO object to stream data from audio channel to AAC encoder
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(aac);
    if (audioStreamer.begin() != 0) {
      Serial.println("StreamIO link start failed");
    }

    // Configure StreamIO object to stream data from video channel and AAC encoder to rtsp output
    avMixStreamer.registerInput1(Camera.getStream(0));
    avMixStreamer.registerInput2(aac);
    avMixStreamer.registerOutput(rtsp);
    if (avMixStreamer.begin() != 0) {
      Serial.println("StreamIO link start failed");
    }

    // Start data stream from video channel
    Camera.channelBegin(0);
    Camera.channelBegin(1);
    delay(1000);
    printInfo();



    client.setServer(mqttServer, 1883);
    client.setCallback(callback);

    //Allow Hardware to sort itself out
    delay(1500);
}

void loop() {
    if (!(client.connected())) {
        reconnect();
    }
    // client.loop();
    if ((millis() - MQTTLastPublishTime) >= MQTTPublishInterval) {
    String payload = SendImageMQTT();
    Serial.println(payload);
    MQTTLastPublishTime = millis();  //更新最後傳輸時間
  }
  delay(10);
}


String SendImageMQTT() {
  int buf = 8192;
  //找到照片起始位置及大小
  Camera.getImage(1, &img_addr, &img_len);
  //int ps = 512;
  //開始傳遞影像檔，批次傳檔案
  client.beginPublish(MQTTPubTopic1, img_len, false);

  uint8_t* fbBuf = (uint8_t*)img_addr;
  size_t fbLen = img_len;
  for (size_t n = 0; n < fbLen; n = n + buf) {
    if (n + buf < fbLen) {
      client.write(fbBuf, buf);
      fbBuf += buf;
    } else if (fbLen % buf > 0) {
      size_t remainder = fbLen % buf;
      client.write(fbBuf, remainder);
    }
  }
  boolean isPublished = client.endPublish();
  if (isPublished) return "MQTT傳輸成功";
  else return "MQTT傳輸失敗，請檢查網路設定";
}

void printInfo(void) {
    Serial.println("------------------------------");
    Serial.println("- Summary of Streaming -");
    Serial.println("------------------------------");
    Camera.printInfo();

    IPAddress ip = WiFi.localIP();

    Serial.println("- RTSP -");
    Serial.print("rtsp://");
    Serial.print(ip);
    Serial.print(":");
    rtsp.printInfo();

    Serial.println("- Audio -");
    audio.printInfo();
}
