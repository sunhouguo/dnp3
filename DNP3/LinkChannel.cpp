//
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Enery Corp licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
#include "LinkChannel.h"

#include "Stack.h"
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <APL/Logger.h>
#include <APL/IPhysicalLayerAsync.h>
#include <APL/AsyncTaskGroup.h>

namespace apl
{
namespace dnp
{

LinkChannel::LinkChannel(Logger* apLogger, const std::string& arName, ITimerSource* apTimerSrc, IPhysicalLayerAsync* apPhys, AsyncTaskGroup* apTaskGroup, millis_t aOpenRetry) :
	Loggable(apLogger),
	LinkLayerRouter(apLogger, apPhys, apTimerSrc, aOpenRetry),
	mName(arName),		
	mState(PLS_CLOSED),
	mpTaskGroup(apTaskGroup)
{
	
}

	void LinkChannel::OnStateChange(PhysLayerState aState)
{
	CriticalSection cs(&mLock);
	mState = aState;
	cs.Signal();
}

void LinkChannel::WaitForStop()
{
	CriticalSection cs(&mLock);
	while(mState != PLS_STOPPED) cs.Wait();
}

void LinkChannel::BindStackToChannel(const std::string& arStackName, Stack* apStack, const LinkRoute& arRoute)
{
	LOG_BLOCK(LEV_DEBUG, "Linking stack to port w/ route " << arRoute);
	this->AddContext(&apStack->mLink, arRoute); // this function can throw, do it first
	apStack->mLink.SetRouter(this);	
	mStackMap[arStackName] = StackRecord(apStack, arRoute);			
	this->Start();	
}

void LinkChannel::RemoveStackFromChannel(const std::string& arStackName)
{
	StackMap::iterator i = mStackMap.find(arStackName);
	if(i == mStackMap.end()) throw ArgumentException("Stack with that name not bound: " + arStackName);
	else {
		StackRecord r = i->second;
		LOG_BLOCK(LEV_DEBUG, "Unlinking stack from port w/ route " << r.route);
		this->RemoveContext(r.route);
		mStackMap.erase(i);
		if(this->NumContext() == 0) {
			LOG_BLOCK(LEV_DEBUG, "Stopping router");
			this->Stop();
		}
	}
}

}
}
