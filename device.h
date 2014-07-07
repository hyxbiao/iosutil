
#ifndef	__DEVICE_H__
#define	__DEVICE_H__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

class Device
{
public:
	Device(struct am_device *device);
	~Device();

	int install(const char *app_path);
	int uninstall(const char *str_bundle_id);

	int listApps();
	int listDirectory(const char *dir_path);

protected:
	int startService(CFStringRef service_name, service_conn_t *handle);
	void stopService(service_conn_t handle);

	int startSession();
	int stopSession();

private:
	static mach_error_t transferCallback(CFDictionaryRef dict, int arg);
	static mach_error_t installCallback(CFDictionaryRef dict, int arg);

private:
	struct am_device *_device;
};

#endif	//__DEVICE_H__
