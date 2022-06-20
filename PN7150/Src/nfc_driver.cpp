//
// Created by Chidiebere Onyedinma on 18/06/2022.
//

#include <cstring>
#include "nfc_driver.h"

uint8_t gNextTag_Protocol = PROT_UNDETERMINED;

uint8_t NCIStartDiscovery_length = 0;
uint8_t NCIStartDiscovery[30];
uint8_t rxBuffer[
        MaxPayloadSize + MsgHeaderSize]; // buffer where we store bytes received until they form a complete message

unsigned char DiscoveryTechnologiesCE[] = { // Emulation
        MODE_LISTEN | MODE_POLL};

unsigned char DiscoveryTechnologiesRW[] = { // Read & Write
        MODE_POLL | TECH_PASSIVE_NFCA,
        MODE_POLL | TECH_PASSIVE_NFCF,
        MODE_POLL | TECH_PASSIVE_NFCB,
        MODE_POLL | TECH_PASSIVE_15693};

unsigned char DiscoveryTechnologiesP2P[] = { // P2P
        MODE_POLL | TECH_PASSIVE_NFCA,
        MODE_POLL | TECH_PASSIVE_NFCF,

        /* Only one POLL ACTIVE mode can be enabled, if both are defined only NFCF applies */
        MODE_POLL | TECH_ACTIVE_NFCA,
        // MODE_POLL | TECH_ACTIVE_NFCF,

        // MODE_LISTEN | TECH_PASSIVE_NFCA,

        MODE_LISTEN | TECH_PASSIVE_NFCF,
        MODE_LISTEN | TECH_ACTIVE_NFCA,
        MODE_LISTEN | TECH_ACTIVE_NFCF};


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_9) // If The INT Source Is EXTI Line9 (A9 Pin)
    {
        DMSG("Data read\n");
        //NFCDriver::readData(rxBuffer);
    }
}

NFCDriver::NFCDriver(uint8_t I2Caddress) {
    _I2Caddress = I2Caddress << 1;
}

uint8_t NFCDriver::begin(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(3);
    return SUCCESS;
}

bool NFCDriver::hasMessage() {

    return (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
}

HAL_StatusTypeDef NFCDriver::writeData(uint8_t *data, uint32_t dataLength) const {

    return HAL_I2C_Master_Transmit(&hi2c1, _I2Caddress, data, dataLength, HAL_MAX_DELAY);

}

uint32_t NFCDriver::readData(uint8_t *data) {
    if(hasMessage()) {
        HAL_StatusTypeDef res = HAL_I2C_Master_Receive(&hi2c1, _I2Caddress, data, 258, HAL_MAX_DELAY);
        if (res != HAL_OK) {
            return 0;
        }
        uint32_t len = data[2] + 3;
        memset(rxBuffer, '\0', 258);
        for(uint32_t i = 0; i < len; i++) {
            rxBuffer[i] = data[i];
        }
        return len;
    }
    return 0;
}

void NFCDriver::setTimeOut(unsigned long theTimeOut) {
    timeOutStartTime = HAL_GetTick();
    timeOut = theTimeOut;
}

bool NFCDriver::isTimeOut() const {
    return ((HAL_GetTick() - timeOutStartTime) >= timeOut);
}

bool NFCDriver::getMessage(uint16_t timeout) {
    setTimeOut(timeout);
    rxMessageLength = 0;
    uint8_t buff[258];
    if(isTimeOut()) {
        DMSG("TIME OUT\n");
    }
    while (!isTimeOut()) {
        rxMessageLength = readData(buff);
        if (rxMessageLength) {
            std::ostringstream ss;
            ss << "Length: " << rxMessageLength << "\n";
            DMSG(ss.str().c_str());
            break;
        } else if (timeout == 1337) {
            setTimeOut(timeout);
        }
    }
    std::ostringstream tty;
    tty << "Actual Len: " << buff[2] << "\n";
    DMSG(tty.str().c_str());
    return rxMessageLength;
}

uint32_t NFCDriver::dataLength(uint8_t *data) {
    uint32_t len = 0;
    for (uint16_t i = 0; i < 258; i++) {
        //ret = HAL_I2C_Master_Transmit(&hi2c1, TMP102_ADDR, rec, 10, HAL_MAX_DELAY);
        if (data[i] == 0xFF || data[i] == '\0') {
            len = i;
            break;
        }
    }
    return len;
}

uint8_t NFCDriver::connectNCI() {
    uint8_t i = 2;
    uint8_t NCICoreInit[] = {0x20, 0x01, 0x00};
    // Open connection to NXPNCI
    begin();
    // Loop until NXPNCI answers
    while (wakeupNCI() != SUCCESS) {
        if (i-- == 0) {
            return ERROR;
        }
        HAL_Delay(500);
    }

    writeData(NCICoreInit, sizeof(NCICoreInit));
    getMessage();
    if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x01) || (rxBuffer[3] != 0x00)) {
        return ERROR;
    }


    // Retrieve NXP-NCI NFC Controller generation
    if (rxBuffer[17 + rxBuffer[8]] == 0x08)
        gNfcController_generation = 1;
    else if (rxBuffer[17 + rxBuffer[8]] == 0x10)
        gNfcController_generation = 2;

    // Retrieve NXP-NCI NFC Controller FW version
    gNfcController_fw_version[0] = rxBuffer[17 + rxBuffer[8]]; // 0xROM_CODE_V
    gNfcController_fw_version[1] = rxBuffer[18 + rxBuffer[8]]; // 0xFW_MAJOR_NO
    gNfcController_fw_version[2] = rxBuffer[19 + rxBuffer[8]]; // 0xFW_MINOR_NO
#ifdef __USART_H__
    std::ostringstream ss1;
    ss1 << "0xROM_CODE_V: " << std::hex << gNfcController_fw_version[0] << "\n";
    DMSG(ss1.str().c_str());
    std::ostringstream ss2;
    ss2 << "FW_MAJOR_NO: " << std::hex << gNfcController_fw_version[1] << "\n";
    DMSG(ss2.str().c_str());
    std::ostringstream ss3;
    ss3 << "0xFW_MINOR_NO: " << std::hex << gNfcController_fw_version[2] << "\n";
    DMSG(ss3.str().c_str());
    std::ostringstream ss4;
    ss4 << "gNfcController_generation: " << std::hex << gNfcController_generation << "\n";
    DMSG(ss4.str().c_str());
#endif
    return SUCCESS;
}

uint8_t NFCDriver::wakeupNCI() {
    // the device has to wake up using a core reset
    uint8_t NCICoreReset[] = {0x20, 0x00, 0x01, 0x01};
    uint16_t NbBytes = 0;

    // Reset RF settings restauration flag
    if(writeData(NCICoreReset, 4) == HAL_OK) {
        DMSG("Data SENT\n");
    }
    getMessage(15);
    NbBytes = rxMessageLength;
    if ((NbBytes == 0) || (rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x00)) {
        DMSG("ERROR\n");
        return ERROR;
    }
    getMessage();
    NbBytes = rxMessageLength;

    if (NbBytes != 0) {
        std::ostringstream st;
        st << "rxLength: " << NbBytes << "\n";
        DMSG(st.str().c_str());
        // NCI_PRINT_BUF("NCI << ", Answer, NbBytes);
        //  Is CORE_GENERIC_ERROR_NTF ?
        if ((rxBuffer[0] == 0x60) && (rxBuffer[1] == 0x07)) {
            /* Is PN7150B0HN/C11004 Anti-tearing recovery procedure triggered ? */
            // if ((rxBuffer[3] == 0xE6)) gRfSettingsRestored_flag = true;
        } else {
            return ERROR;
        }
    }
    DMSG("Wake up SUCCESS\n");
    return SUCCESS;
}

uint8_t NFCDriver::ConfigMode(uint8_t modeSE) {
    unsigned mode = (modeSE == 1 ? MODE_RW : modeSE == 2 ? MODE_CARDEMU
                                                         : MODE_P2P);

    uint8_t Command[MAX_NCI_FRAME_SIZE];

    uint8_t Item = 0;
    uint8_t NCIDiscoverMap[] = {0x21, 0x00};

    // Emulation mode
    const uint8_t DM_CARDEMU[] = {0x4, 0x2, 0x2};
    const uint8_t R_CARDEMU[] = {0x1, 0x3, 0x0, 0x1, 0x4};

    // RW Mode
    const uint8_t DM_RW[] = {0x1, 0x1, 0x1, 0x2, 0x1, 0x1, 0x3, 0x1, 0x1, 0x4, 0x1, 0x2, 0x80, 0x01, 0x80};
    uint8_t NCIPropAct[] = {0x2F, 0x02, 0x00};

    // P2P Support
    const uint8_t DM_P2P[] = {0x5, 0x3, 0x3};
    const uint8_t R_P2P[] = {0x1, 0x3, 0x0, 0x1, 0x5};
    uint8_t NCISetConfig_NFC[] = {0x20, 0x02, 0x1F, 0x02, 0x29, 0x0D, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x03, 0x02,
                                  0x00, 0x01, 0x04, 0x01, 0xFA, 0x61, 0x0D, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x03,
                                  0x02, 0x00, 0x01, 0x04, 0x01, 0xFA};

    uint8_t NCIRouting[] = {0x21, 0x01, 0x07, 0x00, 0x01};
    uint8_t NCISetConfig_NFCA_SELRSP[] = {0x20, 0x02, 0x04, 0x01, 0x32, 0x01, 0x00};

    if (mode == 0)
        return SUCCESS;

    /* Enable Proprietary interface for T4T card presence check procedure */
    if (modeSE == 1) {
        if (mode == MODE_RW) {
            (void) writeData(NCIPropAct, sizeof(NCIPropAct));
            getMessage();

            if ((rxBuffer[0] != 0x4F) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00))
                return ERROR;
        }
    }

    //* Building Discovery Map command
    Item = 0;

    if ((mode & MODE_CARDEMU and modeSE == 2) || (mode & MODE_P2P and modeSE == 3)) {
        memcpy(&Command[4 + (3 * Item)], (modeSE == 2 ? DM_CARDEMU : DM_P2P),
                sizeof((modeSE == 2 ? DM_CARDEMU : DM_P2P)));
        Item++;
    }
    if (mode & MODE_RW and modeSE == 1) {
        memcpy(&Command[4 + (3 * Item)], DM_RW, sizeof(DM_RW));
        Item += sizeof(DM_RW) / 3;
    }
    if (Item != 0) {
        memcpy(Command, NCIDiscoverMap, sizeof(NCIDiscoverMap));
        Command[2] = 1 + (Item * 3);
        Command[3] = Item;
        (void) writeData(Command, 3 + Command[2]);
        getMessage(10);
        if ((rxBuffer[0] != 0x41) || (rxBuffer[1] != 0x00) || (rxBuffer[3] != 0x00)) {
            return ERROR;
        }
    }

    // Configuring routing
    Item = 0;

    if (modeSE == 2 || modeSE == 3) { // Emulation or P2P
        memcpy(&Command[5 + (5 * Item)], (modeSE == 2 ? R_CARDEMU : R_P2P), sizeof((modeSE == 2 ? R_CARDEMU : R_P2P)));
        Item++;

        if (Item != 0) {
            memcpy(Command, NCIRouting, sizeof(NCIRouting));
            Command[2] = 2 + (Item * 5);
            Command[4] = Item;
            (void) writeData(Command, 3 + Command[2]);
            getMessage();
            if ((rxBuffer[0] != 0x41) || (rxBuffer[1] != 0x01) || (rxBuffer[3] != 0x00))
                return ERROR;
        }
        NCISetConfig_NFCA_SELRSP[6] += (modeSE == 2 ? 0x20 : 0x40);

        if (NCISetConfig_NFCA_SELRSP[6] != 0x00) {
            (void) writeData(NCISetConfig_NFCA_SELRSP, sizeof(NCISetConfig_NFCA_SELRSP));
            getMessage();

            if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00))
                return ERROR;
            else
                return SUCCESS;
        }

        if (mode & MODE_P2P and modeSE == 3) {
            (void) writeData(NCISetConfig_NFC, sizeof(NCISetConfig_NFC));
            getMessage();

            if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00))
                return ERROR;
        }
    }
    return SUCCESS;
}

bool NFCDriver::ConfigureSettings(void) {
#if NXP_CORE_CONF
    /* NCI standard dedicated settings
     * Refer to NFC Forum NCI standard for more details
     */
    uint8_t NxpNci_CORE_CONF[] = {
            0x20, 0x02, 0x05, 0x01, /* CORE_SET_CONFIG_CMD */
            0x00, 0x02, 0x00, 0x01  /* TOTAL_DURATION */
    };
#endif

#if NXP_CORE_CONF_EXTN
    /* NXP-NCI extension dedicated setting
     * Refer to NFC controller User Manual for more details
     */
    uint8_t NxpNci_CORE_CONF_EXTN[] = {
            0x20, 0x02, 0x0D, 0x03, /* CORE_SET_CONFIG_CMD */
            0xA0, 0x40, 0x01, 0x00, /* TAG_DETECTOR_CFG */
            0xA0, 0x41, 0x01, 0x04, /* TAG_DETECTOR_THRESHOLD_CFG */
            0xA0, 0x43, 0x01, 0x00  /* TAG_DETECTOR_FALLBACK_CNT_CFG */
    };
#endif

#if NXP_CORE_STANDBY
    /* NXP-NCI standby enable setting
     * Refer to NFC controller User Manual for more details
     */
    uint8_t NxpNci_CORE_STANDBY[] = {0x2F, 0x00, 0x01, 0x01}; /* last byte indicates enable/disable */
#endif

#if NXP_TVDD_CONF
    /* NXP-NCI TVDD configuration
     * Refer to NFC controller Hardware Design Guide document for more details
     */
    /* RF configuration related to 1st generation of NXP-NCI controller (e.g PN7120) */
    uint8_t NxpNci_TVDD_CONF_1stGen[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x13, 0x01, 0x00};

    /* RF configuration related to 2nd generation of NXP-NCI controller (e.g PN7150)*/
#if (NXP_TVDD_CONF == 1)
    /* CFG1: Vbat is used to generate the VDD(TX) through TXLDO */
    uint8_t NxpNci_TVDD_CONF_2ndGen[] = {0x20, 0x02, 0x07, 0x01, 0xA0, 0x0E, 0x03, 0x02, 0x09, 0x00};
#else
    /* CFG2: external 5V is used to generate the VDD(TX) through TXLDO */
    uint8_t NxpNci_TVDD_CONF_2ndGen[] = {0x20, 0x02, 0x07, 0x01, 0xA0, 0x0E, 0x03, 0x06, 0x64, 0x00};
#endif
#endif

#if NXP_RF_CONF
    /* NXP-NCI RF configuration
     * Refer to NFC controller Antenna Design and Tuning Guidelines document for more details
     */
    /* RF configuration related to 1st generation of NXP-NCI controller (e.g PN7120) */
    /* Following configuration is the default settings of PN7120 NFC Controller */
    uint8_t NxpNci_RF_CONF_1stGen[] = {
            0x20, 0x02, 0x38, 0x07,
            0xA0, 0x0D, 0x06, 0x06, 0x42, 0x01, 0x00, 0xF1,
            0xFF, /* RF_CLIF_CFG_TARGET          CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x06, 0x06, 0x44, 0xA3, 0x90, 0x03, 0x00, /* RF_CLIF_CFG_TARGET          CLIF_ANA_RX_REG */
            0xA0, 0x0D, 0x06, 0x34, 0x2D, 0xDC, 0x50, 0x0C,
            0x00, /* RF_CLIF_CFG_BR_106_I_RXA_P  CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x04, 0x06, 0x03, 0x00,
            0x70,             /* RF_CLIF_CFG_TARGET          CLIF_TRANSCEIVE_CONTROL_REG */
            0xA0, 0x0D, 0x03, 0x06, 0x16,
            0x00,                   /* RF_CLIF_CFG_TARGET          CLIF_TX_UNDERSHOOT_CONFIG_REG */
            0xA0, 0x0D, 0x03, 0x06, 0x15,
            0x00,                   /* RF_CLIF_CFG_TARGET          CLIF_TX_OVERSHOOT_CONFIG_REG */
            0xA0, 0x0D, 0x06, 0x32, 0x4A, 0x53, 0x07, 0x01,
            0x1B  /* RF_CLIF_CFG_BR_106_I_TXA    CLIF_ANA_TX_SHAPE_CONTROL_REG */
    };

    /* RF configuration related to 2nd generation of NXP-NCI controller (e.g PN7150)*/
    /* Following configuration relates to performance optimization of OM5578/PN7150 NFC Controller demo kit */
    uint8_t NxpNci_RF_CONF_2ndGen[] = {
            0x20, 0x02, 0x94, 0x11,
            0xA0, 0x0D, 0x06, 0x04, 0x35, 0x90, 0x01, 0xF4, 0x01, /* RF_CLIF_CFG_INITIATOR        CLIF_AGC_INPUT_REG */
            0xA0, 0x0D, 0x06, 0x06, 0x30, 0x01, 0x90, 0x03,
            0x00, /* RF_CLIF_CFG_TARGET           CLIF_SIGPRO_ADCBCM_THRESHOLD_REG */
            0xA0, 0x0D, 0x06, 0x06, 0x42, 0x02, 0x00, 0xFF,
            0xFF, /* RF_CLIF_CFG_TARGET           CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x06, 0x20, 0x42, 0x88, 0x00, 0xFF,
            0xFF, /* RF_CLIF_CFG_TECHNO_I_TX15693 CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x04, 0x22, 0x44, 0x23, 0x00,             /* RF_CLIF_CFG_TECHNO_I_RX15693 CLIF_ANA_RX_REG */
            0xA0, 0x0D, 0x06, 0x22, 0x2D, 0x50, 0x34, 0x0C,
            0x00, /* RF_CLIF_CFG_TECHNO_I_RX15693 CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x06, 0x32, 0x42, 0xF8, 0x00, 0xFF,
            0xFF, /* RF_CLIF_CFG_BR_106_I_TXA     CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x06, 0x34, 0x2D, 0x24, 0x37, 0x0C,
            0x00, /* RF_CLIF_CFG_BR_106_I_RXA_P   CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x06, 0x34, 0x33, 0x86, 0x80, 0x00,
            0x70, /* RF_CLIF_CFG_BR_106_I_RXA_P   CLIF_AGC_CONFIG0_REG */
            0xA0, 0x0D, 0x04, 0x34, 0x44, 0x22, 0x00,             /* RF_CLIF_CFG_BR_106_I_RXA_P   CLIF_ANA_RX_REG */
            0xA0, 0x0D, 0x06, 0x42, 0x2D, 0x15, 0x45, 0x0D,
            0x00, /* RF_CLIF_CFG_BR_848_I_RXA     CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x04, 0x46, 0x44, 0x22, 0x00,             /* RF_CLIF_CFG_BR_106_I_RXB     CLIF_ANA_RX_REG */
            0xA0, 0x0D, 0x06, 0x46, 0x2D, 0x05, 0x59, 0x0E,
            0x00, /* RF_CLIF_CFG_BR_106_I_RXB     CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x06, 0x44, 0x42, 0x88, 0x00, 0xFF,
            0xFF, /* RF_CLIF_CFG_BR_106_I_TXB     CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x06, 0x56, 0x2D, 0x05, 0x9F, 0x0C,
            0x00, /* RF_CLIF_CFG_BR_212_I_RXF_P   CLIF_SIGPRO_RM_CONFIG1_REG */
            0xA0, 0x0D, 0x06, 0x54, 0x42, 0x88, 0x00, 0xFF,
            0xFF, /* RF_CLIF_CFG_BR_212_I_TXF     CLIF_ANA_TX_AMPLITUDE_REG */
            0xA0, 0x0D, 0x06, 0x0A, 0x33, 0x80, 0x86, 0x00,
            0x70  /* RF_CLIF_CFG_I_ACTIVE         CLIF_AGC_CONFIG0_REG */
    };
#endif

#if NXP_CLK_CONF
    /* NXP-NCI CLOCK configuration
     * Refer to NFC controller Hardware Design Guide document for more details
     */
#if (NXP_CLK_CONF == 1)
    /* Xtal configuration */
    uint8_t NxpNci_CLK_CONF[] = {
            0x20, 0x02, 0x05, 0x01, /* CORE_SET_CONFIG_CMD */
            0xA0, 0x03, 0x01, 0x08  /* CLOCK_SEL_CFG */
    };
#else
    /* PLL configuration */
    uint8_t NxpNci_CLK_CONF[] = {
        0x20, 0x02, 0x09, 0x02, /* CORE_SET_CONFIG_CMD */
        0xA0, 0x03, 0x01, 0x11, /* CLOCK_SEL_CFG */
        0xA0, 0x04, 0x01, 0x01  /* CLOCK_TO_CFG */
    };
#endif
#endif

    uint8_t NCICoreReset[] = {0x20, 0x00, 0x01, 0x00};
    uint8_t NCICoreInit[] = {0x20, 0x01, 0x00};
    bool gRfSettingsRestored_flag = false;

#if (NXP_TVDD_CONF | NXP_RF_CONF)
    uint8_t *NxpNci_CONF;
    uint16_t NxpNci_CONF_size = 0;
#endif
#if (NXP_CORE_CONF_EXTN | NXP_CLK_CONF | NXP_TVDD_CONF | NXP_RF_CONF)
    uint8_t currentTS[32] = __TIMESTAMP__;
    uint8_t NCIReadTS[] = {0x20, 0x03, 0x03, 0x01, 0xA0, 0x14};
    uint8_t NCIWriteTS[7 + 32] = {0x20, 0x02, 0x24, 0x01, 0xA0, 0x14, 0x20};
#endif
    bool isResetRequired = false;

    /* Apply settings */
#if NXP_CORE_CONF
    if (sizeof(NxpNci_CORE_CONF) != 0) {
        isResetRequired = true;
        (void) writeData(NxpNci_CORE_CONF, sizeof(NxpNci_CORE_CONF));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CORE_CONF\n");
#endif
            return ERROR;
        }
    }
#endif

#if NXP_CORE_STANDBY
    if (sizeof(NxpNci_CORE_STANDBY) != 0) {

        (void) (writeData(NxpNci_CORE_STANDBY, sizeof(NxpNci_CORE_STANDBY)));
        getMessage();
        if ((rxBuffer[0] != 0x4F) || (rxBuffer[1] != 0x00) || (rxBuffer[3] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CORE_STANDBY\n");
#endif
            return ERROR;
        }
    }
#endif

    /* All further settings are not versatile, so configuration only applied if there are changes (application build timestamp)
       or in case of PN7150B0HN/C11004 Anti-tearing recovery procedure inducing RF setings were restored to their default value */
#if (NXP_CORE_CONF_EXTN | NXP_CLK_CONF | NXP_TVDD_CONF | NXP_RF_CONF)
    /* First read timestamp stored in NFC Controller */
    if (gNfcController_generation == 1)
        NCIReadTS[5] = 0x0F;
    (void) writeData(NCIReadTS, sizeof(NCIReadTS));
    getMessage();
    if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x03) || (rxBuffer[3] != 0x00)) {
#ifdef __USART_H__
        DMSG("read timestamp \n");
#endif
        return ERROR;
    }
    /* Then compare with current build timestamp, and check RF setting restauration flag */
    /*if(!memcmp(&rxBuffer[8], currentTS, sizeof(currentTS)) && (gRfSettingsRestored_flag == false))
    {
        // No change, nothing to do
    }
    else
    {
        */
    /* Apply settings */
#if NXP_CORE_CONF_EXTN
    if (sizeof(NxpNci_CORE_CONF_EXTN) != 0) {
        (void) writeData(NxpNci_CORE_CONF_EXTN, sizeof(NxpNci_CORE_CONF_EXTN));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CORE_CONF_EXTN\n");
#endif
            return ERROR;
        }
    }
#endif

#if NXP_CLK_CONF
    if (sizeof(NxpNci_CLK_CONF) != 0) {
        isResetRequired = true;

        (void) writeData(NxpNci_CLK_CONF, sizeof(NxpNci_CLK_CONF));
        getMessage();
        // NxpNci_HostTransceive(NxpNci_CLK_CONF, sizeof(NxpNci_CLK_CONF), Answer, sizeof(Answer), &AnswerSize);
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CLK_CONF\n");
#endif
            return ERROR;
        }
    }
#endif

#if NXP_TVDD_CONF
    if (NxpNci_CONF_size != 0) {

        (void) writeData(NxpNci_TVDD_CONF_2ndGen, sizeof(NxpNci_TVDD_CONF_2ndGen));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CONF_size\n");
#endif
            return ERROR;
        }
    }
#endif

#if NXP_RF_CONF
    if (NxpNci_CONF_size != 0) {

        (void) writeData(NxpNci_RF_CONF_2ndGen, sizeof(NxpNci_RF_CONF_2ndGen));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
            DMSG("NxpNci_CONF_size\n");
#endif
            return ERROR;
        }
    }
#endif
    /* Store curent timestamp to NFC Controller memory for further checks */
    if (gNfcController_generation == 1)
        NCIWriteTS[5] = 0x0F;
    memcpy(&NCIWriteTS[7], currentTS, sizeof(currentTS));
    (void) writeData(NCIWriteTS, sizeof(NCIWriteTS));
    getMessage();
    if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x02) || (rxBuffer[3] != 0x00) || (rxBuffer[4] != 0x00)) {
#ifdef __USART_H__
        DMSG("NFC Controller memory\n");
#endif
        return ERROR;
    }
    //}
#endif

    if (isResetRequired) {
        /* Reset the NFC Controller to insure new settings apply */
        (void) writeData(NCICoreReset, sizeof(NCICoreReset));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x00) || (rxBuffer[3] != 0x00)) {
#ifdef __USART_H__
            DMSG("insure new settings apply\n");
#endif
            return ERROR;
        }

        (void) writeData(NCICoreInit, sizeof(NCICoreInit));
        getMessage();
        if ((rxBuffer[0] != 0x40) || (rxBuffer[1] != 0x01) || (rxBuffer[3] != 0x00)) {
#ifdef __USART_H__
            DMSG("insure new settings apply 2\n");
#endif
            return ERROR;
        }
    }
    return SUCCESS;
}

uint8_t NFCDriver::StartDiscovery(uint8_t modeSE) {
    unsigned char TechTabSize = (modeSE == 1 ? sizeof(DiscoveryTechnologiesRW) : modeSE == 2 ? sizeof(DiscoveryTechnologiesCE)
                                                                                             : sizeof(DiscoveryTechnologiesP2P));

    NCIStartDiscovery_length = 0;
    NCIStartDiscovery[0] = 0x21;
    NCIStartDiscovery[1] = 0x03;
    NCIStartDiscovery[2] = (TechTabSize * 2) + 1;
    NCIStartDiscovery[3] = TechTabSize;
    for (uint8_t i = 0; i < TechTabSize; i++)
    {
        NCIStartDiscovery[(i * 2) + 4] = (modeSE == 1 ? DiscoveryTechnologiesRW[i] : modeSE == 2 ? DiscoveryTechnologiesCE[i]
                                                                                                 : DiscoveryTechnologiesP2P[i]);

        NCIStartDiscovery[(i * 2) + 5] = 0x01;
    }

    NCIStartDiscovery_length = (TechTabSize * 2) + 4;
    (void)writeData(NCIStartDiscovery, NCIStartDiscovery_length);
    getMessage();

    if ((rxBuffer[0] != 0x41) || (rxBuffer[1] != 0x03) || (rxBuffer[3] != 0x00))
        return ERROR;
    else
        return SUCCESS;
}

uint8_t NFCDriver::_I2Caddress;
