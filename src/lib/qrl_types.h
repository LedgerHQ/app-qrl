/*******************************************************************************
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// QRL TX definitions

#define QRLTX_TX (0)
#define QRLTX_TXTOKEN (1)
#define QRLTX_SLAVE (2)
#define QRLTX_MESSAGE (3)

#pragma pack(push, 1)
typedef struct {
    uint8_t address[39];      // address is expected to be 39 bytes
    uint8_t amount[8];        // amount or fee
} qrltx_addr_block;         // 47 bytes

typedef struct {
    uint8_t pk[35];           // ePK is expected to be 35 bytes
    uint8_t access[8];        // access type
} qrltx_slave_block;        // 43 bytes

/////////////////////////////////////////

#define QRLTX_SUBITEM_MAX 3
#define QRLTX_MESSAGE_SUBITEM_MAX 80
#define QUANTA_DECIMALS 9

typedef struct {
    qrltx_addr_block master;                                // 47
    qrltx_addr_block dst[QRLTX_SUBITEM_MAX];
} qrltx_tx_t;                                               // 188 bytes

typedef struct {
    qrltx_addr_block master;                                // 47
    uint8_t token_hash[32];                                 // 32
    qrltx_addr_block dst[QRLTX_SUBITEM_MAX];
} qrltx_txtoken_t;                                          // 220 bytes

typedef struct {
    qrltx_addr_block master;                                // 47
    qrltx_slave_block slaves[QRLTX_SUBITEM_MAX];
} qrltx_slave_t;                                            // 176 bytes

typedef struct {
    qrltx_addr_block master;                                // 47
    uint8_t message[80];                                    // 80
} qrltx_msg_t;                                              // 127 bytes

/////////////////////////////////////////

typedef struct {
    uint8_t type;
    uint8_t subitem_count;
    union {
        qrltx_tx_t tx;
        qrltx_txtoken_t txtoken;
        qrltx_slave_t slave;
        qrltx_msg_t msg;                                    // for messages, subitem_count indicates number of bytes
    };
} qrltx_t;                                                  // 222 bytes
#pragma pack(pop)

int16_t get_qrltx_size(const qrltx_t *tx_p);
int8_t get_qrltx_hash(const qrltx_t *tx_p, uint8_t hash[32]);

#ifdef __cplusplus
}
#endif
