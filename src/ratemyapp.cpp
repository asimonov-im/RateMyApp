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

#include "ratemyapp.h"
#include "ratemyapp_priv.h"
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

#define RMA_STAT_OK (RMA_BPS_ERROR | RMA_WRITE_ERROR)

#define EXTERNAL_API extern "C"

static const unsigned int statVersion = 1;

RateMyApp *RateMyApp::s_rmaInstance = NULL;
dialog_instance_t RateMyApp::s_alertDialog = NULL;
RMAError RateMyApp::s_errCode = RMA_NOT_RUNNING;

// In case of problems, returns values that will avoid rating reminder
const bool dRated = true;
const bool dPostponed = true;
const int dLaunchCount = 0;
const int dSigEventCount = 0;
const long long dFirstLaunchTime = std::time(NULL);
const long long dPostponedTime = std::numeric_limits<long long>::max();
const bool dNetworkAvailable = false;

EXTERNAL_API enum RMAError rma_get_error()
{
	return RateMyApp::getError();
}

EXTERNAL_API void rma_stop()
{
	RateMyApp::destroyInstance();
}

#ifndef _RMA_ADVANCED_MODE_
EXTERNAL_API void rma_start(bool enableReminder)
{
	RateMyApp::createInstance(enableReminder);
}

EXTERNAL_API void rma_app_significant_event(bool enableReminder)
{
	RMAError err = RateMyApp::getError();
	bool die = (err == RMA_NOT_RUNNING) | (err == RMA_READ_ERROR);
	if (!die) {
		RateMyApp::getInstance()->appSignificantEvent(enableReminder);
	}
}

#else
EXTERNAL_API void rma_start()
{
	RateMyApp::createInstance(false);
}

EXTERNAL_API void rma_app_significant_event()
{
	int err = RateMyApp::getError() & RMA_STAT_OK;
	if (!err) {
		RateMyApp::getInstance()->appSignificantEvent(false);
	}
}

EXTERNAL_API bool rma_is_rated()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->isRated();
	} else {
		return dRated;
	}
}

EXTERNAL_API void rma_set_rated(bool val)
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		RateMyApp::getInstance()->setRated(val);
	}
}

EXTERNAL_API bool rma_is_postponed()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->isPostponed();
	} else {
		return dPostponed;
	}
}

EXTERNAL_API void rma_set_postponed(bool val)
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		RateMyApp::getInstance()->setPostponed(val);
	}
}

EXTERNAL_API int rma_launch_count()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->launchCount();
	} else {
		return dLaunchCount;
	}
}

EXTERNAL_API int rma_sig_event_count()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->sigEventCount();
	} else {
		return dSigEventCount;
	}
}

EXTERNAL_API long long rma_first_launch_time()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->firstLaunchTime();
	} else {
		return dFirstLaunchTime;
	}
}

EXTERNAL_API long long rma_postponed_time()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->postponedTime();
	} else {
		return dPostponedTime;
	}
}

EXTERNAL_API bool rma_network_available()
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->networkAvailable();
	} else {
		return dNetworkAvailable;
	}
}

EXTERNAL_API void rma_open_app_world(unsigned int id)
{
	if (RateMyApp::getError() == RMA_NO_ERROR) {
		return RateMyApp::getInstance()->openAppWorld(id);
	}
}
#endif

RateMyApp* RateMyApp::getInstance()
{
	// Calls to getInstance must ALWAYS be guarded with a check against RMA_NO_ERROR
	// to guarantee that we NEVER return NULL.
	assert(s_rmaInstance != NULL);
#if RMA_DEBUG > 0
	if (!s_rmaInstance)
		std::cerr << "RMA: A NULL pointer about to be returned by getInstance. Segmentation fault is imminent." << std::endl;
#endif
	return s_rmaInstance;
}

void RateMyApp::createInstance(bool enableReminder) {
	if (!s_rmaInstance) {
		// Reset error and hope for the best
		s_errCode = RMA_NO_ERROR;

		// Try creating instance
		s_rmaInstance = new RateMyApp();

		// If we failed, make sure an error is set before returning
		if (!s_rmaInstance) {
#if RMA_DEBUG > 0
			std::cerr << "RMA: Failed to create instance of RateMyApp" << std::endl;
#endif
			s_errCode = RMA_NOT_RUNNING;
		} else {
#if RMA_DEBUG > 0
			std::cerr << "RMA: Created instance of RateMyApp" << std::endl;
#endif
		}

		// Increment launch count.
		int err = s_errCode & RMA_STAT_OK;
		if (!err) {
			s_rmaInstance->appLaunched(enableReminder);
		}
	}
}

void RateMyApp::destroyInstance() {
	// Set error to indicate that RMA is no longer running
	if (s_rmaInstance) {
		delete s_rmaInstance;
		s_rmaInstance = NULL;
		s_errCode = RMA_NOT_RUNNING;
#if RMA_DEBUG > 0
		std::cerr << "RMA: Destroyed instance of RateMyApp" << std::endl;
#endif
	}
}

RMAError RateMyApp::getError()
{
	return s_errCode;
}

RateMyApp::RateMyApp()
	: m_rated(false)
	, m_postponed(false)
	, m_launchTime(dFirstLaunchTime)
	, m_postponeTime(dPostponedTime)
	, m_launchCount(dLaunchCount)
	, m_sigEventCount(dSigEventCount)
{
	initStatsFile();
	readStats();
	if (s_errCode != RMA_NO_ERROR) {
		goto end;
	}
	if(bps_initialize() != BPS_SUCCESS ) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to initialize bps" << std::endl;
#endif
		s_errCode = RMA_BPS_ERROR;
		goto end;
	}

#ifndef _RMA_ADVANCED_MODE_
	if (dialog_request_events(0) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to register for dialog events" << std::endl;
#endif
		s_errCode = RMA_BPS_ERROR;
		goto end;
	}
#endif

end:;
}

RateMyApp::~RateMyApp()
{
	if(s_errCode != RMA_BPS_ERROR) {
		bps_shutdown();
	}
}

void RateMyApp::appLaunched(bool enableReminder)
{
	m_launchCount++;
#if RMA_DEBUG > 0
	std::cerr << "RMA: Incremented appLaunchCount" << std::endl;
#endif
#ifndef _RMA_ADVANCED_MODE_
	if (enableReminder) {
		showReminder();
	}
#endif
	writeStats();
}

void RateMyApp::appSignificantEvent(bool enableReminder)
{
	m_sigEventCount++;
#if RMA_DEBUG > 0
	std::cerr << "RMA: Incremented sigEventCount" << std::endl;
#endif
#ifndef _RMA_ADVANCED_MODE_
	if (enableReminder) {
		showReminder();
	}
#endif
	writeStats();
}

void RateMyApp::printStats() {
	std::cerr << "\tm_rated = " << m_rated << std::endl;
	std::cerr << "\tm_postponed = " << m_postponed << std::endl;
	std::cerr << "\tm_launchTime = " << m_launchTime << std::endl;
	std::cerr << "\tm_postponeTime = " << m_postponeTime << std::endl;
	std::cerr << "\tm_launchCount = " << m_launchCount << std::endl;
	std::cerr << "\tm_sigEventCount = " << m_sigEventCount << std::endl;
}

void RateMyApp::writeStats()
{
	if (s_errCode != RMA_NO_ERROR) {
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

#if RMA_DEBUG > 0
		std::cerr << "RMA: Wrote stats to file: " << std::endl;
		std::cerr << "\tversion = " << statVersion << std::endl;
		printStats();
#endif
	}

	// Did anything bad happen while writing
	if (ofs.eof() || ofs.bad() || ofs.fail()) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to write stats - file could not be opened or is corrupt" << std::endl;
#endif
		s_errCode = RMA_WRITE_ERROR;
	}
}

void RateMyApp::readStats()
{
	bool die = (s_errCode == RMA_READ_ERROR) || (s_errCode == RMA_WRITE_ERROR);
	if (die) {
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

#if RMA_DEBUG > 0
			std::cerr << "RMA: Read stats in from file: " << std::endl;
			std::cerr << "\tversion = " << version << std::endl;
			printStats();
#endif
		    break;
		default:
#if RMA_DEBUG > 0
			std::cerr << "RMA: Unrecognised version of stats file (" << version << ")" << std::endl;
#endif
			s_errCode = RMA_READ_ERROR;
		    return;
		}

		ifs.close();
	}

	// Did anything bad happen while writing
	if (!ifs.eof() || ifs.bad() || ifs.fail()) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to read stats - file could not be opened or is corrupt" << std::endl;
#endif
		s_errCode = RMA_READ_ERROR;
	}
}

void RateMyApp::initStatsFile()
{
	char fileName[] = ".ratemyapp";
	char* homeDir = getenv("HOME");
	std::stringstream ss;

	// Find the full path to the stats file
	if(!homeDir) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Failed to find HOME environment variable" << std::endl;
#endif
		s_errCode = RMA_READ_ERROR;
		return;
	}
	ss << homeDir << "/" << fileName;
	m_statPath = ss.str();

#if RMA_DEBUG > 0
	std::cerr << "RMA: Setting statPath to " << m_statPath << std::endl;
#endif

	// Check if the file exists by opening it for reading
	std::ifstream ifs;
	ifs.open(m_statPath.c_str(), std::ifstream::in);
	ifs.close();

	// If the file does not exist, create and initialize it
	if (ifs.fail()) {
#if RMA_DEBUG > 0
	std::cerr << "RMA: Stat file does not exist. Trying to create it." << std::endl;
#endif
		ifs.clear(std::ios::failbit);
		writeStats();
	}
}

bool RateMyApp::networkAvailable() {
	if (s_errCode == RMA_BPS_ERROR) {
		return false;
	}

	bool netAvailable;
	if (BPS_SUCCESS != netstatus_get_availability(&netAvailable)) {
		netAvailable = false;
#if RMA_DEBUG > 0
	std::cerr << "RMA: Could not get network status" << std::endl;
#endif
		s_errCode = RMA_BPS_ERROR;
	}
	return netAvailable;
}

void RateMyApp::openAppWorld(unsigned int id)
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
#if RMA_DEBUG > 0
			std::cerr << "RMA: Error invoking AppWorld - " << err << std::endl;
#endif
			bps_free(err);
		}
		s_errCode = RMA_BPS_ERROR;
		return;
	}
}

bool RateMyApp::isRated()
{
	return m_rated;
}

void RateMyApp::setRated(bool val)
{
	m_rated = val;
#if RMA_DEBUG > 0
	std::cerr << "RMA: Set rated flag to" << val << std::endl;
#endif
	writeStats();
}

bool RateMyApp::isPostponed()
{
	return m_postponed;
}

void RateMyApp::setPostponed(bool val)
{
	m_postponed = val;
	if (val) {
		m_postponeTime = std::time(NULL);
	} else {
		m_postponeTime = dPostponedTime;
	}
#if RMA_DEBUG > 0
	std::cerr << "RMA: Set postponed flag to " << val << " and postponeTime to " << m_postponeTime << std::endl;
#endif
	writeStats();
}

int RateMyApp::launchCount()
{
	return m_launchCount;
}

int RateMyApp::sigEventCount()
{
	return m_sigEventCount;
}

long long RateMyApp::firstLaunchTime()
{
	return m_launchTime;
}

long long  RateMyApp::postponedTime()
{
	if (m_postponed) {
		return m_postponeTime;
	} else {
		return dPostponedTime;
	}
}

#ifndef _RMA_ADVANCED_MODE_
double daysSinceDate(long long date) {
	return (std::time(NULL) - date) / 86500.0;
}

void RateMyApp::showReminder()
{
	if (s_errCode != RMA_NO_ERROR) {
		return;
	}

	if (conditionsMet()) {
		showAlert();

		while (s_errCode == RMA_NO_ERROR) {
			bps_event_t *event = NULL;
			bps_get_event(&event, -1);

			if (event && bps_event_get_domain(event) == dialog_get_domain()) {
				handleResponse(event);
				break;
			}
		}
	}
}

bool RateMyApp::conditionsMet()
{
#if RMA_DEBUG > 1
	// Always return true
	std::cerr << "RMA: Assuming that all reminder conditions were met for purposes of debugging" << std::endl;
	return true;
#endif

	// Check if app has already been rated
	if (m_rated) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: App has already been rated" << std::endl;
#endif
		return false;
	}

	// Check number of launches
	if (m_launchCount < RMA_USES_UNTIL_PROMPT) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Insufficient number of launches (" << m_launchCount << ")" << std::endl;
#endif
		return false;
	}

	// Check days since first launch
	if (daysSinceDate(m_launchTime) < RMA_DAYS_UNTIL_PROMPT) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Insufficient time has passed (" << daysSinceDate(m_launchTime) << ")" << std::endl;
#endif
		return false;
	}

	// Check number of significant events
	if (m_sigEventCount < RMA_SIG_EVENTS_UNTIL_PROMPT) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Insufficient number of significant events (" << m_sigEventCount << ")" << std::endl;
#endif
		return false;
	}

	// Check if reminder was postponed
	if (m_postponed) {
		if (daysSinceDate(m_postponeTime) < RMA_TIME_BEFORE_REMINDING) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Insufficient time has past since postponing (" << daysSinceDate(m_postponeTime) << ")" << std::endl;
#endif
			return false;
		}
	}

	// Check network status
	return networkAvailable();
}

void RateMyApp::showAlert()
{
    if (dialog_create_alert(&s_alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to create alert dialog." << std::endl;
#endif
        s_errCode = RMA_BPS_ERROR;
        return;
    }

    if (dialog_set_alert_message_text(s_alertDialog, RMA_MESSAGE) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to set alert dialog message text." << std::endl;
#endif
    	goto alertErr;
    }

    if (dialog_add_button(s_alertDialog, RMA_RATE_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate button to alert dialog." << std::endl;
#endif
    	goto alertErr;
    }

    if (dialog_add_button(s_alertDialog, RMA_LATER_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate Later button to alert dialog." << std::endl;
#endif
    	goto alertErr;
    }

    if (dialog_add_button(s_alertDialog, RMA_CANCEL_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Cancel button to alert dialog." << std::endl;
#endif
    	goto alertErr;
    }

    if (dialog_show(s_alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to show alert dialog." << std::endl;
#endif
    	goto alertErr;
    }

    return;

alertErr:
	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
	s_errCode = RMA_BPS_ERROR;
}

void RateMyApp::handleResponse(bps_event_t *event)
{
	int selectedIndex = dialog_event_get_selected_index(event);

	switch (selectedIndex) {
	case 0: // rate
		// Temporarily set app as rated. This is to deal with the case when
		// RMA is started before anything has been drawn to the screen and thus
		// may be killed before the user returns from App World
		setRated(true);
		openAppWorld(RMA_APPWORLD_ID);
		if (s_errCode != RMA_NO_ERROR) {
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
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Unknown selection in dialog response handler" << std::endl;
#endif
	    break;
	}

	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
}
#endif
