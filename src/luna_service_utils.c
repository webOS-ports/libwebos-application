/*
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "luna_service_utils.h"

void luna_service_message_reply_custom_error(LSHandle *handle, LSMessage *message, const char *error_text)
{
	bool ret;
	LSError lserror;
	char *payload;

	LSErrorInit(&lserror);

	payload = g_strdup_printf("{\"returnValue\":false, \"errorText\":\"%s\"}", error_text);

	ret = LSMessageReply(handle, message, payload, &lserror);
	if (!ret) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	g_free(payload);
}

void luna_service_message_reply_error_unknown(LSHandle *handle, LSMessage *message)
{
	luna_service_message_reply_custom_error(handle, message, "Unknown Error.");
}

void luna_service_message_reply_error_bad_json(LSHandle *handle, LSMessage *message)
{
	luna_service_message_reply_custom_error(handle, message, "Malformed json.");
}

void luna_service_message_reply_error_invalid_params(LSHandle *handle, LSMessage *message)
{
	luna_service_message_reply_custom_error(handle, message, "Invalid parameters.");
}

void luna_service_message_reply_error_not_implemented(LSHandle *handle, LSMessage *message)
{
	luna_service_message_reply_custom_error(handle, message, "Not implemented.");
}

void luna_service_message_reply_error_internal(LSHandle *handle, LSMessage *message)
{
	luna_service_message_reply_custom_error(handle, message, "Internal error.");
}

void luna_service_message_reply_success(LSHandle *handle, LSMessage *message)
{
	bool ret;
	LSError lserror;

	LSErrorInit(&lserror);

	ret = LSMessageReply(handle, message, "{\"returnValue\":true}", &lserror);
	if (!ret) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}
}

jvalue_ref luna_service_message_parse_and_validate(const char *payload)
{
	jschema_ref input_schema = NULL;
	jvalue_ref parsed_obj = NULL;
	JSchemaInfo schema_info;

	input_schema = jschema_parse(j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
	jschema_info_init(&schema_info, input_schema, NULL, NULL);

	parsed_obj = jdom_parse(j_cstr_to_buffer(payload), DOMOPT_NOOPT, &schema_info);

	jschema_release(&input_schema);
	if (jis_null(parsed_obj))
		return NULL;

	return parsed_obj;
}

bool luna_service_message_validate_and_send(LSHandle *handle, LSMessage *message, jvalue_ref reply_obj)
{
	jschema_ref response_schema = NULL;
	LSError lserror;
	bool success = true;

	LSErrorInit(&lserror);

	response_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
	if(!response_schema) {
		luna_service_message_reply_error_internal(handle, message);
		return false;
	}

	if (!LSMessageReply(handle, message,
					jvalue_tostring(reply_obj, response_schema), &lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
		success = false;
	}

	jschema_release(&response_schema);

	return success;
}

bool luna_service_check_for_subscription_and_process(LSHandle *handle, LSMessage *message)
{
	LSError lserror;
	bool subscribed = false;

	LSErrorInit(&lserror);

	if (LSMessageIsSubscription(message)) {
		if (!LSSubscriptionProcess(handle, message, &subscribed, &lserror)) {
			LSErrorPrint(&lserror, stderr);
			LSErrorFree(&lserror);
		}
	}

	return subscribed;
}

void luna_service_post_subscription(LSHandle *handle, const char *path, const char *method, jvalue_ref reply_obj)
{
	jschema_ref response_schema = NULL;
	LSError lserror;

	LSErrorInit(&lserror);

	response_schema = jschema_parse (j_cstr_to_buffer("{}"), DOMOPT_NOOPT, NULL);
	if(!response_schema)
		goto cleanup;

	if (!LSSubscriptionPost(handle, path, method,
						jvalue_tostring(reply_obj, response_schema), &lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

cleanup:
	if (response_schema)
		jschema_release(&response_schema);
}

// vim:ts=4:sw=4:noexpandtab
