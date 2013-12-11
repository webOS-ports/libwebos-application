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

#ifndef LUNA_SERVICE_UTILS_H_
#define LUNA_SERVICE_UTILS_H_

#include <luna-service2/lunaservice.h>
#include <pbnjson.h>

void luna_service_message_reply_custom_error(LSHandle *handle, LSMessage *message, const char *error_text);
void luna_service_message_reply_error_unknown(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_bad_json(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_invalid_params(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_not_implemented(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_error_internal(LSHandle *handle, LSMessage *message);
void luna_service_message_reply_success(LSHandle *handle, LSMessage *message);

jvalue_ref luna_service_message_parse_and_validate(const char *payload);
bool luna_service_message_validate_and_send(LSHandle *handle, LSMessage *message, jvalue_ref reply_obj);
bool luna_service_check_for_subscription_and_process(LSHandle *handle, LSMessage *message);
void luna_service_post_subscription(LSHandle *handle, const char *path, const char *method, jvalue_ref reply_obj);

#endif

// vim:ts=4:sw=4:noexpandtab
