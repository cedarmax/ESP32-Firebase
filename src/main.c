#include <stdio.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "cJSON.h"

#define WIFI_SSID "Xcover"
#define WIFI_PASS "wifipass"
#define FIREBASE_HOST "https://solar-control-app-default-rtdb.firebaseio.com/"
#define FIREBASE_API_KEY "AIzaSyC1i1-XbvHzPnnm1ZRSrZJpEJNuZM70F5U"
#define FIREBASE_FIRESTORE_PROJECT_ID "solar-control-app"
#define FIREBASE_FIRESTORE_KEY "AIzaSyC1i1-XbvHzPnnm1ZRSrZJpEJNuZM70F5U"

static const char *TAG = "Firebase";

// Wi-Fi event handler
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wi-Fi started, connecting...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Disconnected from Wi-Fi, reconnecting...");
                esp_wifi_connect();
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Initialize Wi-Fi
void wifi_init_sta(void) {
    ESP_LOGI(TAG, "Initializing Wi-Fi...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi initialized.");
}

// Send data to Firebase Realtime Database
void send_to_firebase(const char *path, const char *json_data) {
    char url[256];
    snprintf(url, sizeof(url), "https://%s/%s.json?auth=%s", FIREBASE_HOST, path, FIREBASE_API_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_PUT,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Data sent successfully");
    } else {
        ESP_LOGE(TAG, "Error sending data: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Read data from Firebase Realtime Database
void get_from_firebase(const char *path) {
    char url[256];
    snprintf(url, sizeof(url), "https://%s/%s.json?auth=%s", FIREBASE_HOST, path, FIREBASE_API_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        char response[512];
        esp_http_client_read(client, response, sizeof(response));
        ESP_LOGI(TAG, "Firebase Data: %s", response);
    } else {
        ESP_LOGE(TAG, "Error reading data: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Send data to Firestore
void send_to_firestore(const char *collection, const char *json_data) {
    char url[256];
    snprintf(url, sizeof(url), "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents/%s?key=%s",
             FIREBASE_FIRESTORE_PROJECT_ID, collection, FIREBASE_FIRESTORE_KEY);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Firestore document sent successfully");
    } else {
        ESP_LOGE(TAG, "Error sending document: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Main function
void app_main(void) {
    wifi_init_sta();

    // Wait for Wi-Fi connection
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Send test data to Firebase Realtime Database
    send_to_firebase("test/data", "{\"message\":\"Hello from ESP32-C6\"}");

    // Retrieve data from Firebase
    get_from_firebase("test/data");

    // Send test document to Firestore
    send_to_firestore("testCollection", "{\"fields\":{\"message\":{\"stringValue\":\"Hello Firestore!\"}}}");
}

