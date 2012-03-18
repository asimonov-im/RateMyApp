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

enum RMAError {
	RMA_NO_ERROR = 0,
	RMA_BPS_FAILURE = 1,
	RMA_FILE_ERROR = 2
};

class RateMyApp
{
public:
	RateMyApp();
	~RateMyApp();
	void appLaunched(bool show);
	void appSignificantEvent(bool show);

private:
	void initStatsFile();
	bool writeStats();
	bool readStats();
	void showReminder();
	bool conditionsMet();
	bool showAlert();
	void handleResponse(bps_event_t *event);

	static RateMyApp * s_rmaInstance;
	std::string m_statPath;
	std::string m_appWorldURI;
	std::string m_message;
	std::string m_cancelButton;
	std::string m_rateButton;
	std::string m_rateLater;

	RMAError m_errCode;
	bool m_rated;
	bool m_postponed;
	int m_launchTime;
	int m_postponeTime;
	int m_launchCount;
	int m_sigEventCount;
};

#endif /* RATEMYAPP_PRIV_H_ */
