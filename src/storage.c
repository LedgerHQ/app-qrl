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
#include "storage.h"
#include "actions.h"
#include "libxmss/shash.h"

app_data_t NV_CONST
N_appdata_impl NV_ALIGN;

uint8_t seed_mode;

void app_data_init() {
    uint8_t seed[48];
    uint8_t seed_hash[32];

    seed_mode = SEED_MODE_1;
    if (N_appdata.initialized){
        // get seeds and try to match with current ones
        get_seed(seed, 0);
        __sha256(seed_hash, seed, 48);
        if (memcmp(seed_hash, N_appdata.seed_hash_1, 32) != 0 ){
            // If main seed hash does not match,
            // try alternative seed?
            seed_mode = SEED_MODE_2;
            if (!N_appdata.seed_mode_known) {
                // store alternative seed hash
                MEMCPY_NV(N_appdata.seed_hash_2, seed_hash, 32);
                SET_NV(&N_appdata.seed_mode_known, uint8_t, 1);
            } else {
                // Check the alternative seed matches
                if (memcmp(seed_hash, N_appdata.seed_hash_2, 32) != 0 ){
                    // go into error mode
                    seed_mode = SEED_MODE_ERR;
                }
            }
        }

        // show an error in the screen
        return;
    }

    // Mark as initialized + tree index = 0 + trees_used
    // get starting seed, hash and store
    get_seed(seed, 0);
    __sha256(seed_hash, seed, 48);
    MEMCPY_NV(N_appdata.seed_hash_1, seed_hash, 32);

    uint8_t tmp[] = {1, 0, 0};
    MEMCPY_NV(&N_appdata.initialized, tmp, sizeof(tmp));
}

void app_set_tree(uint8_t tree_index) {
    SET_NV(&N_appdata.tree_idx, uint8_t, tree_index);
}

void app_set_mode_index(uint8_t mode, uint16_t xmss_index) {
    xmss_tree_t tmp;
    tmp.mode = mode;
    tmp.xmss_index = xmss_index;
    nvm_write((void *) &APP_CURTREE.raw, &tmp.raw, sizeof(tmp.raw));
}
