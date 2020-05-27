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

#include <string.h>
#include <glib.h>
#include <lunaservice.h>
#include <pbnjson.h>

#include "webos_application.h"
#include "luna_service_utils.h"

struct webos_application_config {
	bool registered;
	char *app_id;
	struct webos_application_event_handlers *event_handlers;
	void *user_data;
	LSHandle *service_handle;
};

struct webos_application_config *app_config = NULL;

bool webos_application_get_handle(char **app_id, LSHandle **service_handle)
{
	if(!app_id || !service_handle) return false;
	
	*app_id = NULL;
	*service_handle = NULL;

	if(app_config) {
		*app_id = g_strdup(app_config->app_id);
		*service_handle = app_config->service_handle;

		return true;
	}

	return false;
}

static bool register_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	const char *payload;
	jvalue_ref parsed_obj = NULL;
	jvalue_ref return_value_obj = NULL;
	jvalue_ref subscribed_obj = NULL;
	jvalue_ref event_obj = NULL;
	jvalue_ref parameters_obj = NULL;
	jvalue_ref state_obj = NULL;
	raw_buffer event_buf;
	raw_buffer parameters_buf;
	raw_buffer state_buf;
	jvalue_ref reply_obj = NULL;
	enum webos_applocation_low_memory_state lowmemory_state = WEBOS_APPLICATION_LOW_MEMORY_NORMAL;
	bool return_value;
	bool subscribed;

	payload = LSMessageGetPayload(message);
	parsed_obj = luna_service_message_parse_and_validate(payload);
	if (jis_null(parsed_obj))
		goto cleanup;

	if (!app_config->registered) {
		if (!jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("returnValue"), &return_value_obj) ||
			!jis_boolean(return_value_obj))
			goto cleanup;

		if (!jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("subscribed"), &subscribed_obj) ||
			!jis_boolean(subscribed_obj))
			goto cleanup;

		jboolean_get(return_value_obj, &return_value);
		jboolean_get(subscribed_obj, &subscribed);

		if (return_value && subscribed)
			app_config->registered = true;

		goto cleanup;
	}

	app_config->registered = true;

	if (!jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("event"), &event_obj) ||
		!jis_string(event_obj))
		goto cleanup;

	event_buf = jstring_get(event_obj);

	if (g_strcmp0(event_buf.m_str, "relaunched") == 0 && app_config->event_handlers->relaunch) {
		if (!jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("parameters"), &parameters_obj) ||
			!jis_string(parameters_obj))
			goto cleanup;

		parameters_buf = jstring_get(parameters_obj);

		if (app_config->event_handlers && app_config->event_handlers->relaunch)
			app_config->event_handlers->relaunch(parameters_buf.m_str, app_config->user_data);
	}
	else if (g_strcmp0(event_buf.m_str, "activating") == 0) {
		if (app_config->event_handlers && app_config->event_handlers->activate)
			app_config->event_handlers->activate(app_config->user_data);
	}
	else if (g_strcmp0(event_buf.m_str, "deactivating") == 0) {
		if (app_config->event_handlers && app_config->event_handlers->deactivate)
			app_config->event_handlers->deactivate(app_config->user_data);
	}
	else if (g_strcmp0(event_buf.m_str, "suspending") == 0) {
		if (app_config->event_handlers && app_config->event_handlers->suspend)
			app_config->event_handlers->suspend(app_config->user_data);
	}
	else if (g_strcmp0(event_buf.m_str, "lowmemory") == 0) {
			if (!jobject_get_exists(parsed_obj, J_CSTR_TO_BUF("state"), &state_obj) ||
				!jis_string(state_obj))
				goto cleanup;

			state_buf = jstring_get(state_obj);

			if (g_strcmp0(state_buf.m_str, "normal") == 0)
				lowmemory_state = WEBOS_APPLICATION_LOW_MEMORY_NORMAL;
			else if (g_strcmp0(state_buf.m_str, "low") == 0)
				lowmemory_state = WEBOS_APPLICATION_LOW_MEMORY_LOW;
			else if (g_strcmp0(state_buf.m_str, "critical") == 0)
				lowmemory_state = WEBOS_APPLICATION_LOW_MEMORY_CRITICAL;

		if (app_config->event_handlers && app_config->event_handlers->lowmemory)
			app_config->event_handlers->lowmemory(lowmemory_state, app_config->user_data);
	}
	else {
		g_warning("Got unknown event from application manager: %s", event_buf.m_str);
	}

cleanup:
	if (!jis_null(parsed_obj))
		j_release(&parsed_obj);

	return true;
}

bool webos_application_init(const char *app_id, const char *service_name, struct webos_application_event_handlers *event_handlers, void *user_data)
{
	LSHandle *service_handle;
	LSError lserror;
	char *payload;

	if (app_config != NULL)
		return false;

	if (app_id == NULL || strlen(app_id) == 0)
		return false;

	if (service_name == NULL)
		service_name = app_id;

	LSErrorInit(&lserror);

	if (!LSRegister(service_name, &service_handle, &lserror)) {
		g_warning("Failed to register LS2 service object: %s", lserror.message);
		LSErrorFree(&lserror);
		return false;
	}

	app_config = g_new0(struct webos_application_config, 1);
	if (!app_config)
		return false;

	app_config->app_id = g_strdup(app_id);
	app_config->event_handlers = event_handlers;
	app_config->user_data = user_data;
	app_config->service_handle = service_handle;

	payload = g_strdup_printf("{\"subscribe\":true,\"appId\":\"%s\"}", app_config->app_id);

    if (!LSCall(app_config->service_handle, "luna://com.palm.applicationManager/registerApplication",
                payload, register_cb, NULL, NULL, &lserror)) {
		g_warning("Failed to register application events handler: %s", lserror.message);
		LSErrorFree(&lserror);
		webos_application_cleanup();
		g_free(payload);
		return false;
	}

	g_free(payload);

	return true;
}

bool webos_application_attach(GMainLoop *mainloop)
{
	LSError lserror;

	if (!app_config)
		return false;

	if (!app_config->service_handle)
		return false;

	LSErrorInit(&lserror);

	if (!LSGmainAttach(app_config->service_handle, mainloop, &lserror)) {
		g_warning("Failed to attach to provided glib mainloop: %s", lserror.message);
		LSErrorFree(&lserror);
		return false;
	}

	return true;
}

void webos_application_cleanup(void)
{
	if (!app_config)
		return;

	if (app_config->app_id)
		g_free(app_config->app_id);

	if (app_config->service_handle)
		LSUnregister(app_config->service_handle, NULL);

	g_free(app_config);
	app_config = NULL;
}
