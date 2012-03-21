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

#ifndef _APPRAISEME_H_
#define _APPRAISEME_H_

#include <sys/platform.h>

__BEGIN_DECLS

/*
 * The application's BlackBerry App World id
 */
#define APPRAISE_APPWORLD_ID                95522  // unsigned integer

/*
 * The reminder message the user will see once they've passed the day+launches threshold.
 */
#define APPRAISE_MESSAGE	"If you enjoy playing APPNAME, would you mind taking a moment to rate it? It won't take more than a minute. Thanks for your support!"

/*
 * The text of the button that rejects reviewing the app.
 */
#define APPRAISE_CANCEL_BUTTON "No, Thanks"

/*
 * Text of button that will send user to app review page.
 */
#define APPRAISE_RATE_BUTTON	"Rate Now"

/*
 * Text for button to remind the user to review later.
 */
#define APPRAISE_LATER_BUTTON "Rate Later"

/*
 * Number of days the app must be installed before the user is prompted for a review.
 */
#define APPRAISE_DAYS_UNTIL_PROMPT            0.0  // double

/*
 * Number of times the app must be launched before the review reminder is triggered.
 * To indicate a successful app launch, call appraise_app_launched
 */
#define APPRAISE_USES_UNTIL_PROMPT            3  // integer

/*
 * A significant event can be anything you want to be in your app, such
 * as beating a level or a boss. This is just another
 * layer of filtering for review reminders.
 * To indicate the occurrence of a significant event, call appraise_app_significant_event
 */
#define APPRAISE_SIG_EVENTS_UNTIL_PROMPT     -1  // integer

/*
 * Number of days to wait before presenting the review reminder dialogue again
 * after the user postpones reviewing the app.
 */
#define APPRAISE_TIME_BEFORE_REMINDING		 0.0  // double

/*
 * Debug level for AppRaiseMe:
 *   0 off
 *   1 print debug information
 *   2 print debug information and always show reminders
 */
#define APPRAISE_DEBUG 1

struct RemindConditions {
	double daysInUse;
	int launchCount;
	int sigEventCount;
	double daysToPostpone;
};

enum AppRaiseErr {
	APPRAISE_NO_ERROR = 0,
	APPRAISE_NOT_RUNNING = 1,
	APPRAISE_BPS_ERROR = 2,
	APPRAISE_READ_ERROR = 3,
	APPRAISE_WRITE_ERROR = 4
};

/**
 * Returns the current error state
 */
enum AppRaiseErr appraise_get_error();

/**
 * Stops AppRaiseMe
 */
void appraise_stop();

#ifndef _APPRAISE_ADVANCED_MODE_
/**
 * Starts AppRaiseMe and increments the launch counter
 * If enableReminder is set to true, a review reminder alert may be displayed
 * if all necessary conditions are satisfied.
 * If enableReminder is set to false, only the statistics will be updated but
 * no reminder will be presented.
 */
void appraise_start(bool enableReminder);

/**
 * Indicates that the user performed a significant event
 * If enableReminder is set to true, a review reminder alert may be displayed
 * if all necessary conditions are satisfied.
 * If enableReminder is set to false, only the statistics will be updated but
 * no reminder will be presented.
 */
void appraise_app_significant_event(bool enableReminder);

#else
/**
 * Start AppRaiseMe
 */
void appraise_start();

/**
 * Sets the conditions for when a review reminder can be displayed.
 * The conditions are lost when AppRaiseMe is shut down.
 */
void appraise_set_conditions(RemindConditions conditions);

/**
 * Returns true if all conditions for a review reminder have been met.
 */
bool appraise_check_conditions();

/**
 * Returns true if the app has been reviewed by the user
 */
bool appraise_is_rated();

/**
 * Sets the rated status to val
 */
void appraise_set_rated(bool val);

/**
 * Returns true if the user has postponed the review process
 */
bool appraise_is_postponed();

/**
 * Sets the postponed status to val
 */
void appraise_set_postponed(bool val);

/**
 * Returns total number of times the app has been launched
 */
int appraise_launch_count();

/**
 * Returns total count of significant user events
 */
int appraise_sig_event_count();

/**
 * Returns the time of app's first launch in Unix epoch format
 */
long long appraise_first_launch_time();

/**
 * Returns the time of postponing the review process in Unix epoch format
 */
long long appraise_postponed_time();

/**
 * Returns true if a network connection is available for accessing App World
 */
bool appraise_network_available();

/**
 * Opens App World page for the specified app id
 */
void appraise_open_app_world(unsigned int id);

#endif

__END_DECLS

#endif /* _APPRAISEME_H_ */
