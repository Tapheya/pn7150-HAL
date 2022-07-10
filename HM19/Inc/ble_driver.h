//
// Created by Chidiebere Onyedinma on 22/06/2022.
//

#ifndef TAPHEYAHW_BLE_DRIVER_H
#define TAPHEYAHW_BLE_DRIVER_H


#include <cstdint>
#include "usart.h"

#define MAX_RX_SIZE 16

class BleDriver {
public:
    BleDriver();
    void init(void(*onReceive)(uint8_t *data));
    HAL_StatusTypeDef SendData(uint8_t *data, uint16_t  length);
    static void (*callback)(uint8_t *data);
};


#endif //TAPHEYAHW_BLE_DRIVER_H
