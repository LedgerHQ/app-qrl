/*******************************************************************************
*   (c) 2016 Ledger
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

#include "os_io_seproxyhal.h"
#include "os.h"
#include "apdu_codes.h"

#include "view.h"
#include "storage.h"
#include "app_main.h"
#include "app_types.h"

#include "libxmss/xmss.h"
#include "libxmss/nvram.h"
#include "storage.h"
#include "actions.h"

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

void parse_unsigned_message(volatile uint32_t *tx, uint32_t rx);

void parse_view_address(volatile uint32_t *tx, uint32_t rx);

unsigned char io_event(unsigned char channel) {
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT: //
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT: // for Nano S
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            if (!UX_DISPLAYED())
                UX_DISPLAYED_EVENT();
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT: {
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {if (UX_ALLOWED) UX_REDISPLAY()});
        }
            break;

            // unknown events are acknowledged
        default: {
            UX_DEFAULT_EVENT();
            break;
        }
    }
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }
    return 1; // DO NOT reset the current APDU transport
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD: {
            break;
        }

            // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI: {
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx
                // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                              sizeof(G_io_apdu_buffer), 0);
            }
        }

        default: {
            THROW(INVALID_PARAMETER);
        }
    }
    return 0;
}


////////////////////////////////////////////////
////////////////////////////////////////////////
void parse_unsigned_message(volatile uint32_t *tx, uint32_t rx) {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    if (rx <= 5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    const uint8_t *msg = G_io_apdu_buffer + 5;
    const qrltx_t *tx_p = (qrltx_t *) msg;

    // Validate message size
    const int32_t req_size = get_qrltx_size(tx_p);
    if (req_size < 0 || (uint32_t) req_size + 5 != rx) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    // move the buffer to the tx ctx
    memcpy((uint8_t * ) & ctx.qrltx, msg, rx);
}

////////////////////////////////////////////////
////////////////////////////////////////////////

#ifdef TESTING_ENABLED
void test_set_state(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    MEMCPY_NV((void*)APP_CURTREE.raw, G_io_apdu_buffer+2, 3);

    view_update_state();
}

void test_pk_gen2(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    const uint16_t idx = (p1<<8u)+p2;
    const uint8_t *p=XMSS_CUR_NODES + 32 * idx;

    uint8_t seed[48];
    get_seed(seed, N_appdata.tree_idx);

    xmss_gen_keys_1_get_seeds(&XMSS_CUR_SK, seed);
    xmss_gen_keys_2_get_nodes((uint8_t*) &N_XMSS_DATA.wots_buffer, (void*)p, &XMSS_CUR_SK, idx);

    MEMMOVE(G_io_apdu_buffer, (const void *)p, 32);
    *tx+=32;

    view_update_state();
}

void test_keygen(volatile uint32_t* tx, uint32_t rx)
{
    if (rx<5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    actions_tree_init_step();

    G_io_apdu_buffer[0] = APP_CURTREE_MODE;
    G_io_apdu_buffer[1] = APP_CURTREE_XMSSIDX >> 8;
    G_io_apdu_buffer[2] = APP_CURTREE_XMSSIDX & 0xFF;
    *tx += 3;

    view_update_state();
}

void test_write_leaf(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5+32 || (rx-5)%32!=0)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    const uint8_t size = rx-4;
    const uint8_t index = p1;
    const uint8_t *p=XMSS_CUR_NODES + 32 * index;

    print_status("W[%03d]: %03d", size, index);

    MEMCPY_NV((void*)p, (void *)data, size);
    view_update_state();
}

void test_calc_pk(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    print_status("keygen: root");

    xmss_pk_t pk;
    memset(pk.raw, 0, 64);

    xmss_gen_keys_3_get_root(XMSS_CUR_NODES, &XMSS_CUR_SK);
    xmss_pk(&pk, &XMSS_CUR_SK);

    nvm_write(APP_CURTREE.pk.raw, pk.raw, 64);

    xmss_tree_t tmp;
    tmp.mode = APPMODE_READY;
    tmp.xmss_index = 0;
    nvm_write((void*) &APP_CURTREE.raw, &tmp.raw, sizeof(tmp.raw));

    view_update_state();
}

void test_read_leaf(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    const uint8_t index = p1;
    const uint8_t *p=XMSS_CUR_NODES + 32 * index;

    MEMMOVE(G_io_apdu_buffer, (void *) p, 32);

    print_status("Read %d", index);

    *tx+=32;
    view_update_state();
}

void test_get_seed(volatile uint32_t *tx, uint32_t rx)
{
    if (rx<5)
    {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    uint8_t seed[48];
    get_seed(seed, N_appdata.tree_idx);

    MEMMOVE(G_io_apdu_buffer, seed, 48);
    *tx+=48;

    print_status("GET_SEED");

    view_update_state();
}

void test_digest(volatile uint32_t *tx, uint32_t rx)
{
    parse_unsigned_message(tx, rx);

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer+5;
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    uint8_t msg[32];
    hash_tx(msg);

    uint8_t seed[48];
    get_seed(seed, N_appdata.tree_idx);

    xmss_gen_keys_1_get_seeds(&XMSS_CUR_SK, seed);

    xmss_digest_t digest;
    memset(digest.raw, 0, XMSS_DIGESTSIZE);

    const uint8_t index = p1;
    xmss_digest(&digest, msg, &XMSS_CUR_SK, index);

    print_status("Digest idx %d", index+1);

    os_memmove(G_io_apdu_buffer, digest.raw, 64);

    *tx+=64;
    view_update_state();
}
#endif

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

void app_get_version(volatile uint32_t *tx, uint32_t rx) {
    if (rx < 5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    G_io_apdu_buffer[0] = 0;
#ifdef TESTING_ENABLED
    G_io_apdu_buffer[0] = 0xFF;
#endif

    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    *tx += 4;

    view_update_state();
}

void app_get_state(volatile uint32_t *tx, uint32_t rx) {
    if (rx < 5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    G_io_apdu_buffer[0] = APP_CURTREE_MODE;
    G_io_apdu_buffer[1] = APP_CURTREE_XMSSIDX >> 8;
    G_io_apdu_buffer[2] = APP_CURTREE_XMSSIDX & 0xFF;
    *tx += 3;

    view_update_state();
}

void app_get_pk(volatile uint32_t *tx, uint32_t rx) {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }
    if (rx < 5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    // Add pk descriptor
    G_io_apdu_buffer[0] = 0;        // XMSS, SHA2-256
    G_io_apdu_buffer[1] = 4;        // Height 8
    G_io_apdu_buffer[2] = 0;        // SHA256_X

    os_memmove(G_io_apdu_buffer + 3, APP_CURTREE.pk.raw, 64);
    *tx += 67;

    THROW(APDU_CODE_OK);
    view_update_state();
}

/// This allows extracting the signature by chunks
void app_sign(volatile uint32_t *tx, uint32_t rx) {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    if (APP_CURTREE_XMSSIDX >= 256) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    uint8_t msg[32];        // Used to store the tx hash
    hash_tx(msg);

    // buffer[2..3] are ignored (p1, p2)
    xmss_sign_incremental_init(
            &ctx.xmss_sig_ctx,
            msg,
            &XMSS_CUR_SK,
            (uint8_t * )XMSS_CUR_NODES, APP_CURTREE_XMSSIDX);

    // Move index forward
    xmss_tree_t tmp;

    tmp.mode = APPMODE_READY;
    tmp.xmss_index = APP_CURTREE_XMSSIDX + 1;
    nvm_write((void *) &APP_CURTREE.raw, &tmp.raw, sizeof(tmp.raw));
}

/// This allows extracting the signature by chunks
void app_sign_next(volatile uint32_t *tx, uint32_t rx) {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }
    if (ctx.xmss_sig_ctx.sig_chunk_idx > 10) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    const uint16_t index = APP_CURTREE_XMSSIDX - 1;      // It has already been updated

    if (ctx.xmss_sig_ctx.sig_chunk_idx == 10) {
        xmss_sign_incremental_last(&ctx.xmss_sig_ctx, G_io_apdu_buffer, &XMSS_CUR_SK, index);
        view_update_state();
    } else {
        xmss_sign_incremental(&ctx.xmss_sig_ctx, G_io_apdu_buffer, &XMSS_CUR_SK, index);
        print_status("signing %d0%%", ctx.xmss_sig_ctx.sig_chunk_idx);
    }

    if (ctx.xmss_sig_ctx.written > 0) {
        *tx = ctx.xmss_sig_ctx.written;
    }

    view_idle_show();
    UX_WAIT();
}

void parse_setidx(volatile uint32_t *tx, uint32_t rx) {
    if (rx != 6) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);

    ctx.new_idx = *data;
}

void parse_view_address(volatile uint32_t *tx, uint32_t rx) {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }
    if (rx < 5) {
        THROW(APDU_CODE_WRONG_LENGTH);
    }

    const uint8_t p1 = G_io_apdu_buffer[2];
    const uint8_t p2 = G_io_apdu_buffer[3];
    const uint8_t *data = G_io_apdu_buffer + 5;

    UNUSED(p1);
    UNUSED(p2);
    UNUSED(data);
}

void app_setidx() {
    if (APP_CURTREE_MODE != APPMODE_READY) {
        THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
    }

    const uint16_t tmp = ctx.new_idx;
    MEMCPY_NV((void *) &APP_CURTREE_XMSSIDX, (void *) &tmp, 2);
    view_update_state();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void app_init() {
    io_seproxyhal_init();
    USB_power(0);
    USB_power(1);

    // Initialize storage
    app_data_init();

    // Clear context
    MEMSET(&ctx, 0, sizeof(app_ctx_t));

    // Initialize UI
    view_update_state();
    view_idle_show();
}

void app_main() {
    volatile uint32_t rx = 0, tx = 0, flags = 0;

    for (;;) {
        volatile uint16_t sw = 0;

        BEGIN_TRY;
        {
            TRY;
            {
                rx = tx;
                tx = 0;
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                if (rx == 0) {
                    THROW(0x6982);
                }

                if (seed_mode == SEED_MODE_ERR) {
                    THROW(APDU_CODE_BUSY);
                }

                if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                    THROW(APDU_CODE_CLA_NOT_SUPPORTED);
                }

                switch (G_io_apdu_buffer[OFFSET_INS]) {

                    case INS_VERSION: {
                        app_get_version(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_GETSTATE: {
                        app_get_state(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_PUBLIC_KEY: {
                        if (APP_CURTREE_MODE != APPMODE_READY) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        app_get_pk(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_SIGN: {
                        if (APP_CURTREE_MODE != APPMODE_READY) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        if (APP_CURTREE_XMSSIDX >= 256) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        parse_unsigned_message(&tx, rx);

                        view_sign_show();
                        flags |= IO_ASYNCH_REPLY;
                        break;
                    }

                    case INS_SIGN_NEXT: {
                        if (APP_CURTREE_MODE != APPMODE_READY) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        app_sign_next(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_SETIDX: {
                        if (APP_CURTREE_MODE != APPMODE_READY) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        parse_setidx(&tx, rx);
                        view_setidx_show();
                        flags |= IO_ASYNCH_REPLY;
                        break;
                    }

                    case INS_VIEW_ADDRESS: {
                        if (APP_CURTREE_MODE != APPMODE_READY) {
                            THROW(APDU_CODE_COMMAND_NOT_ALLOWED);
                        }

                        parse_view_address(&tx, rx);
                        view_address_show();
                        UX_WAIT();
                        THROW(APDU_CODE_OK);
                        break;
                    }

#ifdef TESTING_ENABLED
                    case INS_TEST_PK_GEN_1: {
                        uint8_t seed[48];
                        get_seed(seed, N_appdata.tree_idx);

                        xmss_gen_keys_1_get_seeds(&XMSS_CUR_SK, seed);
                        os_memmove(G_io_apdu_buffer, XMSS_CUR_SK.raw, 132);
                        tx+=132;
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_PK_GEN_2: {
                        test_pk_gen2(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_KEYGEN: {
                        test_keygen(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_WRITE_LEAF: {
                        test_write_leaf(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_CALC_PK: {
                        test_calc_pk(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_READ_LEAF: {
                        test_read_leaf(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_SETSTATE: {
                        test_set_state(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_DIGEST: {
                        test_digest(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }

                    case INS_TEST_COMM:
                    {
                        uint8_t count = G_io_apdu_buffer[2];
                        for (int i = 0; i < count; i++)
                            {
                                G_io_apdu_buffer[i] = 1+i;
                                tx++;
                            }
                        THROW(APDU_CODE_OK);
                    }

                    case INS_TEST_GETSEED: {
                        test_get_seed(&tx, rx);
                        THROW(APDU_CODE_OK);
                        break;
                    }
#endif
                    default: {
                        THROW(APDU_CODE_INS_NOT_SUPPORTED);
                    }

                }
            }
            CATCH(EXCEPTION_IO_RESET)
            {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e);
            {
                switch (e & 0xF000) {
                    case 0x6000:
                    case APDU_CODE_OK: {
                        sw = e;
                        break;
                    }
                    default: {
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                    }
                }
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY;
            {}
        }
        END_TRY;
    }
}

#pragma clang diagnostic pop
