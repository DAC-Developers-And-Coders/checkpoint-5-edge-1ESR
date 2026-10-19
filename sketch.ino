#include <PubSubClient.h>
#include <WiFi.h>
#include <DHT.h>
#include <ctime>

#define DHT_PIN 19
#define DHT_TYPE DHT22

#define LDR_PIN 34
#define LED_RED 18
#define LED_YELLOW 17
#define LED_GREEN 16

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const char* MQTT_SERVER = "4.168.234.137";
const int MQTT_PORT = 1883;

const char* DEVICE_ID = "device014";

String topicAttrs;
String topicLuminosity;
String topicCommand;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastReading = 0;
const unsigned long interval = 2000;

const int BRIGHT_LIMIT = 30;
const int HEAT_LIMIT = 15;
const int COLD_LIMIT = 12;
const int WET_LIMIT = 80;
const int DRY_LIMIT = 60;

int alert;

void setup() {
    Serial.begin(115200);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    turnOffLeds();
    dht.begin();

    topicAttrs = "/TEF/" + String(DEVICE_ID) + "/attrs";
    topicCommand = "/TEF/" + String(DEVICE_ID) + "/cmd";

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Conectando ao Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWi-Fi conectado!");

    configTime(-3 * 3600, 0, "pool.ntp.org", "time.google.com");

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback);
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        delay(500);
        return;
    }

    if (!client.connected()) {
        reconnectMQTT();
    }

    digitalWrite(LED_RED, (alert == 3));
    digitalWrite(LED_YELLOW, (alert == 2));
    digitalWrite(LED_GREEN, (alert == 1));

    client.loop();

    unsigned long now = millis();

    if (now - lastReading >= interval) {
        lastReading = now;

        readSensors();
    }
}

void turnOffLeds() {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
}

void updateLeds(int luminosity, int temperature, int humidity) {
    turnOffLeds();

    if(luminosity > BRIGHT_LIMIT || temperature > HEAT_LIMIT + 4 || temperature < COLD_LIMIT - 4
        || humidity < DRY_LIMIT || humidity > WET_LIMIT){
      alert = 3;
    } else if((luminosity >= BRIGHT_LIMIT - 10 && luminosity <= BRIGHT_LIMIT) || 
            (temperature >= COLD_LIMIT - 4 && temperature < COLD_LIMIT) ||
            (temperature > HEAT_LIMIT && temperature <= HEAT_LIMIT + 4) || 
            (humidity >= DRY_LIMIT && humidity <= DRY_LIMIT + 5) ||
            (humidity >= WET_LIMIT - 5 && humidity <= WET_LIMIT)){
      alert = 2;
    } else if(luminosity < BRIGHT_LIMIT - 10 || 
            (temperature >= COLD_LIMIT && temperature <= HEAT_LIMIT) ||
            (humidity >= DRY_LIMIT && humidity <= WET_LIMIT)){
      alert = 1;
    }
}

String getTimestamp() {
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 1000)) {
        return "";
    }

    char buffer[25];

    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    return String(buffer) + "-03:00";
}

void readSensors() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    int luminosity = map(analogRead(LDR_PIN), 0, 4095, 100, 0);
    luminosity = constrain(luminosity, 0, 100);

    Serial.printf("Luminosidade: %d%%\n", luminosity);

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Falha ao ler o temperatura e umidade do DHT11.");
        return;
    }

    updateLeds(luminosity, temperature, humidity);

    Serial.printf("Temperatura: %.1f °C - Umidade: %.1f%%\n", temperature, humidity);

    String timestamp = getTimestamp();

    Serial.print("Timestamp: ");
    Serial.println(timestamp);

    if (timestamp.isEmpty()) {
        Serial.println("Timestamp indisponível; leitura não publicada.");
        return;
    }

    String payload = timestamp + "|t|" + String(temperature, 1) + "|h|" + String(humidity, 1) + 
    "|l|" + String(luminosity);

    if (client.connected()) {
        if (client.publish(topicAttrs.c_str(), payload.c_str())) {
            Serial.print("Atributos publicados: ");
            Serial.println(payload);
        } else {
            Serial.println("Falha ao publicar dados.");
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Comando recebido: ");
    Serial.println(topic);
    
    String message;

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println(message);
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.println("Tentando conectar ao MQTT...");

        String clientId = "ESP32_Wokwi_" + String(random(0xffff), HEX);

        if (client.connect(clientId.c_str())) {
            Serial.println("MQTT conectado!");
            client.subscribe(topicCommand.c_str());
        } else {
            Serial.print("Falha MQTT, codigo: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}
