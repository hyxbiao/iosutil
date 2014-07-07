
#include <stdio.h>
#include "MobileDevice.h"
#include "common.h"
#include "device.h"

#include "manager.h"

Manager* Manager::_inst = 0;

Manager::Manager()
	:_device_id(NULL), _cmd(CMD_UNKNOWN), _active(false)
{
}

Manager * Manager::getInstance()
{
	if (_inst == NULL) {
		_inst = new Manager();
	}
	return _inst;
}

int Manager::parse(int argc, char *argv[])
{
	int i = 1;

	_cmd = CMD_UNKNOWN;
	while (_cmd == CMD_UNKNOWN && i < argc) {
		if (strcmp(argv[i], "-s") == 0 && i+1 < argc) {
			_device_id = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "devices") == 0) {
			_cmd = CMD_DEVICES;
		} else if (strcmp(argv[i], "install") == 0) {
			_cmd = CMD_INSTALL;
			i++;
		} else if (strcmp(argv[i], "uninstall") == 0) {
			_cmd = CMD_UNINSTALL;
			i++;
		} else if (strcmp(argv[i], "list") == 0) {
			_cmd = CMD_LISTAPP;
		} else if (strcmp(argv[i], "ls") == 0) {
			_cmd = CMD_LISTDIR;
			i++;
		} else {
			return -1;
		}
		i++;
	}
	if (argc <= 1 || i > argc) {
		return -1;
	}
	if (_device_id != NULL && _cmd == CMD_UNKNOWN) {
		return -1;
	}
	if (_device_id == NULL && _cmd != CMD_DEVICES) {
		return -1;
	}

	_argc = argc;
	_argv = argv;
	_argindex = i-1;

	return 0;
}

void Manager::run()
{
    AMDSetLogLevel(5); // otherwise syslog gets flooded with crap

    int timeout = 1;
	CFRunLoopTimerContext context;
	context.version = 0;
	context.info = this;
	context.retain = NULL;
	context.release = NULL;
	context.copyDescription = NULL;

	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + timeout, 0, 0, 0, timeoutCallback, &context);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
	//printf("[....] Waiting up to %d seconds for iOS device to be connected\n", timeout);

    struct am_device_notification *notify;
    AMDeviceNotificationSubscribe(&deviceCallback, 0, 0, this, &notify);
    CFRunLoopRun();
}

void Manager::handleDevice(struct am_device *amdevice) 
{
	CFStringRef device_id = AMDeviceCopyDeviceIdentifier(amdevice);
	const char *str_device_id = CFStringGetCStringPtr(device_id, CFStringGetSystemEncoding());

	if (_device_id != NULL) {
		if (strcmp(_device_id, str_device_id) == 0) {
			this->setActive(true);
		} else {
			CFRelease(device_id);
			return;
		}
	}

	//dispatch
	
	Device *device = new Device(amdevice);
	if (_cmd == CMD_DEVICES) {
		this->setActive(true);
		printf("[device] %s\n", str_device_id);
	} else if (_cmd == CMD_INSTALL) {
		const char *app_path = _argv[_argindex++];
		if (device->install(app_path) == 0) {
			printf("Install success!\n");
		} else {
			printf("Install fail!\n");
		}
	} else if (_cmd == CMD_UNINSTALL) {
		const char *bundle_id = _argv[_argindex++];
		if (device->uninstall(bundle_id) == 0) {
			printf("Uninstall success!\n");
		} else {
			printf("Uninstall fail!\n");
		}
	} else if (_cmd == CMD_LISTAPP) {
		if (device->listApps() != 0) {
			printf("List applications fail!\n");
		}
	} else if (_cmd == CMD_LISTDIR) {
		const char *path = _argv[_argindex++];
		device->listDirectory(path);
	}

	delete device;

	CFRelease(device_id);
}

void Manager::setActive(bool active)
{
	_active = active;
}

bool Manager::isActive()
{
	return _active;
}

void Manager::timeoutCallback(CFRunLoopTimerRef timer, void *arg) 
{
	Manager *manager = (Manager *)arg;

    if (!manager->isActive()) {
        fprintf(stderr, "Timed out waiting for device.\n");
        exit(-1);
    }
}

void Manager::deviceCallback(struct am_device_notification_callback_info *info, void *arg) 
{
	Manager *manager = (Manager *)arg;

    switch (info->msg) {
        case ADNCI_MSG_CONNECTED: {
            manager->handleDevice(info->dev);
			break;
		}
        case ADNCI_MSG_DISCONNECTED: {
			break;
		}
        default:
            break;
    }

	CFRunLoopStop(CFRunLoopGetCurrent());
}
