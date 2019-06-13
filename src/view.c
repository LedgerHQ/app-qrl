/*******************************************************************************
*   (c) 2016 Ledger
*   (c) 2018, 2019 ZondaX GmbH
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
#include <string.h>
#include "glyphs.h"
#include "view.h"
#include "app_main.h"
#include "apdu_codes.h"
#include "storage.h"
#include "view_templates.h"
#include "app_types.h"

#include "actions.h"

#define REVIEW_DATA_AVAILABLE 1
#define REVIEW_NO_MORE_DATA   0

view_t viewdata;

#define ARRTOHEX(X, Y) array_to_hexstr(X, Y, sizeof(Y))
#define AMOUNT_TO_STR(OUTPUT, AMOUNT, DECIMALS) fpuint64_to_str(OUTPUT, uint64_from_BEarray(AMOUNT), DECIMALS)

void view_sign_internal_show();

void view_sign_internal_show();

void h_tree_init(unsigned int _) {
    actions_tree_init();
    view_update_state();
    view_idle_show();
}

void h_tree_switch(unsigned int _) {
    app_set_tree((APP_TREE_IDX +1) % 2);
    view_update_state();
    view_idle_show();
}

void h_setidx_accept() {
    // Accept changing the index
    app_setidx();
    view_update_state();
    view_idle_show();
    UX_WAIT();

    set_code(G_io_apdu_buffer, 0, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

void h_setidx_reject() {
    // Cancel changing the index
    view_update_state();
    view_idle_show();
    UX_WAIT();

    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

void h_show_addr(unsigned int _) {
    UNUSED(_);
    view_address_show();
    UX_WAIT();
}

void h_back() {
    view_update_state();
    view_idle_show();
    UX_WAIT();
}

void h_review(unsigned int _) {
    UNUSED(_);
    viewdata.idx = 0;
    view_review_show();
}

void h_sign_accept(unsigned int _) {
    UNUSED(_);
    app_sign();
    view_update_state();
    view_idle_show();
    UX_WAIT();

    set_code(G_io_apdu_buffer, 0, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

void h_sign_reject(unsigned int _) {
    UNUSED(_);
    view_update_state();
    view_idle_show();
    UX_WAIT();

    set_code(G_io_apdu_buffer, 0, APDU_CODE_COMMAND_NOT_ALLOWED);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

#if defined(TARGET_NANOX)

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

UX_STEP_NOCB(ux_idle_flow_0_step, pbb, { &C_icon_app, "QRL", "seed error", });
UX_STEP_NOCB(ux_idle_flow_1_step, pbb, { &C_icon_app, viewdata.key, viewdata.value, });
UX_STEP_VALID(ux_idle_flow_2init_step, pb, h_tree_init(0), { &C_icon_key, "Initialize",});
UX_STEP_VALID(ux_idle_flow_2pk_step, pb, h_show_addr(0), { &C_icon_key, "Show Addr",});
UX_STEP_VALID(ux_idle_flow_3_step, pb, h_tree_switch(0), { &C_icon_refresh, "Switch Tree",});
UX_STEP_NOCB(ux_idle_flow_4_step, bn, { "Version", APPVERSION, });
UX_STEP_VALID(ux_idle_flow_5_step, pb, os_sched_exit(-1), { &C_icon_dashboard, "Quit",});

UX_FLOW(
    ux_error_flow,
    &ux_idle_flow_0_step,
    &ux_idle_flow_4_step,
    &ux_idle_flow_5_step
);

UX_FLOW(
    ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2pk_step,
    &ux_idle_flow_3_step,
    &ux_idle_flow_4_step,
    &ux_idle_flow_5_step
);

UX_FLOW(
    ux_idle_init_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2init_step,
    &ux_idle_flow_3_step,
    &ux_idle_flow_4_step,
    &ux_idle_flow_5_step
);

UX_STEP_NOCB(ux_set_index_flow_1_step, bn, { viewdata.key, viewdata.value, });
UX_STEP_VALID(ux_set_index_flow_2_step, pbb, h_setidx_accept(), { &C_icon_validate_14, "Accept", "Change" });
UX_STEP_VALID(ux_set_index_flow_3_step, pbb, h_setidx_reject(), { &C_icon_crossmark, "Reject", "Change" });

UX_FLOW(
    ux_set_index_flow,
    &ux_set_index_flow_1_step,
    &ux_set_index_flow_2_step,
    &ux_set_index_flow_3_step
);

UX_STEP_NOCB(ux_addr_flow_1_step, bnnn_paging, { .title = viewdata.key, .text = viewdata.value, });
UX_STEP_VALID(ux_addr_flow_2_step, pb, h_back(), { &C_icon_validate_14, "Back"});

UX_FLOW(
    ux_addr_flow,
    &ux_addr_flow_1_step,
    &ux_addr_flow_2_step
);

typedef struct
{
unsigned char inside : 1;
unsigned char no_more_data : 1;
} review_state_t;

review_state_t review_state;

void h_review_start()
{
    if (review_state.inside) {
    // coming from right
        viewdata.idx--;
        if (viewdata.idx<0) {
            // exit to the left
            review_state.inside = 0;
            ux_flow_prev();
            return;
        }
    } else {
    // coming from left
        viewdata.idx = 0;
    }

    view_update_review();
    ux_flow_next();
}

void h_review_data()
{
    review_state.inside = 1;
}

void h_review_end()
{
    if (review_state.inside) {
    // coming from left
        viewdata.idx++;
        if (view_update_review()== REVIEW_NO_MORE_DATA){
            review_state.inside = 0;
            ux_flow_next();
            return;
        }
        ux_layout_bnnn_paging_reset();
    } else {
    // coming from right
        viewdata.idx--;
        view_update_review();
    }

    // move to prev flow but trick paging to show first page
    CUR_FLOW.prev_index = CUR_FLOW.index-2;
    CUR_FLOW.index--;
    ux_flow_relayout();
}

UX_STEP_NOCB(ux_sign_flow_1_step, pbb, { &C_icon_eye, "Review", "Transaction" });

UX_STEP_INIT(ux_sign_flow_2_start_step, NULL, NULL, { h_review_start(); });
UX_STEP_NOCB_INIT(ux_sign_flow_2_step, bnnn_paging, { h_review_data(); }, { .title = viewdata.key, .text = viewdata.value, });
UX_STEP_INIT(ux_sign_flow_2_end_step, NULL, NULL, { h_review_end(); });

UX_STEP_VALID(ux_sign_flow_3_step, pbb, h_sign_accept(0), { &C_icon_validate_14, "Sign", "Transaction" });
UX_STEP_VALID(ux_sign_flow_4_step, pbb, h_sign_reject(0), { &C_icon_crossmark, "Reject", "Transaction" });
const ux_flow_step_t *const ux_sign_flow[] = {
  &ux_sign_flow_1_step,
  &ux_sign_flow_2_start_step,
  &ux_sign_flow_2_step,
  &ux_sign_flow_2_end_step,
  &ux_sign_flow_3_step,
  &ux_sign_flow_4_step,
  FLOW_END_STEP,
};

#else

#define UIID_STATUS        UIID_LABEL+1
#define UIID_TREE_INIT     UIID_LABEL+3
#define UIID_TREE_PK       UIID_LABEL+4
#define UIID_TREE_SWITCH   UIID_LABEL+5

ux_state_t ux;

const ux_menu_entry_t menu_error[] = {
        {NULL, NULL, UIID_STATUS, &C_icon_app, "QRL", "seed error", 28, 8},
        {NULL, NULL, 0, NULL, "v"APPVERSION, NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_idle[] = {
        {NULL, NULL, UIID_STATUS, &C_icon_app, viewdata.key, viewdata.value, 28, 8},
        {NULL, h_show_addr, UIID_TREE_PK, NULL, "Show Addr", NULL, 0, 0},
        {NULL, h_tree_switch, UIID_TREE_PK, NULL, "Switch Tree", NULL, 0, 0},
        {NULL, NULL, 0, NULL, "v"APPVERSION, NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_idle_init[] = {
        {NULL, NULL, UIID_STATUS, &C_icon_app, viewdata.key, viewdata.value, 28, 8},
        {NULL, h_tree_init, UIID_TREE_INIT, NULL, "Init Tree", NULL, 0, 0},
        {NULL, h_tree_switch, UIID_TREE_PK, NULL, "Switch Tree", NULL, 0, 0},
        {NULL, NULL, 0, NULL, "QRL v"APPVERSION, NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_sign[] = {
        {NULL, h_review, 0, NULL, "View transaction", NULL, 0, 0},
        {NULL, h_sign_accept, 0, NULL, "Sign transaction", NULL, 0, 0},
        {NULL, h_sign_reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
        UX_MENU_END
};

static const bagl_element_t view_review[] = {
        UI_BACKGROUND_LEFT_RIGHT_ICONS,
        UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.title),
        UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
        UI_LabelLineScrolling(UIID_LABELSCROLL, 6, 30, 112, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
};

static const bagl_element_t view_setidx[] = {
        UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
        UI_Icon(0, 0, 0, 7, 7, BAGL_GLYPH_ICON_CROSS),
        UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
        UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.title),
        UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
        UI_LabelLineScrolling(UIID_LABELSCROLL, 6, 30, 112, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
};

static const bagl_element_t view_address[] = {
        UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
        UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
        UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.title),
        UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
        UI_LabelLineScrolling(UIID_LABELSCROLL, 6, 30, 112, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
};

static unsigned int view_review_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Press both left and right buttons to quit
            view_sign_internal_show();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to the previous element
            viewdata.idx--;
            if (view_update_review() == REVIEW_NO_MORE_DATA) {
                view_sign_internal_show();
            } else {
                view_review_show();
            }
            UX_WAIT();
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to the next element
            viewdata.idx++;
            if (view_update_review() == REVIEW_NO_MORE_DATA) {
                view_sign_internal_show();
            } else {
                view_review_show();
            }
            UX_WAIT();
            break;
    }
    return 0;
}

static unsigned int view_setidx_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        // Press both left and right buttons to quit
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to cancel
            h_setidx_reject();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to accept
            h_setidx_accept();
            break;
    }
    return 0;
}

static unsigned int view_address_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Press both left and right buttons to quit
            // DO NOTHING
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Press left to progress to cancel
            // DO NOTHING
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Press right to progress to accept
            // Go back to main menu
            view_update_state();
            view_idle_show();
            UX_WAIT();
            break;
    }
    return 0;
}

const bagl_element_t *view_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
        case UIID_ICONRIGHT:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

#endif

////////////////////////////////////////////////
////////////////////////////////////////////////

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_init(void) {
    UX_INIT();
}

void view_idle_show(void) {

#if defined(TARGET_NANOS)
    if (seed_mode >= SEED_MODE_ERR) {
        UX_MENU_DISPLAY(0, menu_error, NULL);
        return;
    }
    if (APP_CURTREE_MODE != APPMODE_READY) {
        UX_MENU_DISPLAY(0, menu_idle_init, NULL);
    } else {
        UX_MENU_DISPLAY(0, menu_idle, NULL);
    }
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    if (seed_mode >= SEED_MODE_ERR) {
        ux_flow_init(0, ux_error_flow, NULL);
        return;
    }
    if (APP_CURTREE_MODE != APPMODE_READY) {
        ux_flow_init(0, ux_idle_init_flow, NULL);
    } else {
        ux_flow_init(0, ux_idle_flow, NULL);
    }
#endif
}

void view_sign_show() {
#if defined(TARGET_NANOS)
    viewdata.idx = 0;
    view_update_review();
    view_review_show();
#elif defined(TARGET_NANOX)
    view_sign_internal_show();
#endif
}

void view_sign_internal_show(void) {
#if defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_sign, NULL);
#elif defined(TARGET_NANOX)
    viewdata.idx = -1;
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    review_state.inside = 0;
    review_state.no_more_data = 0;
    ux_flow_init(0, ux_sign_flow, NULL);
#endif
}

void view_review_show(void) {
#if defined(TARGET_NANOS)
    UX_DISPLAY(view_review, view_prepro);
#endif
}

void view_setidx_show() {
    strcpy(viewdata.title, "WARNING!");
    strcpy(viewdata.key, "Set XMSS Index");
    print_status("New Value %d", ctx.new_idx);

#if defined(TARGET_NANOS)
    UX_DISPLAY(view_setidx, view_prepro);
#elif defined(TARGET_NANOX)
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_set_index_flow, NULL);
#endif
}

void view_address_show() {
    // See https://docs.theqrl.org/developers/address/#format-sha256_2x
    // Add Ledger Nano S wallet address descriptor
    unsigned char desc[3];
    desc[0] = 0; // XMSS, SHA2-256
    desc[1] = 4; // Height 8
    desc[2] = 0; // SHA256_X

    // Copy raw pk into pk
    unsigned char pk[64];
    MEMCPY(pk, (void *) APP_CURTREE.pk.raw, 64);

    // Copy desc and pk to form extended_pubkey
    unsigned char extended_pubkey[67];
    MEMCPY(extended_pubkey, desc, 3);
    MEMCPY(extended_pubkey + 3, pk, 64);

    // Objects to store sha256 hashes in
    unsigned char hash_out[32];
    unsigned char verh_out[32];

    // Calculate HASH - sha2-256(DESC+PK)
    cx_hash_sha256(extended_pubkey, 67, hash_out, 32);

    // Copy desc and hash_out to create hashable bytes for verification checksum
    unsigned char verification_sum[35];
    MEMCPY(verification_sum, desc, 3);
    MEMCPY(verification_sum + 3, hash_out, 32);

    // Calculate VERH - sha2-256(DESC+hash_out)
    cx_hash_sha256(verification_sum, 35, verh_out, 32);

    // Now join desc, hash_out and last 4 bytes of verh_out to form address
    unsigned char address[39];
    MEMCPY(address, desc, 3);
    MEMCPY(address + 3, hash_out, 32);
    MEMCPY(address + 3 + 32, verh_out + 28, 4);

    // Prepare for display
    strcpy(viewdata.title, "VERIFY");
    strcpy(viewdata.key, "Your Address");
    viewdata.value[0] = 'Q';
    ARRTOHEX(viewdata.value + 1, address);

#if defined(TARGET_NANOS)
    UX_DISPLAY(view_address, view_prepro);
#elif defined(TARGET_NANOX)
    ux_layout_bnnn_paging_reset();
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_addr_flow, NULL);
#endif
}

////////////////////////////////////////////////
////////////////////////////////////////////////

void view_update_state() {
#ifdef TESTING_ENABLED
    print_key("QRL (T%d) (TEST)", APP_TREE_IDX + 1)
#else
    print_key("QRL (Tree%d)", ((APP_TREE_IDX) % 2) + 1);
#endif

    if (APP_CURTREE_MODE == APPMODE_NOT_INITIALIZED){
        print_status("not ready");
        return;
    }

    if (APP_CURTREE_MODE == APPMODE_KEYGEN_RUNNING) {
        print_status("KEYGEN rem:%03d", 256 - APP_CURTREE_XMSSIDX);
        return;
    }

    if (APP_CURTREE_MODE == APPMODE_READY) {
        if (APP_CURTREE_XMSSIDX >= 256) {
            print_status("NO SIGS LEFT");
            return;
        }

        if (APP_CURTREE_XMSSIDX > 250) {
            print_status("WARN! rem:%03d", 256 - APP_CURTREE_XMSSIDX);
            return;
        }

        print_status("READY rem:%03d", 256 - APP_CURTREE_XMSSIDX);
    }
}

// returns 1 while there is still data to show
int8_t view_update_review() {
#define PTR_DIST(p2, p1) ((char *)p2) - ((char *)p1)

    uint8_t elem_idx = 0;

    switch (ctx.qrltx.type) {

        case QRLTX_TX: {
            strcpy(viewdata.title, "TRANSFER");

            switch (viewdata.idx) {
                case 0: {
                    strcpy(viewdata.key, "Source Addr");
                    viewdata.value[0] = 'Q';
                    ARRTOHEX(viewdata.value + 1, ctx.qrltx.tx.master.address);
                    break;
                }
                case 1: {
                    strcpy(viewdata.key, "Fee (QRL)");
                    AMOUNT_TO_STR(viewdata.value,
                                  ctx.qrltx.tx.master.amount,
                                  QUANTA_DECIMALS);
                    break;
                }
                default: {
                    elem_idx = (viewdata.idx - 2) >> 1;
                    if (elem_idx >= QRLTX_SUBITEM_MAX) return REVIEW_NO_MORE_DATA;
                    if (elem_idx >= ctx.qrltx.subitem_count) return REVIEW_NO_MORE_DATA;

                    qrltx_addr_block *dst = &ctx.qrltx.tx.dst[elem_idx];

                    if (viewdata.idx % 2 == 0) {
                        print_key("Dst %d", elem_idx);
                        viewdata.value[0] = 'Q';
                        ARRTOHEX(viewdata.value + 1, dst->address);
                    } else {
                        print_key("Amount %d (QRL)", elem_idx);
                        AMOUNT_TO_STR(viewdata.value, dst->amount, QUANTA_DECIMALS);
                    }
                    break;
                }
            }
            break;
        }
#ifdef TXTOKEN_ENABLED
        case QRLTX_TXTOKEN: {
            strcpy(viewdata.title, "TRANSFER TOKEN");

            switch (viewdata.idx) {
                case 0: {
                    strcpy(viewdata.key, "Source Addr");
                    viewdata.value[0] = 'Q';
                    ARRTOHEX(viewdata.value + 1, ctx.qrltx.txtoken.master.address);
                    break;
                }
                case 1: {
                    strcpy(viewdata.key, "Fee (QRL)");
                    AMOUNT_TO_STR(viewdata.value,
                                  ctx.qrltx.txtoken.master.amount,
                                  QUANTA_DECIMALS);
                    break;
                }
                case 2: {
                    strcpy(viewdata.key, "Token Hash");
                    ARRTOHEX(viewdata.value, ctx.qrltx.txtoken.token_hash);
                    break;
                }
                default: {
                    elem_idx = (viewdata.idx - 3) >> 2;
                    if (elem_idx >= QRLTX_SUBITEM_MAX) return REVIEW_NO_MORE_DATA;
                    if (elem_idx >= ctx.qrltx.subitem_count) return REVIEW_NO_MORE_DATA;

                    qrltx_addr_block *dst = &ctx.qrltx.txtoken.dst[elem_idx];

                    if (view_idx % 2 == 0) {
                        print_key("Dst %d", elem_idx);
                        viewdata.value[0] = 'Q';
                        ARRTOHEX(viewdata.value + 1, dst->address);
                    } else {
                        print_key("Amount %d (QRL)", elem_idx);
                        // TODO: Decide what to do with token decimals
                        AMOUNT_TO_STR(viewdata.value, dst->amount, 0);
                    }
                    break;
                }
            }
            break;
        }
#endif
#ifdef SLAVE_ENABLED
        case QRLTX_SLAVE: {
            strcpy(viewdata.title, "CREATE SLAVE");

            switch (view_idx) {
                case 0: {
                    strcpy(viewdata.key, "Master Addr");
                    viewdata.value[0] = 'Q';
                    ARRTOHEX(viewdata.value + 1, ctx.qrltx.slave.master.address);
                    break;
                }
                case 1: {
                    strcpy(viewdata.key, "Fee (QRL)");
                    AMOUNT_TO_STR(viewdata.value,
                                  ctx.qrltx.slave.master.amount,
                                  QUANTA_DECIMALS);
                    break;
                }
                default: {
                    elem_idx = (view_idx - 2) >> 1;
                    if (elem_idx >= QRLTX_SUBITEM_MAX) EXIT_VIEW();
                    if (elem_idx >= ctx.qrltx.subitem_count) EXIT_VIEW();

                    if (view_idx % 2 == 0) {
                        print_key("Slave PK %d", elem_idx);
                        ARRTOHEX(viewdata.value, ctx.qrltx.slave.slaves[elem_idx].pk);
                    } else {
                        print_key("Access Type %d", elem_idx);
                        ARRTOHEX(viewdata.value, ctx.qrltx.slave.slaves[elem_idx].access);
                    }
                    break;
                }
            }
            break;
        }
#endif
        case QRLTX_MESSAGE: {
            strcpy(viewdata.title, "MESSAGE");

            switch (viewdata.idx) {
                case 0: {
                    strcpy(viewdata.key, "Source Addr");
                    viewdata.value[0] = 'Q';
                    ARRTOHEX(viewdata.value + 1,
                             ctx.qrltx.msg.master.address);
                    break;
                }
                case 1: {
                    strcpy(viewdata.key, "Fee (QRL)");
                    AMOUNT_TO_STR(viewdata.value,
                                  ctx.qrltx.msg.master.amount,
                                  QUANTA_DECIMALS);
                    break;
                }
                case 2: {
                    uint8_t numchars = ctx.qrltx.subitem_count;

                    if (numchars <= MAX_CHARS_HEXMESSAGE) {
                        print_key("Message");
                    } else {
                        numchars = MAX_CHARS_HEXMESSAGE;
                        print_key("Message [1/2]");
                    }
                    array_to_hexstr(viewdata.value,
                                    ctx.qrltx.msg.message,
                                    numchars);
                    break;
                }
                case 3: {
                    if (ctx.qrltx.subitem_count <= MAX_CHARS_HEXMESSAGE) {
                        return REVIEW_NO_MORE_DATA;
                    }
                    print_key("Message [2/2]");
                    array_to_hexstr(viewdata.value,
                                    ctx.qrltx.msg.message + MAX_CHARS_HEXMESSAGE,
                                    ctx.qrltx.subitem_count - MAX_CHARS_HEXMESSAGE);
                    break;
                }
                default: {
                    return REVIEW_NO_MORE_DATA;
                }
            }
            break;
        }
        default:
            return REVIEW_NO_MORE_DATA;
    }

    return REVIEW_DATA_AVAILABLE;
}
