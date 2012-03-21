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

#include "appraiseme.h"
#include "appraiseme_priv.h"
#include <bps/bps.h>
#include "bps/netstatus.h"
#include "bps/dialog.h"
#include "bps/navigator.h"
#include <stdlib.h>
#include <ctime>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>

#define EXTERNAL_API extern "C"

static const unsigned int statVersion = 1;

AppRaiseMe *AppRaiseMe::s_appraiseInstance = NULL;
dialog_instance_t AppRaiseMe::s_alertDialog = NULL;
AppRaiseErr AppRaiseMe::s_errCode = APPRAISE_NOT_RUNNING;

// In case of problems, returns values that will avoid a rating reminder
const bool dRated = true;
const bool dPostponed = true;
const int dLaunchCount = std::numeric_limits<int>::min();
const int dSigEventCount = std::numeric_limits<int>::min();
const long long dFirstLaunchTime = std::numeric_limits<long long>::max();
const long long dPostponedTime = std::numeric_limits<long long>::max();
const bool dNetworkAvailable = false;

EXTERNAL_API enum AppRaiseErr appraise_get_error()
{
	return AppRaiseMe::getError();
}

EXTERNAL_API void appraise_stop()
{
	AppRaiseMe::destroyInstance();
}

#ifndef _APPRAISE_ADVANCED_MODE_
EXTERNAL_API void appraise_start(bool enableReminder)
{
	RemindConditions defConditions;
	defConditions.daysInUse = APPRAISE_DAYS_UNTIL_PROMPT;
	defConditions.launchCount = APPRAISE_USES_UNTIL_PROMPT;
	defConditions.sigEventCount = APPRAISE_SIG_EVENTS_UNTIL_PROMPT;
	defConditions.daysToPostpone = APPRAISE_TIME_BEFORE_REMINDING;

	AppRaiseMe::createInstance(enableReminder, &defConditions);
}

EXTERNAL_API void appraise_significant_event(bool enableReminder)
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		AppRaiseMe::getInstance()->appSignificantEvent(enableReminder);
	}
}

#else
EXTERNAL_API void appraise_start()
{
	AppRaiseMe::createInstance(false, NULL);
}

EXTERNAL_API void appraise_set_conditions(RemindConditions conditions)
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		AppRaiseMe::getInstance()->setConditions(&conditions);
	}
}

EXTERNAL_API bool appraise_check_conditions()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->conditionsMet();
	} else {
		return false;
	}
}

EXTERNAL_API void appraise_significant_event()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		AppRaiseMe::getInstance()->appSignificantEvent(false);
	}
}

EXTERNAL_API bool appraise_is_rated()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->isRated();
	} else {
		return dRated;
	}
}

EXTERNAL_API void appraise_set_rated(bool val)
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		AppRaiseMe::getInstance()->setRated(val);
	}
}

EXTERNAL_API bool appraise_is_postponed()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->isPostponed();
	} else {
		return dPostponed;
	}
}

EXTERNAL_API void appraise_set_postponed(bool val)
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		AppRaiseMe::getInstance()->setPostponed(val);
	}
}

EXTERNAL_API int appraise_launch_count()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->launchCount();
	} else {
		return dLaunchCount;
	}
}

EXTERNAL_API int appraise_significant_event_count()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->sigEventCount();
	} else {
		return dSigEventCount;
	}
}

EXTERNAL_API long long appraise_first_launch_time()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->firstLaunchTime();
	} else {
		return dFirstLaunchTime;
	}
}

EXTERNAL_API long long appraise_postponed_time()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->postponedTime();
	} else {
		return dPostponedTime;
	}
}

EXTERNAL_API bool appraise_network_available()
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->networkAvailable();
	} else {
		return dNetworkAvailable;
	}
}

EXTERNAL_API void appraise_open_app_world(unsigned int id)
{
	if (AppRaiseMe::getError() == APPRAISE_NO_ERROR) {
		return AppRaiseMe::getInstance()->openAppWorld(id);
	}
}
#endif

AppRaiseMe* AppRaiseMe::getInstance()
{
	// Calls to getInstance must ALWAYS be guarded with a check against APPRAISE_NOT_RUNNING
	// to guarantee that we NEVER return NULL.
	assert(s_appraiseInstance != NULL);
#if APPRAISE_DEBUG > 0
	if (!s_appraiseInstance)
		std::cerr << "AppRaiseMe: A NULL pointer about to be returned by getInstance. Segmentation fault is imminent." << std::endl;
#endif
	return s_appraiseInstance;
}

void AppRaiseMe::createInstance(bool enableReminder, RemindConditions *conditions) {
	if (!s_appraiseInstance) {
		// Reset error and hope for the best
		s_errCode = APPRAISE_NO_ERROR;

		// Try creating instance
		s_appraiseInstance = new AppRaiseMe();

		// If we failed, make sure an error is set before returning
		if (!s_appraiseInstance) {
#if APPRAISE_DEBUG > 0
			std::cerr << "AppRaiseMe: Failed to create instance of AppRaiseMe" << std::endl;
#endif
			s_errCode = APPRAISE_NOT_RUNNING;
		} else {
#if APPRAISE_DEBUG > 0
			std::cerr << "AppRaiseMe: Created instance of AppRaiseMe" << std::endl;
#endif
		}

		// Set reminder conditions and increment launch count
		if (s_errCode == APPRAISE_NO_ERROR) {
			s_appraiseInstance->setConditions(conditions);
			s_appraiseInstance->appLaunched(enableReminder);
		}
	}
}

void AppRaiseMe::destroyInstance() {
	if (s_appraiseInstance) {
		delete s_appraiseInstance;
		s_appraiseInstance = NULL;
		s_errCode = APPRAISE_NOT_RUNNING;
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Destroyed instance of AppRaiseMe" << std::endl;
#endif
	}
}

AppRaiseErr AppRaiseMe::getError()
{
	return s_errCode;
}

AppRaiseMe::AppRaiseMe()
	: m_bpsInitialized(false)
	, m_conditionsSet(false)
	, m_statPath()
	, m_rated(false)
	, m_postponed(false)
	, m_launchTime(std::time(NULL))
	, m_postponeTime(dPostponedTime)
	, m_launchCount(0)
	, m_sigEventCount(0)
{
	initStatsFile();
	readStats();
	if (s_errCode != APPRAISE_NO_ERROR) {
		return;
	}
	if(bps_initialize() != BPS_SUCCESS ) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Unable to initialize bps" << std::endl;
#endif
		s_errCode = APPRAISE_BPS_ERROR;
		return;
	} else {
		m_bpsInitialized = true;
	}

#ifndef _APPRAISE_ADVANCED_MODE_
	if (dialog_request_events(0) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Unable to register for dialog events" << std::endl;
#endif
		s_errCode = APPRAISE_BPS_ERROR;
		return;
	}
#endif
}

AppRaiseMe::~AppRaiseMe()
{
	if(m_bpsInitialized) {
		bps_shutdown();
	}
}

void AppRaiseMe::appLaunched(bool enableReminder)
{
	m_launchCount++;
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Incremented appLaunchCount" << std::endl;
#endif
#ifndef _APPRAISE_ADVANCED_MODE_
	if (enableReminder && showReminder()) {
		// If reminder was shown, we already recorded the stats
		return;
	}
#endif
	writeStats();
}

void AppRaiseMe::appSignificantEvent(bool enableReminder)
{
	m_sigEventCount++;
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Incremented sigEventCount" << std::endl;
#endif
#ifndef _APPRAISE_ADVANCED_MODE_
	if (enableReminder) {
		showReminder();
	}
#endif
	writeStats();
}

void AppRaiseMe::printStats() {
	std::cerr << "\tm_rated = " << m_rated << std::endl;
	std::cerr << "\tm_postponed = " << m_postponed << std::endl;
	std::cerr << "\tm_launchTime = " << m_launchTime << std::endl;
	std::cerr << "\tm_postponeTime = " << m_postponeTime << std::endl;
	std::cerr << "\tm_launchCount = " << m_launchCount << std::endl;
	std::cerr << "\tm_sigEventCount = " << m_sigEventCount << std::endl;
}

void AppRaiseMe::writeStats()
{
	if (s_errCode != APPRAISE_NO_ERROR) {
		return;
	}

	std::ofstream ofs;
	ofs.open(m_statPath.c_str());
	if (ofs.is_open()) {
		ofs << statVersion << " ";
		ofs << m_rated << " ";
		ofs << m_postponed << " ";
		ofs << m_launchTime << " ";
		ofs << m_postponeTime << " ";
		ofs << m_launchCount << " ";
		ofs << m_sigEventCount;
		ofs.close();

#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Wrote stats to file: " << std::endl;
		std::cerr << "\tversion = " << statVersion << std::endl;
		printStats();
#endif
	}

	// Did anything bad happen while reading
	if (ofs.eof() || ofs.bad() || ofs.fail()) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Unable to write stats - file could not be opened or is corrupt" << std::endl;
#endif
		s_errCode = APPRAISE_WRITE_ERROR;
	}
}

void AppRaiseMe::readStats()
{
	if (s_errCode != APPRAISE_NO_ERROR) {
		return;
	}

	std::ifstream ifs;
	ifs.open(m_statPath.c_str());
	if (ifs.is_open()) {
		// Read in version first
		unsigned int version = 0;
		ifs >> version;

		// Read in the rest of the stats accordingly
		switch (version) {
		case 1:
			ifs >> m_rated;
			ifs >> m_postponed;
			ifs >> m_launchTime;
			ifs >> m_postponeTime;
			ifs >> m_launchCount;
			ifs >> m_sigEventCount;

#if APPRAISE_DEBUG > 0
			std::cerr << "AppRaiseMe: Read stats from file: " << std::endl;
			std::cerr << "\tversion = " << version << std::endl;
			printStats();
#endif
		    break;
		default:
#if APPRAISE_DEBUG > 0
			std::cerr << "AppRaiseMe: Unrecognised version of stats file (" << version << ")" << std::endl;
#endif
			s_errCode = APPRAISE_READ_ERROR;
		    return;
		}

		ifs.close();
	}

	// Did anything bad happen while writing
	if (!ifs.eof() || ifs.bad() || ifs.fail()) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Unable to read stats - file could not be opened or is corrupt" << std::endl;
#endif
		s_errCode = APPRAISE_READ_ERROR;
	}
}

void AppRaiseMe::initStatsFile()
{
	char fileName[] = ".appraiseme";
	char* homeDir = getenv("HOME");
	std::stringstream ss;

	// Find the full path to the stats file
	if(!homeDir) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Failed to get value for HOME environment variable" << std::endl;
#endif
		s_errCode = APPRAISE_READ_ERROR;
		return;
	}
	ss << homeDir << "/" << fileName;
	m_statPath = ss.str();

#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Setting statPath to " << m_statPath << std::endl;
#endif

	// Check if the file exists by opening it for reading
	std::ifstream ifs;
	ifs.open(m_statPath.c_str(), std::ifstream::in);
	ifs.close();

	// If the file does not exist, create and initialize it
	if (ifs.fail()) {
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Stat file does not exist. Trying to create it." << std::endl;
#endif
		ifs.clear(std::ios::failbit);
		writeStats();
	}
}

bool AppRaiseMe::networkAvailable() {
	if (!m_bpsInitialized) {
		return false;
	}

	bool netAvailable;
	if (BPS_SUCCESS != netstatus_get_availability(&netAvailable)) {
		netAvailable = false;
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Could not get network status" << std::endl;
#endif
		s_errCode = APPRAISE_BPS_ERROR;
	}
	return netAvailable;
}

void AppRaiseMe::openAppWorld(unsigned int id)
{
	std::stringstream ss;
	if (id == 0) {
		ss << "appworld://myworld";
	} else {
		ss << "appworld://content/" << id;
	}

	char *err = NULL;
	if (navigator_invoke(ss.str().c_str(), &err) != BPS_SUCCESS) {
		if (err) {
#if APPRAISE_DEBUG > 0
			std::cerr << "AppRaiseMe: Error invoking AppWorld - " << err << std::endl;
#endif
			bps_free(err);
		}
		s_errCode = APPRAISE_BPS_ERROR;
		return;
	}
}

bool AppRaiseMe::isRated()
{
	return m_rated;
}

void AppRaiseMe::setRated(bool val)
{
	m_rated = val;
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Set rated flag to " << val << std::endl;
#endif
	writeStats();
}

bool AppRaiseMe::isPostponed()
{
	return m_postponed;
}

void AppRaiseMe::setPostponed(bool val)
{
	m_postponed = val;
	if (val) {
		m_postponeTime = std::time(NULL);
	} else {
		m_postponeTime = dPostponedTime;
	}
#if APPRAISE_DEBUG > 0
	std::cerr << "AppRaiseMe: Set postponed flag to " << val << " and postponeTime to " << m_postponeTime << std::endl;
#endif
	writeStats();
}

int AppRaiseMe::launchCount()
{
	return m_launchCount;
}

int AppRaiseMe::sigEventCount()
{
	return m_sigEventCount;
}

long long AppRaiseMe::firstLaunchTime()
{
	return m_launchTime;
}

long long  AppRaiseMe::postponedTime()
{
	if (m_postponed) {
		return m_postponeTime;
	} else {
		return dPostponedTime;
	}
}

void AppRaiseMe::setConditions(RemindConditions *conditions)
{
	if (conditions) {
		m_conditions = *conditions;
		m_conditionsSet = true;
	}
}

double daysSinceDate(long long date) {
	return (std::time(NULL) - date) / 86500.0;
}

bool AppRaiseMe::conditionsMet()
{
#if APPRAISE_DEBUG > 1
	// Always return true
	std::cerr << "AppRaiseMe: Assuming that all reminder conditions were met for purposes of debugging" << std::endl;
	return true;
#endif

	// Check if the reminder conditions have been set
	if (!m_conditionsSet) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: No reminder conditions have been set" << std::endl;
#endif
		return false;
	}

	// Check if app has already been rated
	if (m_rated) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: App has already been rated" << std::endl;
#endif
		return false;
	}

	// Check number of launches
	if (m_launchCount < m_conditions.launchCount) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Insufficient number of launches (" << m_launchCount << ")" << std::endl;
#endif
		return false;
	}

	// Check days since first launch
	if (daysSinceDate(m_launchTime) < m_conditions.daysInUse) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Insufficient time has passed (" << daysSinceDate(m_launchTime) << ")" << std::endl;
#endif
		return false;
	}

	// Check number of significant events
	if (m_sigEventCount < m_conditions.sigEventCount) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Insufficient number of significant events (" << m_sigEventCount << ")" << std::endl;
#endif
		return false;
	}

	// Check if reminder was postponed
	if (m_postponed) {
		if (daysSinceDate(m_postponeTime) < m_conditions.daysToPostpone) {
#if APPRAISE_DEBUG > 0
		std::cerr << "AppRaiseMe: Insufficient time has past since postponing (" << daysSinceDate(m_postponeTime) << ")" << std::endl;
#endif
			return false;
		}
	}

	// Check network status
	return networkAvailable();
}

#ifndef _APPRAISE_ADVANCED_MODE_
bool AppRaiseMe::showReminder()
{
	if (s_errCode != APPRAISE_NO_ERROR) {
		return false;
	}

	if (conditionsMet()) {
		showAlert();

		while (s_errCode == APPRAISE_NO_ERROR) {
			bps_event_t *event = NULL;
			bps_get_event(&event, -1);

			if (event && bps_event_get_domain(event) == dialog_get_domain()) {
				handleResponse(event);
				return true;
			}
		}
	}

	// IF we got here, no reminder was shown
	return false;
}

void AppRaiseMe::showAlert()
{
    if (dialog_create_alert(&s_alertDialog) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to create alert dialog." << std::endl;
#endif
    	goto alertErr2;
    }

    if (dialog_set_alert_message_text(s_alertDialog, APPRAISE_MESSAGE) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to set alert dialog message text." << std::endl;
#endif
    	goto alertErr1;
    }

    if (dialog_add_button(s_alertDialog, APPRAISE_RATE_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to add Rate button to alert dialog." << std::endl;
#endif
    	goto alertErr1;
    }

    if (dialog_add_button(s_alertDialog, APPRAISE_LATER_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to add Rate Later button to alert dialog." << std::endl;
#endif
    	goto alertErr1;
    }

    if (dialog_add_button(s_alertDialog, APPRAISE_CANCEL_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to add Cancel button to alert dialog." << std::endl;
#endif
    	goto alertErr1;
    }

    if (dialog_show(s_alertDialog) != BPS_SUCCESS) {
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Failed to show alert dialog." << std::endl;
#endif
    	goto alertErr1;
    }

    // Success!
    return;

alertErr1:
	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
alertErr2:
	s_errCode = APPRAISE_BPS_ERROR;
}

void AppRaiseMe::handleResponse(bps_event_t *event)
{
	int selectedIndex = dialog_event_get_selected_index(event);

	switch (selectedIndex) {
	case 0: // rate
		// Temporarily set app as rated. This is to deal with the case when
		// AppRaiseMe is started before anything has been drawn to the screen and thus
		// may be killed before the user returns from App World
		setRated(true);
		openAppWorld(APPRAISE_APPWORLD_ID);
		if (s_errCode != APPRAISE_NO_ERROR) {
			// If opening App World failed then postpone reminder
			setPostponed(true);
			setRated(false);
		}
		setPostponed(false);
		break;
	case 2: // cancel
		setRated(true);
		setPostponed(false);
		break;
	case 1: // rate later
		setPostponed(true);
	    break;
	default:
#if APPRAISE_DEBUG > 0
    	std::cerr << "AppRaiseMe: Unknown selection in dialog response handler" << std::endl;
#endif
	    break;
	}

	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
}
#endif
