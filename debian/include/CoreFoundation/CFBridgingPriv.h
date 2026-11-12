/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */
#ifndef __COREFOUNDATION_CFBRIDGINGPRIV__
#define __COREFOUNDATION_CFBRIDGINGPRIV__

#include <CoreFoundation/CFRuntime.h>

#define CF_IS_OBJC(typeID, obj) (0)
#define CF_OBJC_CALL(rettype, retvar, obj, sel, ...) do { } while (0)
#define CF_OBJC_FUNCDISPATCH(typeID, rettype, obj, sel, ...) do { } while (0)
#define _CFRuntimeBridgeClasses(typeID, name) ((void)0)

#endif
