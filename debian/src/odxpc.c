/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 The Blueberry Pie OS Authors
 */
#include <stdlib.h>
#include <unistd.h>
#include <Block.h>
#include <xpc/xpc.h>
#include <opendirectory/odipc.h>
#include <CFOpenDirectory/CFOpenDirectoryConstants.h>

#include "odxpc.h"

struct od_conn {
	struct od_conn *next;
	uid_t euid;
	xpc_connection_t xconn;
};

struct od_request {
	struct od_request *next;
	uint64_t id;
	struct od_conn *conn;
	dispatch_queue_t replyq;
	odxpc_handler_t handler;
};

static dispatch_queue_t od_queue;
static struct od_conn *od_conns;
static struct od_request *od_requests;
static uint64_t od_last_id;
static const uint8_t od_null_uuid[16];

static CFPropertyListRef
copy_reply_plist(xpc_object_t msg)
{
	const void *bytes;
	size_t len = 0;
	CFDataRef data;
	CFPropertyListRef plist = NULL;
	CFPropertyListFormat format = 0;

	bytes = xpc_dictionary_get_data(msg, kODXPCReplyData, &len);
	if (bytes == NULL || len == 0) {
		return NULL;
	}
	data = CFDataCreate(NULL, bytes, (CFIndex)len);
	if (data == NULL) {
		return NULL;
	}
	plist = CFPropertyListCreateWithData(NULL, data, kCFPropertyListImmutable, &format, NULL);
	CFRelease(data);
	if (plist != NULL && format != kCFPropertyListBinaryFormat_v1_0) {
		CFRelease(plist);
		plist = NULL;
	}
	return plist;
}

static void
deliver(struct od_request *req, CFPropertyListRef plist, uint64_t error, bool complete)
{
	dispatch_queue_t replyq = req->replyq;
	odxpc_handler_t handler = req->handler;

	if (complete) {
		struct od_request **pp;

		for (pp = &od_requests; *pp != NULL; pp = &(*pp)->next) {
			if (*pp == req) {
				*pp = req->next;
				break;
			}
		}
		free(req);
	}

	dispatch_async(replyq, ^{
		handler(plist, error, complete);
		if (plist != NULL) {
			CFRelease(plist);
		}
		if (complete) {
			Block_release(handler);
			dispatch_release(replyq);
		}
	});
}

static void
drop_conn(struct od_conn *conn)
{
	struct od_conn **pp;

	for (pp = &od_conns; *pp != NULL; pp = &(*pp)->next) {
		if (*pp == conn) {
			*pp = conn->next;
			break;
		}
	}
	xpc_connection_cancel(conn->xconn);
	xpc_release(conn->xconn);
	free(conn);
}

static void
handle_event(struct od_conn *conn, xpc_object_t event)
{
	struct od_request *req, *next;
	xpc_type_t type = xpc_get_type(event);

	if (type == XPC_TYPE_DICTIONARY) {
		uint64_t id = xpc_dictionary_get_uint64(event, kODXPCReplyClientID);

		for (req = od_requests; req != NULL; req = req->next) {
			if (req->id == id && req->conn == conn) {
				break;
			}
		}
		if (req != NULL) {
			deliver(req, copy_reply_plist(event), xpc_dictionary_get_uint64(event, kODXPCReplyError),
			    xpc_dictionary_get_bool(event, kODXPCReplyComplete));
		}
		return;
	}

	if (type != XPC_TYPE_ERROR) {
		return;
	}

	for (req = od_requests; req != NULL; req = next) {
		next = req->next;
		if (req->conn == conn) {
			deliver(req, NULL, kODErrorDaemonError, true);
		}
	}
	if (event == XPC_ERROR_CONNECTION_INVALID) {
		drop_conn(conn);
	}
}

static struct od_conn *
conn_for_euid(uid_t euid)
{
	struct od_conn *conn;

	for (conn = od_conns; conn != NULL; conn = conn->next) {
		if (conn->euid == euid) {
			return conn;
		}
	}

	conn = calloc(1, sizeof(*conn));
	if (conn == NULL) {
		return NULL;
	}
	conn->euid = euid;
	conn->xconn = xpc_connection_create_mach_service(kODAPIPortName, od_queue,
	    XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);
	if (conn->xconn == NULL) {
		free(conn);
		return NULL;
	}
	xpc_connection_set_event_handler(conn->xconn, ^(xpc_object_t event) {
		handle_event(conn, event);
	});
	xpc_connection_resume(conn->xconn);
	conn->next = od_conns;
	od_conns = conn;

	return conn;
}

static void
set_uuid(xpc_object_t msg, const char *key, const uint8_t *uuid)
{
	xpc_object_t value = xpc_uuid_create(uuid != NULL ? uuid : od_null_uuid);

	xpc_dictionary_set_value(msg, key, value);
	xpc_release(value);
}

void
odxpc_send_message_with_reply(uint64_t reqtype, const uint8_t *session, const uint8_t *node,
    CFDataRef data, dispatch_queue_t replyq, odxpc_handler_t handler)
{
	static dispatch_once_t once;

	dispatch_once(&once, ^{
		od_queue = dispatch_queue_create("org.blueberrypieos.opendirectory.odxpc", NULL);
	});

	dispatch_sync(od_queue, ^{
		struct od_request *req;
		struct od_conn *conn;
		xpc_object_t msg;

		req = calloc(1, sizeof(*req));
		if (req == NULL) {
			abort();
		}
		req->id = ++od_last_id;
		req->replyq = replyq;
		dispatch_retain(replyq);
		req->handler = Block_copy(handler);
		req->next = od_requests;
		od_requests = req;

		conn = conn_for_euid(geteuid());
		if (conn == NULL) {
			deliver(req, NULL, kODErrorSessionDaemonNotRunning, true);
			return;
		}
		req->conn = conn;

		msg = xpc_dictionary_create(NULL, NULL, 0);
		xpc_dictionary_set_uint64(msg, kODXPCMessageClientID, req->id);
		xpc_dictionary_set_uint64(msg, kODXPCMessageReqType, reqtype);
		set_uuid(msg, kODXPCMessageSession, session);
		set_uuid(msg, kODXPCMessageNode, node);
		xpc_dictionary_set_data(msg, kODXPCMessageRequest, CFDataGetBytePtr(data), (size_t)CFDataGetLength(data));
		xpc_connection_send_message(conn->xconn, msg);
		xpc_release(msg);
	});
}
