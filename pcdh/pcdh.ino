#include <SoftwareSerial.h>
#include <DHT.h>
//#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

SoftwareSerial esp8266(8, 9);
DHT dht(A2, DHT22);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int pulseAccount;

void setup() {
  Serial.begin(9600);
  esp8266.begin(115200);
  dht.begin();
  lcd.begin();

  //Mensagem de inicialização
  displayInfo("INICIANDO PCDH...", -1);
  
  setupESP8266(2000);
  setupSensorWaterFlow();

  connectToWiFi("", "", 2000);
}

void loop() {
  String hydroponic_id = "";
  String server = "";
  String path = "";
  String json;

  const size_t capacity = JSON_OBJECT_SIZE(10);
  DynamicJsonDocument root(capacity);

  root["hydroponic_id"] = hydroponic_id;
  root["humidity"] = dht.readHumidity();
  root["temperature"] = dht.readTemperature();
  root["waterflow"] = getWaterFlow();
  root["ph"] = getPh();
  root["ce"] = getCe();

  // Mostra dados no LCD
  displayInfo("TEMP. AMBIENTE", dht.readTemperature());
  displayInfo("UMIDADE", dht.readHumidity());
  displayInfo("PH", getPh());
  displayInfo("VAZAO", getWaterFlow());
  displayInfo("CE", getCe());

  serializeJson(root, json);

  String request;
  request.reserve(200);

  request = "POST " + path + " HTTP/1.1\r\n";
  request += "Host: " + server + "\r\n";
  request += "Accept: application/json\r\n";
  request += "Content-Length: ";
  request += json.length();
  request += "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "\r\n";
  request += json;

  Serial.println("Antes");
  Serial.println(request);

  displayInfo("ENVIANDO DADOS...", -1);
  
  startTCPConnection(server, 10000);
  sendRequest(request, 10000);
  closeTCPConnection(2000);

  delay(5000);
}

// ================= ESP8266 ===================
String atCommand(const String& command, const int timeout) {
  String response = "";
  esp8266.println(command);

  long int time = millis();

  while( (time+timeout) > millis() ) {
    while(esp8266.available()) {
      char c = esp8266.read();
      response += c;
    }
  }

  Serial.println(response);
  return response;
}

void setupESP8266(const int timeout) {
  atCommand("AT+RST", timeout);
  atCommand("AT+CWMODE=1", timeout);
}

void connectToWiFi(const String& ssid, const String& password, const int timeout) {
  String connect = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";
  atCommand(connect, 6000);
  atCommand("AT+CIFSR", timeout);
}

void startTCPConnection(const String& server, const int timeout) {
  String connect = "AT+CIPSTART=\"TCP\",\"" + server + "\",2695";
  atCommand(connect, timeout);
}

void closeTCPConnection(const int timeout) {
  atCommand("AT+CIPCLOSE", timeout);
  delay(5000);
}

String sendRequest(const String& request, const int timeout) {
  Serial.println("Depois");
  Serial.println(request);
  atCommand("AT+CIPSEND=" + String(request.length()), timeout);
  String response = atCommand(request, 6000);
  return response;
}

// =================== Sensor de Fluxo ===================
void setupSensorWaterFlow(){
  pinMode(4, INPUT);
  attachInterrupt(digitalPinToInterrupt(4), pulseInc, RISING);  
}

float getWaterFlow(){
  pulseAccount = 0;
  //sei();
  delay (1000);
  //cli();
  auto flow = pulseAccount / 5.5;
  return flow;
}

void pulseInc()  {
  pulseAccount++;
}

// =================== Sensor de PH ===================
float getPh(){
  auto measure = analogRead(A0);
  auto voltage = 5 / 1024.0 * measure; 
  auto ph = 7 + ((2.5 - voltage) / 0.18);
  return ph;
}

// =================== Sensor de CE ===================
float getCe(){
  // Implementar
  float ce = 23.3;
  return ce;
}

// =================== Diplay ===================
void displayInfo(const String& title, const float& info){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  if (info != -1 ){
    lcd.print(info);
  }
  delay(4000);
}
