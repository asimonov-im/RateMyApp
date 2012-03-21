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

#ifndef APPRAISEME_PRIV_H_
#define APPRAISEME_PRIV_H_

#include <string>
#include <bps/bps.h>
#include "bps/dialog.h"

class AppRaiseMe
{
public:
	static AppRaiseMe *getInstance();
	static void createInstance(bool enableReminder, RemindConditions *conditions);
	static void destroyInstance();
	static AppRaiseErr getError();

	~AppRaiseMe();
	void appLaunched(bool enableReminder);
	void appSignificantEvent(bool enableReminder);

	void setConditions(RemindConditions *conditions);
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
	AppRaiseMe();
	void initStatsFile();
	void printStats();
	void writeStats();
	void readStats();
	bool conditionsMet();

#ifndef _RMA_ADVANCED_MODE_
	bool showReminder();
	void showAlert();
	void handleResponse(bps_event_t *event);
#endif

	static AppRaiseMe* s_appraiseInstance;
	static AppRaiseErr s_errCode;
	static dialog_instance_t s_alertDialog;
	bool m_bpsInitialized;
	bool m_conditionsSet;
	std::string m_statPath;
	RemindConditions m_conditions;
	bool m_rated;
	bool m_postponed;
	long long m_launchTime;
	long long m_postponeTime;
	int m_launchCount;
	int m_sigEventCount;
};

#endif /* APPRAISEME_PRIV_H_ */