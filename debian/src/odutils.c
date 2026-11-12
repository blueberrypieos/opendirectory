/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */
#include <opendirectory/odutils.h>

const uuid_t kODNullUUID;

static CFArrayRef
wrap_value(CFTypeRef value)
{
	return CFArrayCreate(NULL, value ? &value : NULL, value ? 1 : 0, &kCFTypeArrayCallBacks);
}

CFDictionaryRef
schema_construct_requestv(CFStringRef funcname, CFIndex count, va_list ap)
{
	CFMutableArrayRef params;
	CFMutableDictionaryRef request;
	CFIndex i;

	if (funcname == NULL || count < 0) {
		return NULL;
	}

	params = CFArrayCreateMutable(NULL, count, &kCFTypeArrayCallBacks);
	for (i = 0; i < count; i++) {
		CFArrayRef slot = wrap_value(va_arg(ap, CFTypeRef));
		CFArrayAppendValue(params, slot);
		CFRelease(slot);
	}

	request = CFDictionaryCreateMutable(NULL, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(request, CFSTR("funcname"), funcname);
	CFDictionarySetValue(request, CFSTR("parameters"), params);
	CFRelease(params);

	return request;
}

CFDictionaryRef
schema_construct_request(CFStringRef funcname, CFIndex count, ...)
{
	CFDictionaryRef request;
	va_list ap;

	va_start(ap, count);
	request = schema_construct_requestv(funcname, count, ap);
	va_end(ap);

	return request;
}

void
schema_deconstruct_result(CFDictionaryRef result, CFStringRef *funcname, CFArrayRef *values)
{
	CFTypeRef name = NULL;
	CFTypeRef results = NULL;

	if (result != NULL && CFGetTypeID(result) == CFDictionaryGetTypeID()) {
		name = CFDictionaryGetValue(result, CFSTR("funcname"));
		results = CFDictionaryGetValue(result, CFSTR("results"));
	}
	if (name != NULL && CFGetTypeID(name) != CFStringGetTypeID()) {
		name = NULL;
	}
	if (results != NULL && CFGetTypeID(results) != CFArrayGetTypeID()) {
		results = NULL;
	}

	if (funcname != NULL) {
		*funcname = name ? CFRetain(name) : NULL;
	}
	if (values != NULL) {
		*values = results ? CFRetain(results) : NULL;
	}
}

CFTypeRef
schema_get_value_at_index(CFArrayRef values, CFIndex index)
{
	CFTypeRef slot;

	if (values == NULL || index < 0 || index >= CFArrayGetCount(values)) {
		return NULL;
	}

	slot = CFArrayGetValueAtIndex(values, index);
	if (slot == NULL || CFGetTypeID(slot) != CFArrayGetTypeID() || CFArrayGetCount(slot) != 1) {
		return NULL;
	}

	return CFArrayGetValueAtIndex(slot, 0);
}
