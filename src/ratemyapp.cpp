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

#define EXTERNAL_API extern "C"

EXTERNAL_API void rma_start()
{
	RateMyApp *rma = RateMyApp::getInstance();
	rma->start();
}

EXTERNAL_API void rma_app_launched(bool suppress)
{
	RateMyApp *rma = RateMyApp::getInstance();
	rma->appLaunched(suppress);
}

EXTERNAL_API void rma_app_entered_foreground(bool suppress)
{
	RateMyApp *rma = RateMyApp::getInstance();
	rma->appEnteredForeground(suppress);
}

EXTERNAL_API void rma_app_significant_event(bool suppress)
{
	RateMyApp *rma = RateMyApp::getInstance();
	rma->appSignificantEvent(suppress);
}

RateMyApp *RateMyApp::instance = 0:

RateMyApp::RateMyApp()
{
	m_name = RMA_APP_NAME;
}
