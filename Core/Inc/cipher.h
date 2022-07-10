//
// Created by Chidiebere Onyedinma on 28/06/2022.
//

#ifndef TAPHEYAHW_CIPHER_H
#define TAPHEYAHW_CIPHER_H


#include <cstdint>
#include <vector>
#include "cmox_crypto.h"

class Cipher {
    /** Extract from NIST Special Publication 800-38A
  * F.2.1 CBC-AES128.Encrypt
    Key 2b7e151628aed2a6abf7158809cf4f3c
    IV 000102030405060708090a0b0c0d0e0f
    Block #1
    Plaintext 6bc1bee22e409f96e93d7e117393172a
    Input Block 6bc0bce12a459991e134741a7f9e1925
    Output Block 7649abac8119b246cee98e9b12e9197d
    Ciphertext 7649abac8119b246cee98e9b12e9197d
    Block #2
    Plaintext ae2d8a571e03ac9c9eb76fac45af8e51
    Input Block d86421fb9f1a1eda505ee1375746972c
    Output Block 5086cb9b507219ee95db113a917678b2
    Ciphertext 5086cb9b507219ee95db113a917678b2
    Block #3
    Plaintext 30c81c46a35ce411e5fbc1191a0a52ef
    Input Block 604ed7ddf32efdff7020d0238b7c2a5d
    Output Block 73bed6b8e3c1743b7116e69e22229516
    Ciphertext 73bed6b8e3c1743b7116e69e22229516
    Block #4
    Plaintext f69f2445df4f9b17ad2b417be66c3710
    Input Block 8521f2fd3c8eef2cdc3da7e5c44ea206
    Output Block 3ff1caa1681fac09120eca307586e1a7
    Ciphertext 3ff1caa1681fac09120eca307586e1a7
  */
public:
    explicit Cipher();
    void encrypt(const uint8_t* plain_text, uint16_t len, uint8_t* cipher);
    void decrypt(uint8_t* cipher, uint16_t len, uint8_t* plain_text);
    uint16_t get_new_size(uint16_t len);
    void pad_data(const uint8_t* data, uint16_t len, uint8_t* padded, uint16_t new_len);
    bool init();

private:
    cmox_cipher_retval_t retval;
};


#endif //TAPHEYAHW_CIPHER_H
