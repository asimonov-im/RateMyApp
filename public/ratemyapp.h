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

#include <sys/platform.h>

__BEGIN_DECLS

/*
 Place your BlackBerry App Store id here.
 */
#define RMA_APPWORLD_ID "appworld://content/95522"

/*
 Your app's name.
 */
#define RMA_APP_NAME "Frogatto"

/*
 This is the message your users will see once they've passed the day+launches
 threshold.
 */
#define RMA_MESSAGE	"If you enjoy playing Frogatto, would you mind taking a moment to rate it? It won't take more than a minute. Thanks for your support!"

/*
 The text of the button that rejects reviewing the app.
 */
#define RMA_CANCEL_BUTTON "No, Thanks"

/*
 Text of button that will send user to app review page.
 */
#define RMA_RATE_BUTTON	"Rate now"

/*
 Text for button to remind the user to review later.
 */
#define RMA_RATE_LATER "Rate later"

/*
 Users will need to have the same version of your app installed for this many
 days before they will be prompted to rate it.
 */
#define RMA_DAYS_UNTIL_PROMPT		0		// double

/*
 An example of a 'use' would be if the user launched the app. Bringing the app
 into the foreground (on devices that support it) would also be considered
 a 'use'. You tell Appirater about these events using the two methods:
 [Appirater appLaunched:]
 [Appirater appEnteredForeground:]

 Users need to 'use' the same version of the app this many times before
 before they will be prompted to rate it.
 */
#define RMA_USES_UNTIL_PROMPT		0		// integer

/*
 A significant event can be anything you want to be in your app. In a
 telephone app, a significant event might be placing or receiving a call.
 In a game, it might be beating a level or a boss. This is just another
 layer of filtering that can be used to make sure that only the most
 loyal of your users are being prompted to rate you on the app store.
 If you leave this at a value of -1, then this won't be a criteria
 used for rating. To tell Appirater that the user has performed
 a significant event, call the method:
 [Appirater userDidSignificantEvent:];
 */
#define RMA_SIG_EVENTS_UNTIL_PROMPT	-1	// integer

/*
 Once the rating alert is presented to the user, they might select
 'Remind me later'. This value specifies how long (in days) Appirater
 will wait before reminding them.
 */
#define RMA_TIME_BEFORE_REMINDING		1	// double

/*
 'YES' will show the Appirater alert everytime. Useful for testing how your message
 looks and making sure the link to your app's review page works.
 */
#define RMA_DEBUG 1

/**
 * Start the RateMyApp service.
 */
void rma_start();

/**
 * Stop the RateMyApp service.
 */
void rma_stop();

/**
 * Indicate that the app has been launched
 */
void rma_app_launched(bool show);

/**
 * Indicate that the user performed a significant event
 */
void rma_app_significant_event(bool show);

__END_DECLS

#endif /* _RATEMYAPP_H_ */
