//
// Created by Chidiebere Onyedinma on 18/06/2022.
//

#ifndef TAPHEYAHW_NFC_DRIVER_H
#define TAPHEYAHW_NFC_DRIVER_H
#include <iostream>
#include <sstream>
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Following definitions specifies which settings will apply when NxpNci_ConfigureSettings()
 * API is called from the application
 */
#define NXP_CORE_CONF 1
#define NXP_CORE_STANDBY 1
#define NXP_CORE_CONF_EXTN 1
#define NXP_CLK_CONF 1  // 1=Xtal, 2=PLL
#define NXP_TVDD_CONF 2 // 1=CFG1, 2=CFG2
#define NXP_RF_CONF 1

#define NFC_FACTORY_TEST 1

#define NFC_SUCCESS 0
#define NFC_ERROR 1
#define TIMEOUT_2S 2000
#define SUCCESS NFC_SUCCESS
#define ERROR NFC_ERROR
#define MAX_NCI_FRAME_SIZE 258

/*
 * Flag definition used for NFC library configuration
 */
#define MODE_CARDEMU (1 << 0)
#define MODE_P2P (1 << 1)
#define MODE_RW (1 << 2)

/*
 * Flag definition used as Mode values
 */
#define MODE_POLL 0x00
#define MODE_LISTEN 0x80
#define MODE_MASK 0xF0

/*
 * Flag definition used as Technologies values
 */
#define TECH_PASSIVE_NFCA 0
#define TECH_PASSIVE_NFCB 1
#define TECH_PASSIVE_NFCF 2
#define TECH_ACTIVE_NFCA 3
#define TECH_ACTIVE_NFCF 5
#define TECH_PASSIVE_15693 6

/*
 * Flag definition used as Protocol values
 */
#define PROT_UNDETERMINED 0x0
#define PROT_T1T 0x1
#define PROT_T2T 0x2
#define PROT_T3T 0x3
#define PROT_ISODEP 0x4
#define PROT_NFCDEP 0x5
#define PROT_ISO15693 0x6
#define PROT_MIFARE 0x80

// Command APDU
#define C_APDU_CLA 0
#define C_APDU_INS 1  // instruction
#define C_APDU_P1 2   // parameter 1
#define C_APDU_P2 3   // parameter 2
#define C_APDU_LC 4   // length command
#define C_APDU_DATA 5 // data

#define C_APDU_P1_SELECT_BY_ID 0x00
#define C_APDU_P1_SELECT_BY_NAME 0x04

// Response APDU
#define R_APDU_SW1_COMMAND_COMPLETE 0x90
#define R_APDU_SW2_COMMAND_COMPLETE 0x00

#define R_APDU_SW1_NDEF_TAG_NOT_FOUND 0x6a
#define R_APDU_SW2_NDEF_TAG_NOT_FOUND 0x82

#define R_APDU_SW1_FUNCTION_NOT_SUPPORTED 0x6A
#define R_APDU_SW2_FUNCTION_NOT_SUPPORTED 0x81

#define R_APDU_SW1_MEMORY_FAILURE 0x65
#define R_APDU_SW2_MEMORY_FAILURE 0x81

#define R_APDU_SW1_END_OF_FILE_BEFORE_REACHED_LE_BYTES 0x62
#define R_APDU_SW2_END_OF_FILE_BEFORE_REACHED_LE_BYTES 0x82

// ISO7816-4 commands
#define ISO7816_SELECT_FILE 0xA4
#define ISO7816_READ_BINARY 0xB0
#define ISO7816_UPDATE_BINARY 0xD6

#define NDEF_MAX_LENGTH 256

typedef enum
{
    COMMAND_COMPLETE,
    TAG_NOT_FOUND,
    FUNCTION_NOT_SUPPORTED,
    MEMORY_FAILURE,
    END_OF_FILE_BEFORE_REACHED_LE_BYTES
} responseCommand;

typedef enum
{
    NONE,
    CC,
    NDEF
} tag_file;


/*
 * Flag definition used as Interface values
 */
#define INTF_UNDETERMINED 0x0
#define INTF_FRAME 0x1
#define INTF_ISODEP 0x2
#define INTF_NFCDEP 0x3
#define INTF_TAGCMD 0x80

#define MaxPayloadSize 255 // See NCI specification V1.0, section 3.1
#define MsgHeaderSize 3

/***** Factory Test dedicated APIs *********************************************/
#ifdef NFC_FACTORY_TEST

/*
 * Definition of technology types
 */
typedef enum
{
    NFC_A,
    NFC_B,
    NFC_F
} NxpNci_TechType_t;

/*
 * Definition of bitrate
 */
typedef enum
{
    BR_106,
    BR_212,
    BR_424,
    BR_848
} NxpNci_Bitrate_t;
#endif
/*
 * Definition of discovered remote device properties information
 */

/* POLL passive type A */
struct RfIntf_info_APP_t
{
    unsigned char SensRes[2];
    unsigned char NfcIdLen;
    unsigned char NfcId[10];
    unsigned char SelResLen;
    unsigned char SelRes[1];
    unsigned char RatsLen;
    unsigned char Rats[20];
};

/* POLL passive type B */
struct RfIntf_info_BPP_t
{
    unsigned char SensResLen;
    unsigned char SensRes[12];
    unsigned char AttribResLen;
    unsigned char AttribRes[17];
};

/* POLL passive type F */
struct RfIntf_info_FPP_t
{
    unsigned char BitRate;
    unsigned char SensResLen;
    unsigned char SensRes[18];
};

/* POLL passive type ISO15693 */
struct RfIntf_info_VPP_t
{
    unsigned char AFI;
    unsigned char DSFID;
    unsigned char ID[8];
};

typedef union
{
    RfIntf_info_APP_t NFC_APP;
    RfIntf_info_BPP_t NFC_BPP;
    RfIntf_info_FPP_t NFC_FPP;
    RfIntf_info_VPP_t NFC_VPP;
} RfIntf_Info_t;

/*
 * Definition of discovered remote device properties
 */
struct RfIntf_t
{
    unsigned char Interface;
    unsigned char Protocol;
    unsigned char ModeTech;
    bool MoreTags;
    RfIntf_Info_t Info;
};

/*
 * Definition of operations handled when processing Reader mode
 */
typedef enum
{
#ifndef NO_NDEF_SUPPORT
    READ_NDEF,
    WRITE_NDEF,
#endif
    PRESENCE_CHECK
} RW_Operation_t;

/*
 * Definition of discovered remote device properties information
 */
/* POLL passive type A */
typedef struct
{
    unsigned char SensRes[2];
    unsigned char NfcIdLen;
    unsigned char NfcId[10];
    unsigned char SelResLen;
    unsigned char SelRes[1];
    unsigned char RatsLen;
    unsigned char Rats[20];
} NxpNci_RfIntf_info_APP_t;

/* POLL passive type B */
typedef struct
{
    unsigned char SensResLen;
    unsigned char SensRes[12];
    unsigned char AttribResLen;
    unsigned char AttribRes[17];
} NxpNci_RfIntf_info_BPP_t;

/* POLL passive type F */
typedef struct
{
    unsigned char BitRate;
    unsigned char SensResLen;
    unsigned char SensRes[18];
} NxpNci_RfIntf_info_FPP_t;

/* POLL passive type ISO15693 */
typedef struct
{
    unsigned char AFI;
    unsigned char DSFID;
    unsigned char ID[8];
} NxpNci_RfIntf_info_VPP_t;

typedef union
{
    NxpNci_RfIntf_info_APP_t NFC_APP;
    NxpNci_RfIntf_info_BPP_t NFC_BPP;
    NxpNci_RfIntf_info_FPP_t NFC_FPP;
    NxpNci_RfIntf_info_VPP_t NFC_VPP;
} NxpNci_RfIntf_Info_t;

class NFCDriver {
private:
    static uint8_t _I2Caddress;

    void setTimeOut(unsigned long);                   // set a timeOut for an expected next event, eg reception of Response after sending a Command
    bool isTimeOut() const;
    static uint32_t dataLength(uint8_t data[]);
    bool getMessage(uint16_t timeout = 5); // 5 miliseconds as default to wait for interrupt responses
    unsigned long timeOut;
    unsigned long timeOutStartTime;
    uint32_t rxMessageLength; // length of the last message received. As these are not 0x00 terminated, we need to remember the length
    uint8_t gNfcController_generation = 0;
    uint8_t gNfcController_fw_version[3] = {0};
    bool tagWriteable = true;
    uint8_t sendlen;
    int16_t status;
    tag_file currentFile = NONE;
    static void setResponse(responseCommand cmd, uint8_t *buf, uint8_t *sendlen, uint8_t sendlenOffset);

public:
    NFCDriver(uint8_t I2Caddress);
    int GetFwVersion();
    uint8_t begin(void);
    HAL_StatusTypeDef writeData(uint8_t data[], uint32_t dataLength) const; // write data from DeviceHost to PN7150. Returns success (0) or Fail (> 0)
    static uint32_t readData(uint8_t data[]);                      // read data from PN7150, returns the amount of bytes read
    static bool hasMessage();
    uint8_t ConfigMode(uint8_t modeSE);
    uint8_t StartDiscovery(uint8_t modeSE);
    uint8_t connectNCI();
    uint8_t wakeupNCI();
    void init(uint8_t modeSE);
    HAL_StatusTypeDef CardModeSend(unsigned char *pData, unsigned char DataSize);
    bool CardModeReceive(unsigned char *pData, unsigned char *pDataSize);
    bool WaitForDiscoveryNotification(RfIntf_t *pRfIntf, uint8_t tout = 0);
    void FillInterfaceInfo(RfIntf_t *pRfIntf, uint8_t *pBuf);
    bool EmulateTag(void (*callback)(uint8_t *buf, uint16_t length), uint32_t timeout = 0);
    bool ReaderTagCmd(unsigned char *pCommand, unsigned char CommandSize, unsigned char *pAnswer, unsigned char *pAnswerSize);
    bool StopDiscovery(void);
    void ProcessReaderMode(RfIntf_t RfIntf, RW_Operation_t Operation);
    void PresenceCheck(RfIntf_t RfIntf);
    bool ReaderReActivate(RfIntf_t *pRfIntf);
    bool ReaderActivateNext(RfIntf_t *pRfIntf);
    bool ConfigureSettings(void);
    bool NxpNci_FactoryTest_Prbs(NxpNci_TechType_t type, NxpNci_Bitrate_t bitrate);
    bool NxpNci_FactoryTest_RfOn(void);
    void ProcessP2pMode(RfIntf_t RfIntf);
    void ReadNdef(RfIntf_t RfIntf);
    void WriteNdef(RfIntf_t RfIntf);
    void SetNDEFFile(const uint8_t *ndef, const int16_t ndefLength);
    void PrintChar(const uint8_t * data, const long numBytes);

};


#endif //TAPHEYAHW_NFC_DRIVER_H
