//
// Created by Chidiebere Onyedinma on 22/06/2022.
//

#include <cstdio>
#include "../Inc/ble_driver.h"

uint8_t rxBuff[5];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //printf("Data Received from BLE\n");
    BleDriver::callback(rxBuff);
    HAL_UART_Receive_IT(&huart5, rxBuff, 5);
}

BleDriver::BleDriver() {

}

HAL_StatusTypeDef BleDriver::SendData(uint8_t *data, uint16_t length) {
    return HAL_UART_Transmit(&huart5, data,  length,HAL_MAX_DELAY);
}

void BleDriver::init(void (*onReceive)(uint8_t *)) {
    callback = onReceive;
    HAL_UART_Receive_IT(&huart5, rxBuff, 5);
}


void (*BleDriver::callback)(uint8_t  *);