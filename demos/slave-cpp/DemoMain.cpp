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

#include "SlaveDemo.h"

#include <signal.h>

#include <APL/Log.h>
#include <APL/Lock.h>

using namespace std;
using namespace apl;
using namespace apl::dnp;

SigLock gLock;
SlaveDemoBase* gpDemo = NULL;
void SetDemo(SlaveDemoBase* apDemo)
{
	CriticalSection cs(&gLock);
	gpDemo = apDemo;
}

void Terminate(int sig)
{
	CriticalSection cs(&gLock);
	std::cout << "Signal " << sig << ", shutdown... " << std::endl;
	if(gpDemo) gpDemo->Shutdown();
}

/*
 * command line syntax:
 *
 *    ./demo-slave-cpp [tcp-server-port [local-dnp3-addr [remote-dnp3-addr]]]
 *
 * Examples:
 *
 *    ./demo-slave-cpp
 *    ./demo-slave-cpp 5000
 *    ./demo-slave-cpp 5001 1234
 *    ./demo-slave-cpp 5002 1234 5678
 */
int main(int argc, char* argv[])
{
	unsigned port = 4999;
	unsigned local_addr = 1;
	unsigned remote_addr = 100;

	if (argc >= 2) {
		std::istringstream iss(argv[1]);
		iss >> port;
	}

	if (argc >= 3) {
		std::istringstream iss(argv[2]);
		iss >> local_addr;
	}

	if (argc >= 4) {
		std::istringstream iss(argv[3]);
		iss >> remote_addr;
	}

	// Create a log object for the stack to use and configure it with a subscriber
	// that print alls messages to the stdout
	EventLog log;
	log.AddLogSubscriber(LogToStdio::Inst());

	// Specify a FilterLevel for the stack/physical layer to use. Log statements with a lower priority
	// will not be logged.
	const FilterLevel LOG_LEVEL = LEV_INFO;

	// create our demo application that handles commands and demonstrates how to publish data
	// give it a loffer with a unique name and log level
	SlaveDemoApp app(log.GetLogger(LOG_LEVEL, "demoapp"));

	// This is the main point of interaction with the stack. The AsyncStackManager object instantiates
	// master/slave DNP stacks, as well as their physical layers
	AsyncStackManager mgr(log.GetLogger(LOG_LEVEL, "dnp"));

	// add a TCPServer to the manager with the name "tcpserver"
	// The server will wait 3000 ms in between failed bind calls
	// The server will *only* respond to connections on the loopback
	mgr.AddTCPServer("tcpserver", PhysLayerSettings(LOG_LEVEL, 3000), "0.0.0.0", port);

	// The master config object for a slave. The default are useable, but understaning the options are important
	SlaveStackConfig stackConfig;

	//override the default link addressing
	stackConfig.link.LocalAddr = local_addr;
	stackConfig.link.RemoteAddr = remote_addr;

	// The DeviceTemplate struct specifies the structure of the slave's database, as well as the index range of controls
	// and setpoints it accepts.
	DeviceTemplate device(5, 5, 5, 5, 5, 5, 5);
	stackConfig.device = device;

	// Create a new slave on a previously declared port, with a name, log level, command acceptor, and config info
	// This returns a thread-safe interface used for updating the slave's database.
	IDataObserver* pDataObserver = mgr.AddSlave("tcpserver", "slave", LOG_LEVEL, app.GetCmdAcceptor(), stackConfig);

	// Tell the app where to write opdates
	app.SetDataObserver(pDataObserver);

	//configure signal handlers so we can exit gracefully
	SetDemo(&app);
	signal(SIGTERM, &Terminate);
	signal(SIGABRT, &Terminate);
	signal(SIGINT, &Terminate);

	app.Run();

	SetDemo(NULL);

	return 0;
}
