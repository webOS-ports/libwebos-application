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

#ifndef WEBOS_APPLICATION_H_
#define WEBOS_APPLICATION_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum webos_applocation_low_memory_state {
	WEBOS_APPLICATION_LOW_MEMORY_NORMAL,
	WEBOS_APPLICATION_LOW_MEMORY_LOW,
	WEBOS_APPLICATION_LOW_MEMORY_CRITICAL
};

struct webos_application_event_handlers {
	void (*activate)(void *user_data);
	void (*deactivate)(void *user_data);
	void (*suspend)(void *user_data);
	void (*relaunch)(const char *parameters, void *user_data);
	void (*lowmemory)(enum webos_applocation_low_memory_state state, void *user_data);
};

bool webos_application_init(const char *app_id, struct webos_application_event_handlers *event_handlers, void *user_data);
bool webos_application_attach(GMainLoop *mainloop);
void webos_application_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
