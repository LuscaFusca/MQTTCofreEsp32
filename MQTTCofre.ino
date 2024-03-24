#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Arduino.h>

#define pinLedVermelho 19
#define pinLedVerde 18
#define pinBotao 13
#define pinServo 4

Servo servo;

String minhaSenha = "0000";
String mensagem = "";
String aux = "";
bool enviado = false;
bool tentativaSenha = false;
bool mqttStatus = 0;
int valorCorreto = 0;

const char* ssid = "connect_modulo"; // REDE
const char* password = "C0n3ct@!"; // SENHA

// MQTT Broker
const char *mqtt_broker = "broker.hivemq.com";  //Host do broket
const char *topic = "equipe01";                //Topico a ser subscrito e publicado
const char *topicResp = "equipe01/resp";    //Topico a ser subscrito e publicado
const char *mqtt_username = "";            //Usuario
const char *mqtt_password = "";            //Senha
const int mqtt_port = 1883;                //Porta
int qtdCaractere = 4;

//Objetos
WiFiClient espClient;
PubSubClient client(espClient);

//Prototipos
bool connectMQTT();
void callback(char *topic, byte * payload, unsigned int length);

void setup(void){
  Serial.begin(9600);

  // Conectar
  WiFi.begin(ssid, password);

  servo.attach(pinServo);
  pinMode(pinLedVermelho, OUTPUT);
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinBotao, INPUT_PULLUP);
  ativaLeds();
  ativaServo();

  //Aguardando conexão
  Serial.println();
  Serial.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //Envia IP através da UART
  Serial.println(WiFi.localIP());

  mqttStatus =  connectMQTT();
}

void loop() {
 static long long pooling  = 0;
  if (mqttStatus){
    
    client.loop();    

    if (millis() > pooling +1000){
      pooling = millis();
      String topico = String(topic) + "senha. Cofre Trancado, defina nova senha";
      if(extrairLadoEsquerdo(mensagem, 13) == String(topic) + "senha"){
        minhaSenha = extrairLadoDireito(mensagem, qtdCaractere);
      }else if(!digitalRead(pinBotao)){
        client.publish(topic, topico.c_str());
        resetCofre();
      }

      if(enviado && extrairLadoEsquerdo(mensagem, 13) != String(topic) + "senha" && valorCorreto != qtdCaractere){
        client.publish(topicResp, analisaSenha(extrairLadoDireito(mensagem, qtdCaractere), minhaSenha).c_str());
        enviado = false;
      }

      Serial.println(minhaSenha);
      Serial.println(mensagem);
      Serial.println(extrairLadoEsquerdo(mensagem, 13));
      Serial.println(String(topic) + "senha");

      if(minhaSenha == "0000"){
        valorCorreto = 0;
      }else if(valorCorreto == qtdCaractere){
        abrirTranca();
        String invasor = extrairLadoEsquerdo(mensagem, 8)+" invadiu o seu cofre!!!";
        client.publish(topic, invasor.c_str());
      }else if(valorCorreto == 3 && tentativaSenha){
        tentandoAbrir();
        tentativaSenha = false;
      }
    }   
  }
}



bool connectMQTT() {
  byte tentativa = 0;
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  do {
    String client_id = "BOBSIEN-";
    client_id += String(WiFi.macAddress());

    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Exito na conexão:");
      Serial.printf("Cliente %s conectado ao broker\n", client_id.c_str());
    } else {
      Serial.print("Falha ao conectar: ");
      Serial.print(client.state());
      Serial.println();
      Serial.print("Tentativa: ");
      Serial.println(tentativa);
      delay(2000);
    }
    tentativa++;
  } while (!client.connected() && tentativa < 5);

  if (tentativa < 5) {
    // publish and subscribe   
    client.publish(topic, "{testeOK}"); 
    client.subscribe(topic);
    return 1;
  } else {
    Serial.println("Não conectado");    
    return 0;
  }
}

void callback(char *topic, byte * payload, unsigned int length) {
  mensagem = "";
  enviado=true;
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
    mensagem += (char)payload[i];
  }
  Serial.println();
  Serial.println("-----------------------");
}

void ativaServo(){
  servo.write(180);
  delay(500);
  servo.write(20);
}

void ativaLeds(){
  digitalWrite(pinLedVermelho, HIGH);
  digitalWrite(pinLedVerde, HIGH);
  delay(500);
  digitalWrite(pinLedVerde, LOW);
}

void tentandoAbrir(){
  servo.write(20);
  digitalWrite(pinLedVerde, HIGH);
  delay(100);
  servo.write(0);
  digitalWrite(pinLedVerde, LOW);
  delay(100);
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
  minhaSenha = "0000";
  enviado = false;
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
  valorCorreto = 0;
  
  // Converter String para char array (C-style string)
  char senha_array[qtdCaractere+1];
  char minhaSenha_array[qtdCaractere+1];
  senha.toCharArray(senha_array, qtdCaractere+1); // 5 caracteres incluindo o terminador nulo
  minhaSenha.toCharArray(minhaSenha_array, qtdCaractere+1); // 5 caracteres incluindo o terminador nulo

  // Vetor para armazenar cada letra da palavra
  char numeroSenha[qtdCaractere];
  char numeroMinhaSenha[qtdCaractere];

  // Copiar cada letra para o vetor
  for (int i = 0; i < qtdCaractere; i++) {
    numeroSenha[i] = senha_array[i];
    numeroMinhaSenha[i] = minhaSenha_array[i];
  }

  for(int i=0; i<qtdCaractere; i++){
    for(int j=0; j<qtdCaractere; j++){
      if(numeroSenha[i] == numeroMinhaSenha[j]){
        bom++;
      }
      if(i == j && numeroSenha[i] == numeroMinhaSenha[j]){
        bom--;
        otimo++;
      }
    }
  }

  if(otimo == qtdCaractere){
    valorCorreto = qtdCaractere;
  }else if(otimo >= 3 || bom >= 3 || (otimo+bom) >= 3){
    valorCorreto = 3;
  }

  if(aux != senha){
    aux = senha;
    tentativaSenha=true;
  }

  return senha + ": "+ String(bom) + " Bom "+String(otimo)+" Otimo";
}