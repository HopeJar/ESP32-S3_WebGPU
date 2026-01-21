/**
 * @file wifi_driver.cpp
 * @brief WiFi driver implementation
 */

#include "wifi_driver.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"
#include <cstring>

static const char* TAG = "WiFi_Driver";

namespace Drivers {
namespace WiFi {

namespace {
    bool initialized = false;
    bool connected = false;
    esp_netif_t* sta_netif = nullptr;
    esp_event_handler_instance_t wifi_event_instance = nullptr;
    esp_event_handler_instance_t ip_event_instance = nullptr;
} // anonymous namespace

void handle_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        connected = false;
        const auto* disconnect = static_cast<wifi_event_sta_disconnected_t*>(event_data);
        ESP_LOGW(TAG, "WiFi disconnected, reason=%d. Reconnecting...", disconnect ? disconnect->reason : -1);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        connected = true;
        const auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

bool initialize(const Config& config) {
    if (initialized) {
        ESP_LOGW(TAG, "WiFi driver already initialized");
        return true;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA netif");
        return false;
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_event,
                                                        nullptr, &wifi_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_event,
                                                        nullptr, &ip_event_instance));

    if (config.hostname && config.hostname[0] != '\0') {
        ESP_ERROR_CHECK(esp_netif_set_hostname(sta_netif, config.hostname));
    }

    if (config.use_static_ip) {
        esp_netif_dhcpc_stop(sta_netif);
        esp_netif_ip_info_t ip_info{};
        IP4_ADDR(&ip_info.ip, config.static_ip[0], config.static_ip[1], config.static_ip[2], config.static_ip[3]);
        IP4_ADDR(&ip_info.gw, config.static_gateway[0], config.static_gateway[1], config.static_gateway[2], config.static_gateway[3]);
        IP4_ADDR(&ip_info.netmask, config.static_netmask[0], config.static_netmask[1], config.static_netmask[2], config.static_netmask[3]);
        ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));

        if (config.use_custom_dns) {
            esp_netif_dns_info_t dns_info{};
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            IP4_ADDR(&dns_info.ip.u_addr.ip4, config.dns_server[0], config.dns_server[1], config.dns_server[2], config.dns_server[3]);
            ESP_ERROR_CHECK(esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info));
        }
    }

    wifi_config_t wifi_config{};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), config.ssid ? config.ssid : "", sizeof(wifi_config.sta.ssid));
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), config.password ? config.password : "", sizeof(wifi_config.sta.password));
    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    wifi_config.sta.threshold.authmode = (config.password && std::strlen(config.password) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    initialized = true;
    ESP_LOGI(TAG, "WiFi driver started (station)");
    return true;
}

void process() {
    // Periodic processing for WiFi driver
    // This would handle connection monitoring, reconnection logic, etc.
}

bool is_connected() {
    return connected;
}

void deinitialize() {
    if (!initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing WiFi driver");
    if (sta_netif) {
        esp_netif_destroy(sta_netif);
        sta_netif = nullptr;
    }
    if (wifi_event_instance) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_instance);
        wifi_event_instance = nullptr;
    }
    if (ip_event_instance) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_instance);
        ip_event_instance = nullptr;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    initialized = false;
}

} // namespace WiFi
} // namespace Drivers
