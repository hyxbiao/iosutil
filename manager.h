
#ifndef	__MANAGER_H__
#define	__MANAGER_H__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

class Manager
{
public:
	static Manager * getInstance();

	int parse(int argc, char *argv[]);
	void run();
	void handleDevice(struct am_device *device);

	void setActive(bool active);
	bool isActive();
private:
	Manager();

	static void timeoutCallback(CFRunLoopTimerRef timer, void *arg);
	static void deviceCallback(struct am_device_notification_callback_info *info, void *arg);

private:
	static Manager *_inst;

private:
	int _argc;
	char **_argv;
	int _argindex;

private:
	char *_device_id;
	int _cmd;
	bool _active;
};

#endif	//__MANAGER_H__
