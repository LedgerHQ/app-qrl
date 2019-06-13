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
#pragma once

#include "os.h"
#include "cx.h"

#if defined(TARGET_NANOX)
#define MAX_CHARS_PER_KEY_LINE      64
#define MAX_CHARS_PER_VALUE_LINE    256
#define MAX_CHARS_HEXMESSAGE        100
#else
#define MAX_CHARS_PER_KEY_LINE      32
#define MAX_CHARS_PER_VALUE_LINE    128
#define MAX_CHARS_HEXMESSAGE        40
#endif

typedef struct {
    char title[16];
    char key[MAX_CHARS_PER_KEY_LINE];
    char value[MAX_CHARS_PER_VALUE_LINE];
    int8_t idx;
} view_t;

extern view_t viewdata;

void view_init(void);
void view_idle_show(void);
void view_sign_show();
void view_review_show();
void view_setidx_show();
void view_address_show();

#define print_key(...) snprintf(viewdata.key, sizeof(viewdata.key), __VA_ARGS__);
#define print_status(...) snprintf(viewdata.value, sizeof(viewdata.value), __VA_ARGS__);

#if defined(TARGET_NANOX)
#define CUR_FLOW G_ux.flow_stack[G_ux.stack_count-1]
#endif

void view_update_state();
int8_t view_update_review();

