
#include "common.h"
#include "fileaccess.h"
#include "device.h"

Device::Device(struct am_device *device)
	:_device(device), _alive(false), _logloop(false)
{
}

Device::~Device()
{
}

int Device::install(const char *app_path)
{
    CFStringRef path = CFStringCreateWithCString(NULL, app_path, kCFStringEncodingASCII);

	//transfer app
	service_conn_t fd_afc;
	if (startService(AMSVC_AFC, &fd_afc) != 0) {
		CFRelease(path);
		return -1;
	}
	mach_error_t status = AMDeviceTransferApplication(fd_afc, path, NULL, transferCallback, NULL);

	stopService(fd_afc);

	if (status != MDERR_OK) {
		CFRelease(path);
		return -1;
	}

	//install app
	service_conn_t fd_install;
	if (startService(AMSVC_INSTALL_PROXY, &fd_install) != 0) {
		CFRelease(path);
		return -1;
	}

	CFStringRef keys[] = { CFSTR("PackageType") };
	CFStringRef values[] = { CFSTR("Developer") };
	CFDictionaryRef options = CFDictionaryCreate(NULL, (const void **)&keys, (const void **)&values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	status = AMDeviceInstallApplication(fd_install, path, options, installCallback, NULL);

	stopService(fd_install);
	CFRelease(options);
	CFRelease(path);

	if (status != MDERR_OK)
	{
		if (status == 0xe8008015) {
			printf("Your application failed code-signing checks. Check your certificates, provisioning profiles, and bundle ids.");
		}
		printf("AMDeviceInstallApplication failed: 0x%X\n", status);
		return -1;
	}

	return 0;
}

mach_error_t Device::transferCallback(CFDictionaryRef dict, int arg) 
{
	int percent;
	CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
	CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

	//CFShow(dict);
	if (CFEqual(status, CFSTR("CopyingFile"))) {
		CFStringRef path = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Path"));

		printf("[Transfer][%3d%%] Copying %s to device\n", percent, CFStringGetCStringPtr(path, kCFStringEncodingMacRoman));
	}

	return 0;
}

mach_error_t Device::installCallback(CFDictionaryRef dict, int arg)
{
	int percent;
	CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
	CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

	//CFShow(dict);
	printf("[Install][%3d%%] arg:%d, Status: %s\n", percent, arg, CFStringGetCStringPtr(status, kCFStringEncodingMacRoman));
	return 0;
}

int Device::uninstall(const char *str_bundle_id)
{
    CFStringRef bundle_id = CFStringCreateWithCString(NULL, str_bundle_id, kCFStringEncodingASCII);

	//uninstall app
	if (startSession() != 0) {
		CFRelease(bundle_id);
		return -1;
	}
	mach_error_t status = AMDeviceSecureUninstallApplication(0, _device, bundle_id, 0, NULL, 0);

	stopSession();

	/* or try this
	service_conn_t fd_uninstall;
	if (startService(AMSVC_INSTALL_PROXY, &fd_uninstall) != 0) {
		CFRelease(bundle_id);
		return -1;
	}
	mach_error_t status = AMDeviceUninstallApplication (fd_uninstall, bundle_id, NULL, installCallback, NULL);

	stopService(fd_uninstall);
	*/

	CFRelease(bundle_id);

	if (status != MDERR_OK)
	{
		return -1;
	}

	return 0;
}

int Device::listApps()
{
	//copy from MobileDeviceAccess
	NSArray *a = [NSArray arrayWithObjects:
		@"CFBundleIdentifier",			// absolute must
		@"ApplicationDSID",
		@"ApplicationType",
		@"CFBundleExecutable",
		@"CFBundleDisplayName",
		@"CFBundleIconFile",
		@"CFBundleName",
		@"CFBundleShortVersionString",
		//@"CFBundleSupportedPlatforms",
		//@"CFBundleURLTypes",
		@"CFBundleVersion",
		//@"CodeInfoIdentifier",
		//@"Container",
		//@"Entitlements",
		//@"HasSettingsBundle",
		//@"IsUpgradeable",
		@"MinimumOSVersion",
		//@"Path",
		//@"SignerIdentity",
		@"UIDeviceFamily",
		@"UIFileSharingEnabled",
		//@"UIStatusBarHidden",
		//@"UISupportedInterfaceOrientations",

		// iPhone Configuration Utility appears to ask for these:
		//@"CFBundlePackageType",
		//@"BuildMachineOSBuild",
		//@"CFBundleResourceSpecification",
		//@"DTPlatformBuild",
		//@"DTCompiler",
		//@"CFBundleSignature",
		//@"DTSDKName",
		//@"NSBundleResolvedPath",
		//@"UISupportedInterfaceOrientations",
		//@"DTXcode",
		//@"CFBundleInfoDictionaryVersion",
		//@"CFBundleSupportedPlatforms",
		//@"DTXcodeBuild",
		//@"UIStatusBarTintParameters",
		//@"DTPlatformVersion",
		//@"DTPlatformName",
		//@"CFBundleDevelopmentRegion",
		//@"DTSDKBuild",

		nil];

	NSDictionary *optionsDict = [NSDictionary dictionaryWithObject:a forKey:@"ReturnAttributes"];
	CFDictionaryRef options = (CFDictionaryRef)optionsDict;

	if (startSession() != 0) {
		return -1;
	}
	CFDictionaryRef apps;
	mach_error_t status = AMDeviceLookupApplications(_device, options, &apps);
	stopSession();
	if (status != MDERR_OK) {
		return -1;
	}

	char str_app_name[256];

	CFIndex count = CFDictionaryGetCount(apps);
	printf("Total %ld applications:\n", count);
	for (NSString *nskey in (NSDictionary*)apps) {
		CFStringRef key = (CFStringRef)nskey;
		CFDictionaryRef app = (CFDictionaryRef)CFDictionaryGetValue(apps, key);

		CFStringRef app_type = (CFStringRef)CFDictionaryGetValue(app, CFSTR("ApplicationType"));
		if (CFEqual(app_type, CFSTR("User"))) {
			CFStringRef app_name = (CFStringRef)CFDictionaryGetValue(app, CFSTR("CFBundleDisplayName"));

			str_app_name[0] = '\0';
			if (app_name != NULL) {
				CFStringGetCString(app_name, str_app_name, sizeof(str_app_name), kCFStringEncodingUTF8 );
			}
			CFStringRef app_version = (CFStringRef)CFDictionaryGetValue(app, CFSTR("CFBundleVersion"));
			const char * str_app_version = (const char *)CFStringGetCStringPtr(app_version, CFStringGetSystemEncoding());
			const char * str_key = (const char *)CFStringGetCStringPtr(key, CFStringGetSystemEncoding());
			//printf("%-15s(%s)\t%s\n", str_app_name, str_app_version, str_key);
			printf("%-40s\t%s(%s)\n", str_key, str_app_name, str_app_version);
		}
	}

	CFRelease(apps);
	return 0;
}

int Device::operateFile(int cmd, const char *str_target, const char *arg1, const char *arg2)
{
	int status = 0;
	int ret = 0;
	service_conn_t fd_afc;
	if (startFileService(str_target, &fd_afc) != 0) {
		return -1;
	}

	FileAccess *fa = new FileAccess(fd_afc);
	if (fa->connect() != 0) {
		status = -1;
		goto OPFILE_END;
	}

	switch (cmd) {
	case CMD_LISTDIR: {
		ret = fa->listDirectory(arg1);
		break;
	}
	case CMD_PUSH: {
		ret = fa->push(arg1, arg2);
		break;
	}
	case CMD_PULL: {
		ret = fa->pull(arg1, arg2);
		break;
	}
	default: {}
		break;
	}
	if (ret != 0) {
		status = -1;
		goto OPFILE_END;
	}

OPFILE_END:
	delete fa;
	stopService(fd_afc);

	return status;

} 

int Device::startLogcat()
{
	service_conn_t fd_log;
	if (startService(AMSVC_SYSLOG_RELAY, &fd_log) != 0) {
		return -1;
	}

	CFSocketContext context = {0, this, NULL, NULL, NULL};
	CFSocketRef socket = CFSocketCreateWithNative(kCFAllocatorDefault,
												  fd_log,
												  kCFSocketDataCallBack,
												  socketCallback,
												  &context);
	if (socket) {
		CFRunLoopSourceRef source = CFSocketCreateRunLoopSource(kCFAllocatorDefault, socket, 0);
		if (source) {
			CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes);
			_loginfo.fd = fd_log;
			_loginfo.socket = socket;
			_loginfo.source = source;
			_logloop = true;
			CFRelease(source);
		}
		CFRelease(socket);
	}
	//stopService(fd_log);
	return 0;
}

void Device::stopLogcat()
{
	if (_logloop) {
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), _loginfo.source, kCFRunLoopCommonModes);
		CFRelease(_loginfo.source);
		CFRelease(_loginfo.socket);
		stopService(_loginfo.fd);
	}
}

void Device::socketCallback(CFSocketRef s, CFSocketCallBackType type, CFDataRef address, const void *data, void *info)
{
	Device *device = (Device *)info;
	// Skip null bytes
	ssize_t length = (ssize_t)CFDataGetLength((CFDataRef)data);
	const char *buffer = (const char *)CFDataGetBytePtr((CFDataRef)data);
	while (length) {
		while (*buffer == '\0') {
			buffer++;
			length--;
			if (length == 0) {
				return;
			}
		}
		size_t extentLength = 0;
		while ((buffer[extentLength] != '\0') && extentLength != length) {
			extentLength++;
		}
		printf("%s", buffer);
		length -= extentLength;
		buffer += extentLength;
	}
}

bool Device::isAlive()
{
	return _alive;
}

void Device::setAlive(bool alive)
{
	_alive = alive;
}

int Device::startFileService(const char *str_target, service_conn_t *handle)
{
	int ret = 0;
	if (str_target == NULL) {
		ret = startService(AMSVC_AFC, handle);
	} else if (strcmp(str_target, "crash") == 0) {
		ret = startService(AMSVC_CRASH_REPORT_COPY_MOBILE, handle);
	} else {
		CFStringRef target = CFStringCreateWithCString(NULL, str_target, kCFStringEncodingASCII);
		if (startSession() == 0) {
			ret = AMDeviceStartHouseArrestService(_device, target, NULL, handle, 0);
			stopSession();
		}
		CFRelease(target);
	}
	if (ret != 0) {
		return -1;
	}
	return 0;
}

int Device::startService(CFStringRef service_name, service_conn_t *handle)
{
	int ret = -1;
	if (startSession() == 0) {
		if (AMDeviceStartService(_device, service_name, handle, NULL) == MDERR_OK) {
			ret = 0;
		}
		stopSession();
	}
	return ret;
}

void Device::stopService(service_conn_t handle)
{
	if (handle) {
		close(handle);
	}
}

int Device::startSession()
{
	if (AMDeviceConnect(_device) == MDERR_OK) {
		if (AMDeviceIsPaired(_device) && AMDeviceValidatePairing(_device) == MDERR_OK) {
			if (AMDeviceStartSession(_device) == MDERR_OK) {
				return 0;
			}
		}
		AMDeviceDisconnect(_device);
	}
	return -1;
}

int Device::stopSession()
{
	AMDeviceStopSession(_device);
	AMDeviceDisconnect(_device);

	return 0;
}


