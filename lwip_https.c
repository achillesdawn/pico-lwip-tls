#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "ssid.h"
#include "tls_client.h"

const uint8_t YELLOW_LED = 15;

#define TLS_CLIENT_TIMEOUT_SECS 15

volatile bool yellow_led_state = false;

bool toggle_led_repeating_callback(struct repeating_timer *t) {
    yellow_led_state = !yellow_led_state;
    gpio_put(YELLOW_LED, yellow_led_state);
    return true;
}

bool connect_to_wifi() {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_BRAZIL)) {
        printf("Failed to initialize wifi hardware/driver\n");
        return false;
    }

    printf("Initialized cyw43\n");

    cyw43_arch_enable_sta_mode();
    printf("Pico operating in Wifi-Station(STA) mode\n");

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000
        )) {
        printf("Failed to connect to wifi\n");
        cyw43_arch_deinit();
        return false;
    }

    printf("connected\n");
    return true;
}

bool connect_with_retries(uint8_t retries) {
    for (uint8_t i = 0; i <= retries; i++) {

        bool connected = connect_to_wifi();

        if (connected) {
            return true;
        } else {
            printf("retrying in 10 seconds\n");
            sleep_ms(10000);
        }
    }
    return false;
}

int main() {
    stdio_init_all();

    sleep_ms(3000);
    printf("Setting alarms\n");

    gpio_init(YELLOW_LED);
    gpio_set_dir(YELLOW_LED, true);

    struct repeating_timer led_timer;
    add_repeating_timer_ms(
        500, toggle_led_repeating_callback, NULL, &led_timer
    );

    printf("alarms set");

    bool connected = connect_with_retries(3);
    if (!connected) {
        printf("CONNECTION FAIL");
        return 1;
    }

    char TLS_CLIENT_HTTP_REQUEST[500];
    const char TLS_CLIENT_SERVER[] = "aegan.ntllgma.workers.dev";

    sprintf(
        TLS_CLIENT_HTTP_REQUEST,
        "GET / HTTP/1.1\r\nHost:%s\r\nConnection: close\r\n\r\n",
        TLS_CLIENT_SERVER
    );

    run_tls_client_test(TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, 20);
}
