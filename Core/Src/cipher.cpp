//
// Created by Chidiebere Onyedinma on 28/06/2022.
//

#include <cstdio>
#include "cipher.h"

const uint8_t Key[] =
        {
                0x37, 0x41, 0x50, 0x23, 0x33, 0x59, 0x61, 0x4C, 0x31, 0x4D, 0x69, 0x54, 0x65, 0x44, 0x4E, 0x47
        };
const uint8_t IV[] =
        {
                0x74, 0x61, 0x70, 0x68, 0x65, 0x79, 0x61, 0x6C, 0x69, 0x6D, 0x69, 0x74, 0x65, 0x64, 0x6E, 0x67
        };

Cipher::Cipher() {

}

void Cipher::encrypt(const uint8_t *plain_text, uint16_t len, uint8_t* cipher) {
    uint16_t new_len = get_new_size(len);

    uint8_t padded[new_len];

    size_t computed_size;
    pad_data(plain_text, len, padded, new_len);

    retval = cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,                  /* Use AES CBC algorithm */
            padded, new_len,           /* Plaintext to encrypt */
            Key, sizeof(Key),                       /* AES key to use */
            IV, sizeof(IV),                         /* Initialization vector */
            cipher, &computed_size);   /* Data buffer to receive generated ciphertext */

    if (retval != CMOX_CIPHER_SUCCESS)
    {
        printf("Encryption not successful\n");
    }

}

void Cipher::decrypt(uint8_t *cipher, uint16_t len, uint8_t *plain_text) {
    uint16_t new_len = get_new_size(len);

    uint8_t padded[new_len];

    size_t computed_size;
    pad_data(cipher, len, padded, new_len);

    retval = cmox_cipher_decrypt(CMOX_AES_CBC_DEC_ALGO,                  /* Use AES CBC algorithm */
            padded, new_len,           /* Plaintext to encrypt */
            Key, sizeof(Key),                       /* AES key to use */
            IV, sizeof(IV),                         /* Initialization vector */
            plain_text, &computed_size);   /* Data buffer to receive generated ciphertext */

    if (retval != CMOX_CIPHER_SUCCESS)
    {
        printf("Decryption not successful\n");
    }

}

bool Cipher::init() {
    /* Initialize cryptographic library */
    if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
    {
        return false;
    }
    return true;
}

void Cipher::pad_data(const uint8_t *data, uint16_t len, uint8_t* padded, uint16_t new_len) {

    for(int i = 0; i < new_len; i++) {
        if(i >= len) {
            padded[i] = 0xFF;
        } else {
            padded[i] = data[i];
        }

        //printf("index: %X\n", padded[i]);
    }
}

uint16_t Cipher::get_new_size(uint16_t len) {
    uint16_t new_len;
    if(len < 16) {
        new_len = 16;
    } else if(len > 16 && len < 32) {
        new_len = 32;
    } else if(len > 32 && len < 64) {
        new_len = 64;
    } else if(len > 64 && len < 128) {
        new_len = 128;
    } else if (len > 128)  {
        new_len = 128;
    } else {
        new_len = len;
    }

    return new_len;
}
