#include <WiFi.h>
#include <PubSubClient.h>

#define BUZZ 4
#define GAS_D 13
#define GAS_A 12

const char* ssid = "#NOME_DA_REDE#";
const char* password = "#SENHA_DA_REDE#";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

WiFiServer server(80);

bool buzzerAtivo = true;
bool mensagemEnviada = false;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == 'A') {
    buzzerAtivo = true;
    snprintf(msg, MSG_BUFFER_SIZE, "Alarme está ativado!");
    Serial.print("Publica mensagem:");
    Serial.println(msg);
    client.publish("alarme_gas/ativo", msg);
  }

  if ((char)payload[0] == 'D') {
    buzzerAtivo = false;
    snprintf(msg, MSG_BUFFER_SIZE, "Alarme está desativado!");
    Serial.print("Publica mensagem:");
    Serial.println(msg);
    client.publish("alarme_gas/ativo", msg);
  }
}

void connect() {
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Conectando-se à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");

    String clientId = "ALARME_GAS_DISPOSITIVO";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado.");

      client.subscribe("alarme_gas/ativar");
    } else {
      Serial.print("Tentativa falha: ");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");

      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  pinMode(GAS_D, INPUT);
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, LOW);

  connect();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  int gas = digitalRead(GAS_D);

  if (gas == 0 && buzzerAtivo) {
    digitalWrite(BUZZ, HIGH);
    delay(1000);
    digitalWrite(BUZZ, LOW);
    delay(2000);
  }

  if (gas == 0 && !mensagemEnviada) {
    snprintf(msg, MSG_BUFFER_SIZE, "Vazamento de gás detectado.");
    client.publish("alarme_gas/gas_detectado", msg);
    mensagemEnviada = true;
  } else {
    mensagemEnviada = false;
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}
