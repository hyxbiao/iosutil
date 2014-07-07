#include <stdio.h>
#include "manager.h"

/*
bool found_device = false;

void download_file(struct afc_connection *afc, const char *from, const char *to)
{
	afc_file_ref rfile;
	assert(AFCFileRefOpen(afc, from, 2, &rfile) == MDERR_OK);

	FILE *fp = fopen(to, "wb");

	assert(fp != NULL);

	unsigned int bufsize = 512;
	char buf[bufsize];
	while(true) {
		unsigned int readed = bufsize;
		if (AFCFileRefRead(afc, rfile, buf, &readed) != MDERR_OK || readed == 0) {
			break;
		}
		fwrite(buf, readed, 1, fp);
	}

	fclose(fp);
	AFCFileRefClose(afc, rfile);
}

void handle_device_old(struct am_device *device) {
    if (found_device) return; // handle one device only
	found_device = true;

	CFStringRef deviceId = AMDeviceCopyDeviceIdentifier(device);
	CFStringRef str = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("deviceconsole connected: %@"), deviceId);
	CFRelease(deviceId);
	CFShow(str);
	CFRelease(str);

	assert(AMDeviceConnect(device) == MDERR_OK);
	assert(AMDeviceIsPaired(device));
	assert(AMDeviceValidatePairing(device) == MDERR_OK);
	assert(AMDeviceStartSession(device) == MDERR_OK);

	//do something
	struct afc_connection *afc = NULL;
	service_conn_t service;
	assert(AMDeviceStartService(device, CFSTR("com.apple.crashreportcopymobile"), &service, NULL) == MDERR_OK);
	assert(AFCConnectionOpen(service, 0, &afc) == MDERR_OK);

	struct afc_directory *dir;
	assert(AFCDirectoryOpen(afc, ".", &dir) == MDERR_OK);
	while(true) {
		char *d = NULL;
		AFCDirectoryRead(afc, dir, &d);
		if (d == NULL) {
			break;
		}
		printf("file: %s\n", d);
	}
	AFCDirectoryClose(afc, dir);

	download_file(afc, "SpriteDemo_2014-06-25-164023_hyxbiao-iphone4s.ips", "test.ips");

	AMDeviceStopSession(device);
	AMDeviceDisconnect(device);
}
*/

void usage() {
    printf(
        "Usage: iosutil [OPTION]...\n"
        "  -s <device id>                       specific device\n"
        "  devices                              list all connected devices\n"
		"\n"
        "device commands:\n"
        "  install <.app/.ipa path>             install app\n"
		"  uninstall <bundle id>                uninstall app\n"
        "  list                                 list all installed apps\n"
	);
	exit(0);
}

int main(int argc, char *argv[])
{
	Manager *manager = Manager::getInstance();

	if (manager->parse(argc, argv) < 0) {
		usage();
	}

	manager->run();

	return 0;
}

