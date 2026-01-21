/**
 * @file wifi_task.cpp
 * @brief WiFi task operations
 */

#include "freertos/tasks/wifi_task.hpp"
#include "drivers/wifi_driver.hpp"
#include "web/http_server.hpp"
#include "esp_mac.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>

namespace WiFiTask {

namespace {
    const char* TAG = "WiFiTaskOp";

    // Update these with your local WiFi credentials.
    constexpr char kWiFiSsid[] = "SSIDNAME";
    constexpr char kWiFiPassword[] = "PASSWORD";

    // Network settings (default: DHCP).
    constexpr bool kUseStaticIp = false;
    constexpr uint8_t kStaticIp[4] = {192, 168, 1, 50};
    constexpr uint8_t kStaticGateway[4] = {192, 168, 1, 1};
    constexpr uint8_t kStaticNetmask[4] = {255, 255, 255, 0};
    constexpr bool kUseCustomDns = false;
    constexpr uint8_t kDnsServer[4] = {8, 8, 8, 8};

    char hostname[32] = {};
    bool initialized = false;
} // namespace

void BuildHostname() {
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    std::snprintf(hostname, sizeof(hostname), "ESP32_Mastery_%02X%02X", mac[4], mac[5]);
}

void WiFiOperation() {
    if (!initialized) {
        BuildHostname();
        ESP_LOGI(TAG, "Hostname: %s", hostname);

        Drivers::WiFi::Config config{};
        config.ssid = kWiFiSsid;
        config.password = kWiFiPassword;
        config.use_static_ip = kUseStaticIp;
        config.use_custom_dns = kUseCustomDns;
        config.hostname = hostname;
        std::memcpy(config.static_ip, kStaticIp, sizeof(config.static_ip));
        std::memcpy(config.static_gateway, kStaticGateway, sizeof(config.static_gateway));
        std::memcpy(config.static_netmask, kStaticNetmask, sizeof(config.static_netmask));
        std::memcpy(config.dns_server, kDnsServer, sizeof(config.dns_server));

        Drivers::WiFi::initialize(config);
        initialized = true;
    }

    Drivers::WiFi::process();
    if (Drivers::WiFi::is_connected()) {
        if (!Web::HTTPServer::is_running()) {
            Web::HTTPServer::start();
        }
    } else if (Web::HTTPServer::is_running()) {
        Web::HTTPServer::stop();
    }

    vTaskDelay(pdMS_TO_TICKS(500));
}

} // namespace WiFiTask
