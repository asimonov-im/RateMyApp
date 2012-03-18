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
#include <time.h>
#include <iostream>
#include <sstream>
#include <fstream>

#define EXTERNAL_API extern "C"

RateMyApp *RateMyApp::s_rmaInstance = NULL;
dialog_instance_t RateMyApp::s_alertDialog = NULL;
RMAError RateMyApp::s_errCode = RMA_NOT_RUNNING;

EXTERNAL_API RMAError rma_get_error()
{
	return RateMyApp::getError();
}

EXTERNAL_API void rma_start()
{
	RateMyApp::createInstance();
}

EXTERNAL_API void rma_stop()
{
	RateMyApp::destroyInstance();
}

#ifndef _RMA_ADVANCED_MODE_
EXTERNAL_API void rma_app_launched(bool enableReminder)
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		rmaInstance->appLaunched(enableReminder);
	}
}

EXTERNAL_API void rma_app_significant_event(bool enableReminder)
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		rmaInstance->appSignificantEvent(enableReminder);
	}
}

#else
EXTERNAL_API void rma_app_launched()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		rmaInstance->appLaunched(false);
	}
}

EXTERNAL_API void rma_app_significant_event()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		rmaInstance->appSignificantEvent(false);
	}
}

EXTERNAL_API bool rma_is_rated()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->isRated();
	} else {
		// Return a safe value
		return true;
	}
}

EXTERNAL_API void rma_set_rated()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->setRated();
	}
}

EXTERNAL_API bool rma_is_postponed()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->isPostponed();
	} else {
		// Return a safe value
		return true;
	}
}

EXTERNAL_API void rma_set_postponed()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->setPostponed();
	}
}

EXTERNAL_API int rma_launch_count()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->launchCount();
	} else {
		// Return a safe value
		return 0;
	}
}

EXTERNAL_API int rma_sig_event_count()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->sigEventCount();
	} else {
		// Return a safe value
		return 0;
	}
}

EXTERNAL_API int rma_first_launch_time()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->firstLaunchTime();
	} else {
		// Return a safe value
		return MAX_INT;
	}
}

EXTERNAL_API int rma_postponed_time()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->postponedTime();
	} else {
		// Return a safe value
		return MAX_INT;
	}
}

EXTERNAL_API bool rma_network_available()
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->networkAvailable();
	} else {
		// Return a safe value
		return false;
	}
}

EXTERNAL_API void rma_open_app_world(const char *id)
{
	RateMyApp *rmaInstance = RateMyApp::getInstance();
	if (rmaInstance) {
		return rmaInstance->openAppWorld(id);
	}
}
#endif



RateMyApp::RateMyApp()
	: m_rated(false)
	, m_postponed(false)
	, m_launchTime(time(NULL))
	, m_postponeTime(0)
	, m_launchCount(0)
	, m_sigEventCount(0)
{
	initStatsFile();
	if(!readStats()) {
		goto end;
	}
	if(bps_initialize() != BPS_SUCCESS ) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to initialize bps" << std::endl;
#endif
		s_errCode = RMA_BPS_FAILURE;
		goto end;
	}

#ifndef _RMA_ADVANCED_MODE_
	if (dialog_request_events(0) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to register for dialog events" << std::endl;
#endif
		s_errCode = RMA_BPS_FAILURE;
		goto end;
	}
#endif

end:;
}

RateMyApp* RateMyApp::getInstance()
{
	return s_rmaInstance;
}

void RateMyApp::createInstance() {
	if (!s_rmaInstance) {
		s_rmaInstance = new RateMyApp();

		if (s_rmaInstance && s_errCode == RMA_NOT_RUNNING) {
			s_errCode = RMA_NO_ERROR;
		}
	}
}

void RateMyApp::destroyInstance() {
	if (s_rmaInstance) {
		delete s_rmaInstance;
		s_rmaInstance = NULL;
		s_errCode = RMA_NOT_RUNNING;
	}
}

RateMyApp::~RateMyApp()
{
	if(s_errCode != RMA_BPS_FAILURE) {
		bps_shutdown();
	}
}

void RateMyApp::appLaunched(bool enableReminder)
{
	m_launchCount++;
	if (enableReminder) {
		showReminder();
	}
	writeStats();
}

void RateMyApp::appSignificantEvent(bool enableReminder)
{
	m_sigEventCount++;
	if (enableReminder) {
		showReminder();
	}
	writeStats();
}

bool RateMyApp::writeStats()
{
	if (s_errCode != RMA_NO_ERROR) {
		return false;
	}

	std::ofstream ofs;
	ofs.open(m_statPath.c_str());
	if (ofs.is_open()) {
		ofs << m_rated << m_postponed << m_launchTime << m_postponeTime << m_launchCount << m_sigEventCount;
		ofs.close();
		return true;
	} else {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to write stats - could not open file for writing" << std::endl;
#endif
		return false;
	}
}

bool RateMyApp::readStats()
{
	if (s_errCode != RMA_NO_ERROR) {
		return false;
	}

	std::ifstream ifs;
	ifs.open(m_statPath.c_str());
	if (ifs.is_open()) {
		ifs >> m_rated >> m_postponed >> m_launchTime >> m_postponeTime >> m_launchCount >> m_sigEventCount;

#if RMA_DEBUG > 0
		std::cerr << "RMA: Read stats in from file: " << std::endl;
		std::cerr << "\tm_rated = " << m_rated << std::endl;
		std::cerr << "\tm_postponed = " << m_postponed << std::endl;
		std::cerr << "\tm_launchTime = " << m_launchTime << std::endl;
		std::cerr << "\tm_postponeTime = " << m_postponeTime << std::endl;
		std::cerr << "\tm_launchCount = " << m_launchCount << std::endl;
		std::cerr << "\tm_sigEventCount = " << m_sigEventCount << std::endl;
#endif

		ifs.close();
		return true;
	} else {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to read stats - could not open file for reading" << std::endl;
#endif
		s_errCode = RMA_FILE_ERROR;
		return false;
	}
}

void RateMyApp::initStatsFile()
{
	char fileName[] = ".ratemyapp";
	char* homeDir = getenv("HOME");
	std::stringstream ss;

	if(!homeDir) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Failed to find HOME environment variable" << std::endl;
#endif
		s_errCode = RMA_FILE_ERROR;
		return;
	}
	ss << homeDir << "/" << fileName;
	m_statPath = ss.str();

#if RMA_DEBUG > 0
	std::cerr << "RMA: Setting statPath to " << m_statPath << std::endl;
#endif

	std::ifstream ifs;
	ifs.open(m_statPath.c_str(), std::ifstream::in);
	ifs.close();

	if (ifs.fail()) {
#if RMA_DEBUG > 0
	std::cerr << "RMA: Stat file does not exist. Trying to create it." << std::endl;
#endif
		ifs.clear(std::ios::failbit);
		if(!writeStats()) {
			s_errCode = RMA_FILE_ERROR;
		}
	}
}

void RateMyApp::showReminder()
{
	if (s_errCode != RMA_NO_ERROR) {
		return;
	}

	if (conditionsMet() && showAlert()) {
		while (true) {
			bps_event_t *event = NULL;
			bps_get_event(&event, -1);

			if (event && bps_event_get_domain(event) == dialog_get_domain()) {
				handleResponse(event);
				writeStats();
				break;
			}
		}
	}
}

bool RateMyApp::networkAvailable() {
	if (s_errCode != RMA_BPS_FAILURE) {
		return false;
	}

	bool netAvailable;
	if (BPS_SUCCESS != netstatus_get_availability(&netAvailable)) {
		netAvailable = false;
		s_errCode = RMA_BPS_FAILURE;
	}
	return netAvailable;
}

bool RateMyApp::conditionsMet()
{
#if RMA_DEBUG > 1
	return true;
#endif

	// Check if app has already been rated
	if (m_rated) {
		return false;
	}

	// Check number of launches
	if (m_launchCount < RMA_USES_UNTIL_PROMPT) {
		return false;
	}

	// Check days since first launch
	double days = (time(NULL) - m_launchTime) / 86500.0;
	if (days < RMA_USES_UNTIL_PROMPT) {
		return false;
	}

	// Check number of significant events
	if (m_sigEventCount < RMA_SIG_EVENTS_UNTIL_PROMPT) {
		return false;
	}

	// Check if reminder was postponed
	if (m_postponed) {
		days = (time(NULL) - m_postponeTime) / 86500.0;
		if (days < RMA_TIME_BEFORE_REMINDING) {
			return false;
		}
	}

	// Check network status
	return networkAvailable();

}

bool RateMyApp::showAlert()
{
    if (dialog_create_alert(&s_alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to create alert dialog." << std::endl;
#endif
        return false;
    }

    if (dialog_set_alert_message_text(s_alertDialog, RMA_MESSAGE) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to set alert dialog message text." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(s_alertDialog, RMA_RATE_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(s_alertDialog, RMA_LATER_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate Later button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(s_alertDialog, RMA_CANCEL_BUTTON, true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Cancel button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_show(s_alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to show alert dialog." << std::endl;
#endif
    	goto err;
    }

    return true;

err:
	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
	return false;
}

void RateMyApp::openAppWorld(const char *id)
{
	if (s_errCode != RMA_NO_ERROR) {
		return;
	}

	std::stringstream ss;
	ss << "appworld://";
	if (id) {
		ss << "content/" << id;
	} else {
		ss << "myworld";
	}

	char *err = NULL;
	if (navigator_invoke(ss.str().c_str(), &err) != BPS_SUCCESS) {
		if (err) {
#if RMA_DEBUG > 0
			std::cerr << "RMA: " << err << std::endl;
#endif
			bps_free(err);
		}
		s_errCode = RMA_BPS_FAILURE;
		return;
	}
}

void RateMyApp::setRated()
{
	m_rated = true;
}

void RateMyApp::setPostponed()
{
	m_postponed = true;
	m_postponeTime = time(NULL);
}

void RateMyApp::handleResponse(bps_event_t *event)
{
	int selectedIndex = dialog_event_get_selected_index(event);

	switch (selectedIndex) {
	case 0: // rate
		openAppWorld(RMA_APPWORLD_ID);
		if (s_errCode != RMA_NO_ERROR) {
			break;
		}
	case 2: // cancel
		setRated();
		break;
	case 1: // rate later
		setPostponed();
	    break;
	default:
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Unknown selection in dialog." << std::endl;
#endif
	    break;
	}

	dialog_destroy(s_alertDialog);
	s_alertDialog = 0;
}
