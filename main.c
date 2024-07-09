#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "dhtlib.h"
#include "ssid.h"
#include "tls_client.h"

const uint8_t YELLOW_LED = 15;
const uint8_t DHT_PIN = 18;

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

    const uint32_t PIN_MASK = (1 << YELLOW_LED) | (1 << DHT_PIN);

    gpio_init_mask(PIN_MASK);
    gpio_set_dir_out_masked(PIN_MASK);
    gpio_put_masked(PIN_MASK, true);

    struct repeating_timer led_timer;
    add_repeating_timer_ms(
        500, toggle_led_repeating_callback, NULL, &led_timer
    );

    printf("alarms set");

    const char TLS_CLIENT_SERVER[] = "aegan.ntllgma.workers.dev";

    while (true) {

        DhtData *data = dht_init_sequence();
        if (data == NULL) {
            printf("INIT FAIL, CONTINUE LOOP");
            sleep_ms(60000);
            continue;
        }

        bool connected = connect_with_retries(3);

        if (!connected) {
            printf("CONNECTION FAIL");
        } else {
            char TLS_CLIENT_HTTP_REQUEST[500];

            sprintf(
                TLS_CLIENT_HTTP_REQUEST,
                "GET /insert?humidity=%f&temperature=%f HTTP/1.1\r\n"
                "Host:%s\r\n"
                "Connection: close\r\n\r\n",
                data->humidity,
                data->temperature,
                TLS_CLIENT_SERVER
            );

            printf("\nGETTING:\n%s\n", TLS_CLIENT_HTTP_REQUEST);

            run_tls_client(TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, 20);
            cyw43_arch_deinit();
        }

        printf("sleeping");
        sleep_ms(60000);
    }
}
