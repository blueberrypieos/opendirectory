/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */
#ifndef _PIE_ODXPC_H_
#define _PIE_ODXPC_H_

#include <stdbool.h>
#include <stdint.h>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>

typedef void (^odxpc_handler_t)(CFPropertyListRef plist, uint64_t error, bool complete);

void odxpc_send_message_with_reply(uint64_t reqtype, const uint8_t *session, const uint8_t *node,
    CFDataRef data, dispatch_queue_t replyq, odxpc_handler_t handler);

#endif
