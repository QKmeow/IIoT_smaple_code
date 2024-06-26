enum CMD{
  CONNECT_STATUS = '0',
  READ_TEMP,
  READ_HUMIDITY
};

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("Hello NIU");

}

void loop() {
  // 如果有訊息傳入氣象站
  if (Serial.available()>0){
    int recVal = Serial.read(); //讀取buffer中的內容

    switch (recVal){
      case '0':
        //help CMD menu    
        Serial.println("Hello NIU");
        break;
      case '1':
        Serial.println("Tempture = 25 C");
        break;
      default:
        Serial.println("No CMD or CMD not exist");
        break;
    }
  }
}
