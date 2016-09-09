

#include "mock_conference_control.h"
#include "../bridgeserver/AppServer.h"
AppServer g_app;
int main(int argc, char ** argv) {
	CCS ccs;
	ccs.startConferenceControlServer();
	return 0;
}

