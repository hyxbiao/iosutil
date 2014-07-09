
#ifndef	__MANAGER_H__
#define	__MANAGER_H__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

#include "device.h"

class Manager
{
public:
	enum {
		S_UNKNOWN		=	0x00,
		S_RUN_ONCE		=	0x01,
		S_RUN_LOOP		=	0x02
	};
public:
	static Manager * getInstance();

	int parse(int argc, char *argv[]);
	void run();
	void connectDevice(struct am_device *amdevice);
	void disConnectDevice(struct am_device *amdevice);

	bool isActive();
	bool isRunLoop();

	void release();
private:
	Manager();

	Device * getDevice(struct am_device *amdevice, bool bcreate = false);

	static void timeoutCallback(CFRunLoopTimerRef timer, void *arg);
	static void deviceCallback(struct am_device_notification_callback_info *info, void *arg);

private:
	static Manager *_inst;
	CFMutableDictionaryRef _devices;
	int _state;

private:
	char *_device_id;
	int _cmd;
	char *_arg1;
	char *_arg2;
	char *_option;
};

#endif	//__MANAGER_H__
