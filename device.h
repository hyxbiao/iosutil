
#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#include "MobileDevice.h"

typedef struct {
    service_conn_t fd;
    CFSocketRef socket;
    CFRunLoopSourceRef source;
} DeviceConsoleConnection;

class Device
{
public:
	Device(struct am_device *device);
	~Device();

	int install(const char *app_path);
	int uninstall(const char *str_bundle_id);

	void showInfo();

	int listApps();
	int operateFile(int cmd, const char *target, const char *arg1, const char *arg2);

	int startLogcat();
	void stopLogcat();

	bool isAlive();
	void setAlive(bool alive);
protected:
	int startFileService(const char *target, service_conn_t *handle);

	int startService(CFStringRef service_name, service_conn_t *handle);
	void stopService(service_conn_t handle);

	int startSession();
	int stopSession();

private:
	static mach_error_t transferCallback(CFDictionaryRef dict, int arg);
	static mach_error_t installCallback(CFDictionaryRef dict, int arg);
	static void socketCallback(CFSocketRef s, CFSocketCallBackType type, CFDataRef address, const void *data, void *info);

private:
	struct am_device *_device;

	bool _alive;
	bool _logloop;
	DeviceConsoleConnection _loginfo;
};

#endif	//__DEVICE_H__
