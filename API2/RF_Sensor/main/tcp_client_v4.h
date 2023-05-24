/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"



#define PORT 1515

static const char *TAG = "SOCKET";

char rx_buffer[128];
char host_ip[] = HOST_IP_ADDR;
int addr_family = 0;
int ip_protocol = 0;
int sock;

void _close_socket(){
    shutdown(sock, 0);
    close(sock);
}

bool _socket_is_alive(){
    int error = 0;
    socklen_t len = sizeof (error);
    int retval = getsockopt (sock, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0) {
        return ESP_FAIL;
    }
    if (error != 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t _connect_socket(){
    struct sockaddr_in dest_addr;
    inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        // ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Successfully connected");
    return ESP_OK;
}

esp_err_t sendData(void * buffer, int size){//
    // int err = send(sock, payload, strlen(payload), 0);
    int err = send(sock, buffer, size, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return ESP_FAIL;
    }

    // int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

    // if (len < 0) {
    //     return ESP_FAIL;
    // }
    
    // else {
    //     rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
    //     ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
    //     ESP_LOGI(TAG, "%s", rx_buffer);
    // }

    return ESP_OK;
}

void tcp_client(void)
{   

    while (1) {

        if(_connect_socket() != ESP_OK){
            _close_socket();
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        

        while (1) {
            // sendData();
        }

        if (sock != -1) {
            _close_socket();
        }
    }
}