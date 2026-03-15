#include "uart_driver.h"
#include <Arduino.h>

static bool stm32_uart_enabled = true;
static char stm32_last_packet[64] = {0};

#define GOPIO_NUM_16 16  // RX pin for UART2 
#define GPIO_NUM_17 17   // TX pin for UART2    

/**
 * This UART driver provides basic functionality for UART communication between the ESP32 and an STM32 microcontroller. It includes initialization, sending and receiving data, and a task function to handle incoming packets from the STM32. The driver uses the Arduino Serial library for UART communication, and it is designed to be simple and easy to integrate into your project.  
 * The UART2_Init function initializes the second UART interface (Serial2) with a baud rate of 115200 and sets the RX and TX pins. The UART2_Enable function allows you to enable or disable the UART communication with the STM32. The UART2_GetPacket function returns the last received packet from the STM32. The UART_Init function initializes the primary UART interface (Serial) for communication with a host computer or other devices. The UART2_SendByte function sends a single byte of data over Serial, while the UART_SendString function sends a string. The UART_ReadByte function reads a byte from Serial if available, and the UART_Available function checks if there is data available to read. Finally, the UART2_Task function should be called regularly (e.g., in the main loop) to check for incoming data from the STM32 and store it in a buffer for later retrieval. 
 * This driver is a starting point and can be expanded with additional features such as error handling, support for different baud rates, or more complex packet parsing as needed for your application.    
 * Audience: This code is suitable for developers who are working on a project that involves UART communication between an ESP32 and an STM32 microcontroller. It provides a simple interface for sending and receiving data over UART, making it easier to integrate into your project and manage communication between the two devices.   
 * 
 */
void UART2_Init(void) {
    Serial2.begin(115200, SERIAL_8N1, GPIO_NUM_16, GPIO_NUM_17);  
    // RX = GPIO16, TX = GPIO17
}

void UART2_Enable(bool state) {
    stm32_uart_enabled = state;
}

const char* UART2_GetPacket(void) {
    return stm32_last_packet;
}


void UART_Init(void) {
    // Start UART at 115200 baud (change if needed)
    Serial.begin(115200);

    // Wait for serial port to be ready (optional)
    while (!Serial) {
        ; // Wait
    }
}

void UART2_SendByte(uint8_t data) {
    Serial.write(data);
}

void UART_SendString(const char *str) {
    Serial.print(str);
}

uint8_t UART_ReadByte(void) {
    if (Serial.available() > 0) {
        return Serial.read();
    }
    return 0; // Default return if nothing available
}

bool UART_Available(void) {
    return Serial.available() > 0;
}

void UART2_Task(void) {
    if (!stm32_uart_enabled) return;

    while (Serial2.available()) {
        String packet = Serial2.readStringUntil('\n');
        packet.trim();

        if (packet.length() > 0) {
            strncpy(stm32_last_packet, packet.c_str(), sizeof(stm32_last_packet));
            stm32_last_packet[sizeof(stm32_last_packet)-1] = '\0';

            Serial.print("STM32 UART: ");
            Serial.println(stm32_last_packet);
        }
    }
}
