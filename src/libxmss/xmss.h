// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "xmss_types.h"

__Z_INLINE void xmss_pk(xmss_pk_t *pk_out,
                        NV_VOL const xmss_sk_t *sk_in) {
    MEMCPY((void *) pk_out->root, (void *) sk_in->root, 32);
    MEMCPY((void *) pk_out->pub_seed, (void *) sk_in->pub_seed, 32);
}

void xmss_ltree_gen(NV_VOL NV_CONST uint8_t *leaf,
                    NV_VOL NV_CONST uint8_t *tmp_wotspk,
                    NV_VOL const uint8_t *pub_seed,
                    uint16_t index);

void xmss_treehash(
        uint8_t *root_out,
        uint8_t *authpath,
        NV_VOL const uint8_t *nodes,
        NV_VOL const uint8_t *pub_seed,
        uint16_t leaf_index);

void xmss_randombits(NV_VOL NV_CONST uint8_t *random_bits,
                     NV_VOL const uint8_t sk_seed[48]
);

void xmss_get_seed_i(uint8_t *seed, NV_VOL const xmss_sk_t *sk, uint16_t idx);

void xmss_gen_keys_1_get_seeds(NV_VOL NV_CONST xmss_sk_t *sk,
                               const uint8_t *sk_seed
);

void xmss_gen_keys_2_get_nodes(NV_VOL NV_CONST uint8_t *wots_buffer,
                               NV_VOL NV_CONST uint8_t *xmss_node,
                               NV_VOL NV_CONST xmss_sk_t *sk,
                               uint16_t idx);

void xmss_gen_keys_3_get_root(NV_VOL const uint8_t *xmss_nodes,
                              NV_VOL NV_CONST  xmss_sk_t *sk);

void xmss_gen_keys(xmss_sk_t *sk, const uint8_t *sk_seed);

void xmss_digest(xmss_digest_t *digest,
                 const uint8_t msg[32],
                 NV_VOL const xmss_sk_t *sk,
                 uint16_t index);

void xmss_sign(xmss_signature_t *sig,
               const uint8_t msg[32],
               NV_VOL const xmss_sk_t *sk,
               const uint8_t xmss_nodes[XMSS_NODES_BUFSIZE],
               uint16_t index);

void xmss_sign_incremental_init(xmss_sig_ctx_t *ctx,
                                const uint8_t msg[32],
                                NV_VOL const xmss_sk_t *sk,
                                uint8_t xmss_nodes[XMSS_NODES_BUFSIZE],
                                uint16_t index);

bool xmss_sign_incremental(xmss_sig_ctx_t *ctx,
                           uint8_t *out,
                           NV_VOL const xmss_sk_t *sk,
                           uint16_t index);

bool xmss_sign_incremental_last(xmss_sig_ctx_t *ctx,
                                uint8_t *out,
                                NV_VOL const xmss_sk_t *sk,
                                uint16_t index);

#ifdef __cplusplus
}
#endif
