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

static bool handle_event_cb(LSHandle *handle, LSMessage *message, void *user_data);

static LSMethod application_methods[] = {
	{ "handleEvent", handle_event_cb },
	{ NULL, NULL }
};

static bool handle_event_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	const char *payload;
	jvalue_ref parsed_obj = NULL;
	jvalue_ref event_obj = NULL;
	jvalue_ref parameters_obj = NULL;
	jvalue_ref state_obj = NULL;
	raw_buffer event_buf;
	raw_buffer parameters_buf;
	raw_buffer state_buf;
	jvalue_ref reply_obj = NULL;
	enum webos_applocation_low_memory_state lowmemory_state = WEBOS_APPLICATION_LOW_MEMORY_NORMAL;

	payload = LSMessageGetPayload(message);
	parsed_obj = luna_service_message_parse_and_validate(payload);
	if (jis_null(parsed_obj))
		goto cleanup;

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

	reply_obj = jobject_create();
	jobject_put(reply_obj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));

	if (!luna_service_message_validate_and_send(handle, message, reply_obj))
		goto cleanup;

cleanup:
	if (!jis_null(parsed_obj))
		j_release(&parsed_obj);

	return true;
}

static bool register_cb(LSHandle *handle, LSMessage *message, void *user_data)
{
	app_config->registered = true;

	return true;
}

bool webos_application_init(const char *app_id, struct webos_application_event_handlers *event_handlers, void *user_data)
{
	LSHandle *service_handle;
	LSError lserror;

	if (app_config != NULL)
		return false;

	if (app_id == NULL || strlen(app_id) == 0)
		return false;

	LSErrorInit(&lserror);

	if (!LSRegister(app_id, &service_handle, &lserror)) {
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

	if (!LSRegisterCategory(service_handle, "/", application_methods,
							NULL, NULL, &lserror)) {
		g_warning("Could not register service category: %s", lserror.message);
		LSErrorFree(&lserror);
		webos_application_cleanup();
		return false;
	}

#if 0
	if (!LSCall(app_config->service_handle, "luna://com.palm.applicationManager/registerApplicationEventHandler",
				"{}", register_cb, NULL, NULL, &lserror)) {
		g_warning("Failed to register application events handler: %s", lserror.message);
		LSErrorFree(&lserror);
		webos_application_cleanup();
		return false;
	}
#endif

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
