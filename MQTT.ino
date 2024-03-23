#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Arduino.h>

#define pinLedVermelho 19
#define pinLedVerde 18
#define botao 13

Servo servo;

String minhaSenha = "";
int valorCorreto = 0;

const char* ssid = "connect_modulo";
const char* password = "C0n3ct@!";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "equipeA";
const char* mqtt_topic_resp = "equipeA/resp";

unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 5000;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_INTERVAL 5000 // Enviar uma mensagem a cada 5 segundos

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado à rede WiFi");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String mensagem = "";
  String resposta = "";
  
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");

  if (strcmp(topic, mqtt_topic) == 0) {
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      mensagem += (char)payload[i];
    }

    if(extrairLadoEsquerdo(mensagem, 5) == "senha"){
      minhaSenha = extrairLadoDireito(mensagem, 4);
    }else if(mensagem == "reset"){
      client.publish(mqtt_topic, resposta.c_str(), true);
      resetCofre();
    }

    client.publish(mqtt_topic_resp, analisaSenha(mensagem, minhaSenha).c_str(), true);
    Serial.println(analisaSenha(mensagem, minhaSenha));
    
    if(valorCorreto == 4){
      abrirTranca();
    }
    
  }
  Serial.println();
  Serial.println(minhaSenha);
  Serial.println(valorCorreto);
}

void reconnect() {
  if (client.connected()) {
    return;
  }

  if (millis() - lastReconnectAttempt < reconnectInterval) {
    return;
  }

  lastReconnectAttempt = millis();

  while (!client.connected()) {
    Serial.print("Tentando se reconectar ao MQTT Broker...");
    
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado");
      client.subscribe(mqtt_topic);
      client.subscribe(mqtt_topic_resp);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 0,5 segundos");
      delay(500);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  servo.attach(4);
  ativaServo();
  pinMode(pinLedVermelho, OUTPUT);
  pinMode(pinLedVerde, OUTPUT);
  pinMode(botao, INPUT_PULLUP);
  ativaLeds();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > MSG_INTERVAL) {
    lastMsg = now;
  }
}

void ativaServo(){
  servo.write(180);
  delay(1000);
  servo.write(20);
}

void ativaLeds(){
  digitalWrite(pinLedVermelho, HIGH);
  digitalWrite(pinLedVerde, HIGH);
  delay(1000);
  digitalWrite(pinLedVerde, LOW);
}

void tentandoAbrir(){
    servo.write(20);
    digitalWrite(pinLedVerde, HIGH);
    delay(100);
    servo.write(0);
    digitalWrite(pinLedVerde, LOW);
}

void abrirTranca(){
  servo.write(180);
  digitalWrite(pinLedVermelho, LOW);
  digitalWrite(pinLedVerde, HIGH);
}

void resetCofre(){
  ativaServo();
  ativaLeds();
}

String extrairLadoEsquerdo(const String& palavra, int tamanho) {
    return palavra.substring(0, tamanho);
}

String extrairLadoDireito(const String& palavra, int tamanho) {
    int tamanhoTotal = palavra.length();
    return palavra.substring(tamanhoTotal - tamanho);
}

String analisaSenha(String senha, String minhaSenha){
  int bom = 0;
  int otimo = 0;
  // Converter String para char array (C-style string)
  char senha_array[5];
  char minhaSenha_array[5];
  senha.toCharArray(senha_array, 5); // 5 caracteres incluindo o terminador nulo
  minhaSenha.toCharArray(minhaSenha_array, 5); // 5 caracteres incluindo o terminador nulo

  // Vetor para armazenar cada letra da palavra
  char numeroSenha[4];
  char numeroMinhaSenha[4];

  // Copiar cada letra para o vetor
  for (int i = 0; i < 4; i++) {
    numeroSenha[i] = senha_array[i];
    numeroMinhaSenha[i] = minhaSenha_array[i];
  }

  for(int i=0; i<4; i++){
    for(int j=0; j<4; j++){
      if(numeroSenha[i] == numeroMinhaSenha[j]){
        bom++;
      }
      if(i == j && numeroSenha[i] == numeroMinhaSenha[j]){
        bom--;
        otimo++;
      }
    }
  }

  if(otimo == 4){
    valorCorreto = otimo;
  }

  return " " + String(bom)+" Bom "+String(otimo)+" Otimo";
}