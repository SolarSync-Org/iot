#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configurações do WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configurações do MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

// Configurações dos pinos
#define DHTPIN 26
#define DHTTYPE DHT22
#define LDR_PIN 35

// Constantes para cálculo do LDR
const float GAMMA = 0.7;
const float RL10 = 50;

// Inicialização dos objetos
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

// Variáveis para armazenar as leituras
float temperature = 0;
float humidity = 0;
float lux = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

float calculateLux(int analogValue) {
  const float VOLT_PER_STEP = 3.3f / 4095.0f;
  float voltage = analogValue * VOLT_PER_STEP;
  float resistance = (3.3f * 10000.0f / voltage) - 10000.0f;
  
  // Adicione limites para evitar valores irreais
  if (resistance < 0) resistance = 0;
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  
  // Limite superior para lux
  if (lux > 100000) lux = 100000;
  
  return lux;
}

void readSensors() {
  // Leitura do DHT22
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  // Verifica se as leituras do DHT22 são válidas
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Falha na leitura do sensor DHT22!");
    return;
  }

  // Leitura do LDR e conversão para lux
  int ldrValue = analogRead(LDR_PIN);
  lux = calculateLux(ldrValue);
}

void publishData() {
  char tempString[8];
  char humString[8];
  char luxString[8];

  dtostrf(temperature, 1, 2, tempString);
  dtostrf(humidity, 1, 2, humString);
  dtostrf(lux, 1, 2, luxString);

  client.publish("solarsync/temperature", tempString);
  client.publish("solarsync/humidity", humString);
  client.publish("solarsync/light", luxString);
}

void setup() {
  Serial.begin(115200);
  
  // Inicializa o sensor DHT
  dht.begin();
  
  // Configura o pino do LDR como entrada
  pinMode(LDR_PIN, INPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  readSensors();
  
  // Exibe as leituras no Serial Monitor
  Serial.println("\n=== Leituras dos Sensores ===");
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Umidade: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Luminosidade: ");
  Serial.print(lux);
  Serial.println(" lux");
  Serial.println("===========================");
  
  publishData();
  
  delay(5000);
}