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

#ifndef RATEMYAPP_PRIV_H_
#define RATEMYAPP_PRIV_H_

#include <string>
#include <bps/bps.h>
#include "bps/dialog.h"

class RateMyApp
{
public:
	static RateMyApp *getInstance();
	static void createInstance(bool enableReminder);
	static void destroyInstance();
	static RMAError getError();

	~RateMyApp();
	void appLaunched(bool enableReminder);
	void appSignificantEvent(bool enableReminder);

	bool isRated();
	void setRated(bool val);
	bool isPostponed();
	void setPostponed(bool val);
	int launchCount();
	int sigEventCount();
	long long firstLaunchTime();
	long long postponedTime();
	bool networkAvailable();
	void openAppWorld(unsigned int id);

private:
	RateMyApp();
	void initStatsFile();
	void printStats();
	void writeStats();
	void readStats();

#ifndef _RMA_ADVANCED_MODE_
	void showReminder();
	bool conditionsMet();
	void showAlert();
	void handleResponse(bps_event_t *event);
#endif

	static RateMyApp* s_rmaInstance;
	static RMAError s_errCode;
	static dialog_instance_t s_alertDialog;
	std::string m_statPath;
	bool m_rated;
	bool m_postponed;
	long long m_launchTime;
	long long m_postponeTime;
	int m_launchCount;
	int m_sigEventCount;
};

#endif /* RATEMYAPP_PRIV_H_ */
