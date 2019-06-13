// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "zxmacros.h"
#include "parameters.h"

#include "shash.h"
#include "adrs.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t csum;
    uint32_t total;
    uint32_t in;
    uint8_t bits;
    shash_input_t prf_input1;
    shash_input_t prf_input2;
} wots_sign_ctx_t;
#pragma pack(pop)

__Z_INLINE void BE_inc(uint32_t *val) { *val = NtoHL(HtoNL(*val) + 1); }

void wotsp_expand_seed(NV_VOL NV_CONST uint8_t *pk, const uint8_t *seed);

void wotsp_gen_chain(NV_VOL NV_CONST uint8_t *in_out, shash_input_t *prf_input, uint8_t start, int8_t count);

void wotsp_gen_pk(NV_VOL NV_CONST uint8_t *pk, uint8_t *sk, NV_VOL const uint8_t *pub_seed, uint16_t index);

void wotsp_sign_init_ctx(wots_sign_ctx_t *ctx,
                         NV_VOL const uint8_t *pub_seed,
                         NV_VOL const uint8_t *sk,
                         uint16_t index);

void wotsp_sign_step(wots_sign_ctx_t *ctx, uint8_t *out_sig_p, const uint8_t *msg);

__Z_INLINE bool wotsp_sign_ready(wots_sign_ctx_t *ctx) {
    return WOTS_LEN <= NtoHL(ctx->prf_input1.adrs.otshash.chain);
}

void wotsp_sign(uint8_t *out_sig,
                const uint8_t *msg,
                NV_VOL const uint8_t *pub_seed,
                NV_VOL const uint8_t *sk,
                uint16_t index);

#ifdef __cplusplus
}
#endif
