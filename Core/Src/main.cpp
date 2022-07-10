/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include <stdio.h>
#include <sys/unistd.h>
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include "nfc_driver.h"
#include "NdefMessage.h"
#include "ble_driver.h"
#include "cipher.h"

/* Global variables ----------------------------------------------------------*/
/* CBC context handle */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define TIMEOUT 60000
#define MAJOR_V 0x01
#define MINOR_V 0x00
#define PATCH_V 0x00
#define HW_VER 0x00
#define HEADER_LEN 0x08
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
const char SN[] = {'S', 'N', 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t MAC[6] = {0x54, 0x41, 0x50, 0x2D, 0x5E, 0x4B};
const char APP_VER[] = {'T','A','P','-','A','P','P','-','H','W','-','V',MAJOR_V,'.',MINOR_V, PATCH_V};
const char BT_NAME_PRE[] = {'t','a','p','h','e','y','a'};
const char HW[] = {'R', 'E', 'V', '_', HW_VER};

uint8_t bat_percent = 95;
uint16_t bat_voltage = 4800; //milli volts mV
uint16_t bat_capacity = 480; //mAh
uint8_t bat_charging = 0x01;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
NFCDriver nfc(0x28);
BleDriver ble;
Cipher cipher;

bool begin_trans = false;

void sendBatteryInfo() {
    uint16_t len = sizeof(bat_percent) + sizeof(bat_capacity) + sizeof(bat_voltage) + sizeof(bat_charging);
    uint16_t data_len = len + HEADER_LEN;
    uint16_t var_len = len + 3; // 3 bytes for tag(1byte) and length(2bytes)

    uint8_t data[data_len];
    uint8_t var[var_len];

    data[0] = 0x55;
    data[1] = var_len >> 8;
    data[2] = var_len  & 0x00FF;
    data[3] = 0x01;

    var[0] = 0x03;
    var[1] = len >> 8;
    var[2] = len & 0x00FF;

    memcpy(var + 3, &bat_percent, sizeof(bat_percent));
    memcpy(var + 3 + sizeof(bat_percent), &bat_capacity, sizeof(bat_capacity));
    memcpy(var + 3 + sizeof(bat_percent) + sizeof(bat_capacity), &bat_voltage, sizeof(bat_voltage));
    memcpy(var + 3 + sizeof(bat_percent) + sizeof(bat_capacity) + sizeof(bat_voltage), &bat_charging, sizeof(bat_charging));

    memcpy(data + 4 , var, var_len);

    uint8_t crc = 0;
    for(int i = 0; i < sizeof(data) - 1; i++) {
        crc ^= data[i];
    }

    data[data_len - 1] = crc;

    ble.SendData(data, sizeof(data));

}

void sendDeviceInfo() {
    uint16_t ex_data_len = 3;

    uint16_t len = sizeof(SN) + sizeof(MAC) + sizeof(APP_VER) + sizeof(BT_NAME_PRE) + sizeof(HW) + ex_data_len;
    uint16_t data_len = len + HEADER_LEN;
    uint16_t var_len = len + 3; // 3 bytes for tag(1byte) and length(2bytes)

    uint8_t data[data_len];
    uint8_t var[var_len];

    data[0] = 0x55;
    data[1] = var_len >> 8;
    data[2] = var_len  & 0x00FF;
    data[3] = 0x01;

    var[0] = 0x02;
    var[1] = len >> 8;
    var[2] = len & 0x00FF;

    memcpy(var + 3, SN, sizeof(SN));
    memcpy(var + 3 + sizeof(SN), MAC, 6);
    memcpy(var + 3 + sizeof(SN) + sizeof(MAC), APP_VER, sizeof(APP_VER));

    uint8_t bt_name[10];

    memcpy(bt_name, BT_NAME_PRE, 7);
    memcpy(bt_name + 7, SN + 5, 3);

    memcpy(var + 3 + sizeof(SN) + sizeof(MAC) + sizeof(APP_VER), bt_name, 10);
    memcpy(var + 3 + sizeof(SN) + sizeof(MAC) + sizeof(APP_VER) + sizeof(bt_name), HW, sizeof(HW));

    memcpy(data + 4 , var, var_len);

    uint8_t crc = 0;
    for(int i = 0; i < sizeof(data) - 1; i++) {
        crc ^= data[i];
    }

    data[data_len - 1] = crc;

    ble.SendData(data, sizeof(data));

}

void onTagWritten(uint8_t *nfc_data, uint16_t len) {
    nfc.PrintChar(nfc_data, len);
    NdefMessage msg = NdefMessage(nfc_data, len);
    msg.print();
    begin_trans = false;
    uint16_t en_len =  cipher.get_new_size(len); //get new length for cipher
    uint8_t en[en_len];
    cipher.encrypt(nfc_data, len, en);
    uint8_t data[en_len + HEADER_LEN];
    uint16_t var_len = en_len + 3;
    uint8_t var[var_len];
    data[0] = 0x55;
    data[1] = var_len >> 8;
    data[2] = var_len  & 0x00FF;
    data[3] = 0x01;

    var[0] = 0x04;
    var[1] = en_len >> 8;
    var[2] = en_len & 0x00FF;

    memcpy(var + 3, en, en_len);
    memcpy(data + 4 , var, var_len);

    uint8_t crc = 0;

    for(int i = 0; i < sizeof(data) - 1; i++) {
        crc ^= data[i];
    }

    data[en_len + 7] = crc;

    ble.SendData(data, sizeof(data));
}

void onBleReceive(uint8_t *data) {
    printf("Data REC %X\n", data[0]);
    if(data[0] == 0x44) {
        uint16_t len = 0;
        len = (data[1] << 8) | (data[2]);
        printf("LEN %X\n", len);
        uint8_t var[len];
        memcpy(var, data + 3, len);
        printf("TAG %X\n", var[0]);
        switch (var[0]) {
            case 0x02: //get device info
            sendDeviceInfo();
                break;
            case 0x03:
                sendBatteryInfo();
                break;
            case 0x05:
                begin_trans = false;
                printf("Transaction Cancelled..\n");
                break;
            case 0x04:
                begin_trans = true;
                unsigned long start = HAL_GetTick();
                while (!((HAL_GetTick() - start) >= TIMEOUT) && begin_trans)  {
                    nfc.EmulateTag(onTagWritten, TIMEOUT);
                }
                break;
        }
    }
}

/* USER CODE END 0 */
extern void initialise_monitor_handles(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* USER CODE BEGIN 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_UART5_Init();
    MX_USART2_UART_Init();
    MX_CRC_Init();
    setvbuf(stdout, NULL, _IONBF, 0);
    /* USER CODE BEGIN 2 */

    /* Initialize cryptographic library */
    if (!cipher.init())
    {
        Error_Handler();
    }

//    uint8_t data[cipher.get_new_size(sizeof(To_Encrypt))];
//
//    cipher.encrypt(To_Encrypt, sizeof(To_Encrypt), data);
//
//    printf("%X\n", data[0]);
//
//    uint8_t plain[sizeof(data)];
//
//    cipher.decrypt(data, sizeof(data), plain);
//
//    printf("%X\n", plain[0]);

    uint8_t mode = 2;
    nfc.init(mode);

    ble.init( onBleReceive);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_UART5
                                         | RCC_PERIPHCLK_I2C1;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.Uart5ClockSelection = RCC_UART5CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
