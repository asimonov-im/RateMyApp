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

static RateMyApp *rmaInstance = NULL;
static dialog_instance_t alertDialog = NULL;

EXTERNAL_API void rma_start()
{
	if (!rmaInstance) {
		rmaInstance = new RateMyApp();
	}
}

EXTERNAL_API void rma_stop()
{
	if (rmaInstance) {
		delete rmaInstance;
		rmaInstance = NULL;
	}
}

EXTERNAL_API void rma_app_launched(bool suppress)
{
	if (rmaInstance) {
		rmaInstance->appLaunched(suppress);
	}
}

EXTERNAL_API void rma_app_significant_event(bool suppress)
{
	if (rmaInstance) {
		rmaInstance->appSignificantEvent(suppress);
	}
}

RateMyApp::RateMyApp()
	: m_appWorldURI(RMA_APPWORLD_ID)
 	, m_message(RMA_MESSAGE)
	, m_cancelButton(RMA_CANCEL_BUTTON)
	, m_rateButton(RMA_RATE_BUTTON)
	, m_rateLater(RMA_RATE_LATER)
	, m_errCode(RMA_NO_ERROR)
	, m_rated(false)
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
		m_errCode = RMA_BPS_FAILURE;
		goto end;
	}
	if (dialog_request_events(0) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
		std::cerr << "RMA: Unable to register for dialog events" << std::endl;
#endif
		m_errCode = RMA_BPS_FAILURE;
		goto end;
	}
	showReminder();
end:;
}

RateMyApp::~RateMyApp()
{
	if(m_errCode != RMA_BPS_FAILURE) {
		bps_shutdown();
	}
}

void RateMyApp::appLaunched(bool show)
{
	m_launchCount++;
	if (show) {
		showReminder();
		writeStats();
	}
}

void RateMyApp::appSignificantEvent(bool show)
{
	m_sigEventCount++;
	if (show) {
		showReminder();
		writeStats();
	}
}

bool RateMyApp::writeStats()
{
	if (m_errCode != RMA_NO_ERROR) {
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
	if (m_errCode != RMA_NO_ERROR) {
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
		m_errCode = RMA_FILE_ERROR;
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
		m_errCode = RMA_FILE_ERROR;
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
			m_errCode = RMA_FILE_ERROR;
		}
	}
}

void RateMyApp::showReminder()
{
	if (m_errCode != RMA_NO_ERROR) {
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
	bool netAvailable;
	if (BPS_SUCCESS != netstatus_get_availability(&netAvailable)) {
		netAvailable = false;
	}
	return netAvailable;
}

bool RateMyApp::showAlert()
{
    if (dialog_create_alert(&alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to create alert dialog." << std::endl;
#endif
        return false;
    }

    if (dialog_set_alert_message_text(alertDialog, m_message.c_str()) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to set alert dialog message text." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(alertDialog, m_rateButton.c_str(), true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(alertDialog, m_rateLater.c_str(), true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Rate Later button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_add_button(alertDialog, m_cancelButton.c_str(), true, 0, true) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to add Cancel button to alert dialog." << std::endl;
#endif
    	goto err;
    }

    if (dialog_show(alertDialog) != BPS_SUCCESS) {
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Failed to show alert dialog." << std::endl;
#endif
    	goto err;
    }

    return true;

err:
	dialog_destroy(alertDialog);
	alertDialog = 0;
	return false;
}

void RateMyApp::handleResponse(bps_event_t *event)
{
	int selectedIndex = dialog_event_get_selected_index(event);
	char *err = NULL;

	switch (selectedIndex) {
	case 0: // rate
		if (navigator_invoke(m_appWorldURI.c_str(), &err) != BPS_SUCCESS) {
			if (err) {
#if RMA_DEBUG > 0
				std::cerr << "RMA: " << err << std::endl;
#endif
				bps_free(err);
			}
			break;
		}
	case 2: // cancel
		m_rated = true;
		break;
	case 1: // rate later
		m_postponed = true;
		m_postponeTime = time(NULL);
	    break;
	default:
#if RMA_DEBUG > 0
    	std::cerr << "RMA: Unknown selection in dialog." << std::endl;
#endif
	    break;
	}

	dialog_destroy(alertDialog);
	alertDialog = 0;
}
