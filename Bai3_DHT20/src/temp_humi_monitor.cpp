#include "temp_humi_monitor.h"
#include "coreiot_secrets.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <cstdio>
#include <cstring>

namespace {
constexpr uint8_t I2C3_SDA_PIN = 11;
constexpr uint8_t I2C3_SCL_PIN = 12;
constexpr uint8_t DHT20_ADDRESS = 0x38;
constexpr TickType_t READ_INTERVAL = pdMS_TO_TICKS(5000);
constexpr char BROKER_ADDRESS[] = "app.coreiot.io";
constexpr uint16_t BROKER_PORT = 1883;
constexpr char TELEMETRY_TOPIC[] = "v1/devices/me/telemetry";

DHT20 dht20;
WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

void scanI2CBus() {
    bool deviceFound = false;

    Serial.println("Scanning I2C bus...");
    for (uint8_t address = 1; address < 127; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("I2C device found at 0x%02X%s\n",
                          address,
                          address == DHT20_ADDRESS ? " (DHT20)" : "");
            deviceFound = true;
        }
    }

    if (!deviceFound) {
        Serial.println("No I2C devices found. Check SDA, SCL, power, and wiring.");
    }
}

bool connectToWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.printf("Connecting to Wi-Fi '%s'...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    constexpr uint8_t MAX_WIFI_ATTEMPTS = 30;
    for (uint8_t attempt = 0;
         attempt < MAX_WIFI_ATTEMPTS && WiFi.status() != WL_CONNECTED;
         ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection failed; will retry.");
        return false;
    }

    Serial.printf("Wi-Fi connected. IP: %s\n",
                  WiFi.localIP().toString().c_str());
    return true;
}

bool connectToBroker() {
    if (mqttClient.connected()) {
        return true;
    }

    Serial.println("Connecting to CoreIoT MQTT...");
    if (!mqttClient.connect(
            DEVICE_CLIENT_ID, ACCESS_USERNAME, ACCESS_TOKEN)) {
        Serial.printf("MQTT connection failed (state %d); will retry.\n",
                      mqttClient.state());
        return false;
    }

    Serial.println("Connected to CoreIoT MQTT.");
    return true;
}
}

void temp_humi_monitor(void *pvParameters) {
    (void)pvParameters;
    mqttClient.setServer(BROKER_ADDRESS, BROKER_PORT);

    const bool sensorReady = dht20.begin(I2C3_SDA_PIN, I2C3_SCL_PIN);
    Serial.printf("I2C3 initialized: SDA=GPIO%d, SCL=GPIO%d\n",
                  I2C3_SDA_PIN, I2C3_SCL_PIN);
    scanI2CBus();

    if (strlen(WIFI_SSID) == 0 || strlen(WIFI_PASSWORD) == 0) {
        Serial.println("Add WIFI_SSID and WIFI_PASSWORD to the root .env file.");
    }
    if (strlen(ACCESS_TOKEN) == 0 || strlen(ACCESS_USERNAME) == 0 ||
        strlen(DEVICE_CLIENT_ID) == 0) {
        Serial.println(
            "Add ACCESS_TOKEN, ACCESS_USERNAME, and DEVICE_CLIENT_ID to the root .env file.");
    }

    if (sensorReady) {
        Serial.println("DHT20 detected at address 0x38.");
    } else {
        Serial.println("DHT20 not detected at address 0x38. Check the wiring.");
    }

    while (true) {
        mqttClient.loop();
        const int status = dht20.read();
        if (status != DHT20_OK) {
            Serial.printf("DHT20 read failed (error %d).\n", status);
        } else {
            const float temperature = dht20.getTemperature();
            const float humidity = dht20.getHumidity();

            glob_temperature = temperature;
            glob_humidity = humidity;

            Serial.printf("Temperature: %.1f C, Humidity: %.1f%%\n",
                          temperature, humidity);

            const bool credentialsConfigured =
                strlen(WIFI_SSID) > 0 &&
                strlen(WIFI_PASSWORD) > 0 &&
                strlen(ACCESS_TOKEN) > 0 &&
                strlen(ACCESS_USERNAME) > 0 &&
                strlen(DEVICE_CLIENT_ID) > 0;

            if (credentialsConfigured &&
                connectToWiFi() && connectToBroker()) {
                char payload[96];
                snprintf(payload, sizeof(payload),
                         "{\"temperature\":%.2f,\"humidity\":%.2f}",
                         temperature, humidity);

                if (mqttClient.publish(TELEMETRY_TOPIC, payload)) {
                    Serial.println("DHT20 telemetry published to CoreIoT.");
                } else {
                    Serial.println("Failed to publish DHT20 telemetry.");
                }
            }
        }

        vTaskDelay(READ_INTERVAL);
    }
}
