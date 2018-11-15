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

#include "qrl_types.h"

#define PTR_DIST(p2, p1) (int16_t)(((int8_t *)(p2)) - ((int8_t *)(p1)))

// TODO: Write unit tests
int16_t get_qrltx_size(const qrltx_t *tx_p) {
    if (tx_p->subitem_count == 0) {
        return -1;
    }
    if (tx_p->subitem_count > QRLTX_SUBITEM_MAX) {
        return -1;
    }

    // validate sizes
    switch (tx_p->type) {
        case QRLTX_TX: {
            const uint16_t delta = PTR_DIST(&tx_p->tx.dst, tx_p);
            const uint16_t req_size = delta + sizeof(qrltx_tx_t) * tx_p->subitem_count;
            return req_size;
        }
        case QRLTX_TXTOKEN: {
            const uint16_t delta = PTR_DIST(&tx_p->txtoken.dst, tx_p);
            const uint16_t req_size = delta + sizeof(qrltx_txtoken_t) * tx_p->subitem_count;
            return req_size;
        }
        case QRLTX_SLAVE: {
            const uint16_t delta = PTR_DIST(&tx_p->slave.slaves, tx_p);
            const uint16_t req_size = delta + sizeof(qrltx_slave_t) * tx_p->subitem_count;
            return req_size;
        }
    }
    return -1;
}