/*
 * Copyright (c) 2012 Research In Motion Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RATEMYAPP_ADV_H_
#define _RATEMYAPP_ADV_H_

#ifndef _RMA_ADVANCED_MODE_
#error "To use the advanced version of RateMyApp, include ratemyapp_adv.h and define _RMA_ADVANCED_MODE_. To use the basic version, include ratemyapp.h and undefine _RMA_ADVANCED_MODE_."
#endif

#include <sys/platform.h>

__BEGIN_DECLS

/*
 * Debug level for RateMyApp:
 *   0 off
 *   1 print debug information
 *   2 print debug information and always show reminders
 */
#define RMA_DEBUG 1

enum RMAError {
	RMA_NO_ERROR = 0,
	RMA_NOT_RUNNING = 1,
	RMA_BPS_FAILURE = 2,
	RMA_FILE_ERROR = 3,
	RMA_MEMORY_ERROR = 4
};

/**
 * Returns the current error state
 */
RMAError rma_get_error();

/**
 * Start RateMyApp
 */
void rma_start();

/**
 * Stop RateMyApp
 */
void rma_stop();

/**
 * Updates the statistic for the number of times the app has been launched
 */
void rma_app_launched();

/**
 * Updates the statistic for the number of times the user performed a significant event
 */
void rma_app_significant_event();

/**
 * Returns true if the app has been reviewed by the user
 */
bool rma_is_rated();

/**
 * Records that the app has reviewed
 */
void rma_set_rated();

/**
 * Returns true if the user has postponed the review process
 */
bool rma_is_postponed();

/**
 * Records that the user wanted to postpone the review
 */
void rma_set_postponed();

/**
 * Returns total number of times the app has been launched
 */
int rma_launch_count();

/**
 * Returns total count of significant user events
 */
int rma_sig_event_count();

/**
 * Returns the time of app's first launch in Unix epoch format
 */
long long rma_first_launch_time();

/**
 * Returns the time of postponing the review process in Unix epoch format
 */
long long rma_postponed_time();

/**
 * Returns true if a network connection is available for accessing App World
 */
bool rma_network_available();

/**
 * Opens App World page for the specified app id
 */
void rma_open_app_world(unsigned int id);


__END_DECLS

#endif /* _RATEMYAPP_ADV_H_ */
