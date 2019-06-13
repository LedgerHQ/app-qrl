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

#include "os.h"
#include "xmss_types.h"

#define APPMODE_NOT_INITIALIZED    0x00
#define APPMODE_KEYGEN_RUNNING     0x01
#define APPMODE_READY              0x02

#define SEED_MODE_1         0
#define SEED_MODE_2         1
#define SEED_MODE_ERR       2

#define APP_NUM_TREES   4

#pragma pack(push, 1)
typedef union {
    struct {
        uint8_t mode;
        uint16_t xmss_index;
        xmss_pk_t pk;
    };
    uint8_t raw[3];
} xmss_tree_t;

typedef struct {
    uint8_t initialized;
    uint8_t tree_idx;
    uint8_t seed_mode_known;
    xmss_tree_t tree[APP_NUM_TREES];

    // Tracking alternatives
    uint8_t seed_hash_1[32];
    uint8_t seed_hash_2[32];
} app_data_t;
#pragma pack(pop)

extern uint8_t seed_mode;

extern NV_CONST app_data_t N_appdata_impl NV_ALIGN;
#define N_appdata (*(NV_VOL app_data_t *)PIC(&N_appdata_impl))

#define APP_TREE_IDX (N_appdata.tree_idx + (seed_mode<<1))

#define APP_CURTREE N_appdata.tree[APP_TREE_IDX]
#define APP_CURTREE_MODE APP_CURTREE.mode
#define APP_CURTREE_XMSSIDX  APP_CURTREE.xmss_index

#define XMSS_CUR_NODES (N_XMSS_DATA.trees[APP_TREE_IDX].xmss_nodes)
#define XMSS_CUR_SK (N_XMSS_DATA.trees[APP_TREE_IDX].sk)

void app_data_init();

void app_set_tree(uint8_t tree_index);

void app_set_mode_index(uint8_t mode, uint16_t xmss_index);
