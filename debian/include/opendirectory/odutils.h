/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */
#ifndef _OPENDIRECTORY_ODUTILS_H_
#define _OPENDIRECTORY_ODUTILS_H_

#include <stdarg.h>
#include <uuid/uuid.h>
#include <CoreFoundation/CoreFoundation.h>

__BEGIN_DECLS

extern const uuid_t kODNullUUID;

CFDictionaryRef schema_construct_request(CFStringRef funcname, CFIndex count, ...);
CFDictionaryRef schema_construct_requestv(CFStringRef funcname, CFIndex count, va_list ap);
void schema_deconstruct_result(CFDictionaryRef result, CFStringRef *funcname, CFArrayRef *values);
CFTypeRef schema_get_value_at_index(CFArrayRef values, CFIndex index);

__END_DECLS

#endif
