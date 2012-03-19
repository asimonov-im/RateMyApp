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

#ifndef _RATEMYAPP_H_
#define _RATEMYAPP_H_

#ifdef __RMA_ADVANCED_MODE_
#error "To use the advanced version of RateMyApp, include ratemyapp_adv.h and define _RMA_ADVANCED_MODE_. To use the basic version, include ratemyapp.h and undefine _RMA_ADVANCED_MODE_."
#endif

#include <sys/platform.h>

__BEGIN_DECLS

/*
 * The application's BlackBerry App World id
 */
#define RMA_APPWORLD_ID                  95522  // unsigned integer

/*
 * The reminder message the user will see once they've passed the day+launches threshold.
 */
#define RMA_MESSAGE	"If you enjoy playing Frogatto, would you mind taking a moment to rate it? It won't take more than a minute. Thanks for your support!"

/*
 * The text of the button that rejects reviewing the app.
 */
#define RMA_CANCEL_BUTTON "No, Thanks"

/*
 * Text of button that will send user to app review page.
 */
#define RMA_RATE_BUTTON	"Rate Now"

/*
 * Text for button to remind the user to review later.
 */
#define RMA_LATER_BUTTON "Rate Later"

/*
 * Number of days the app must be installed before the user is prompted for a review.
 */
#define RMA_DAYS_UNTIL_PROMPT            0  // double

/*
 * Number of times the app must be launched before the review reminder is triggered.
 * To indicate a successful app launch, call rma_app_launched
 */
#define RMA_USES_UNTIL_PROMPT            0  // integer

/*
 * A significant event can be anything you want to be in your app, such
 * as beating a level or a boss. This is just another
 * layer of filtering for review reminders.
 * To indicate the occurrence of a significant event, call rma_app_significant_event
 */
#define RMA_SIG_EVENTS_UNTIL_PROMPT     -1  // integer

/*
 * Number of days to wait before presenting the review reminder dialogue again
 * after the user postpones reviewing the app.
 */
#define RMA_TIME_BEFORE_REMINDING		 1  // double

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
 * Starts RateMyApp
 */
void rma_start();

/**
 * StopsRateMyApp
 */
void rma_stop();

/**
 * Indicates that the app has been launched successfully
 * If enableReminder is set to true, a review reminder alert may be displayed
 * if all necessary conditions are satisfied.
 * If enableReminder is set to false, only the statistics will be updated but
 * no reminder will be presented.
 */
void rma_app_launched(bool enableReminder);

/**
 * Indicates that the user performed a significant event
 * If enableReminder is set to true, a review reminder alert may be displayed
 * if all necessary conditions are satisfied.
 * If enableReminder is set to false, only the statistics will be updated but
 * no reminder will be presented.
 */
void rma_app_significant_event(bool enableReminder);

__END_DECLS

#endif /* _RATEMYAPP_H_ */
