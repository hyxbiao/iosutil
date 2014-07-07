
#include "MobileDevice.h"
#include "fileaccess.h"
#include "device.h"

Device::Device(struct am_device *device)
	:_device(device)
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
		char* error = "Unknown error.";
		if (status == 0xe8008015)
			error = "Your application failed code-signing checks. Check your certificates, provisioning profiles, and bundle ids.";
		printf("AMDeviceInstallApplication failed: 0x%X: %s\n", status, error);
		return -1;
	}

	return 0;
}

mach_error_t Device::transferCallback(CFDictionaryRef dict, int arg) 
{
	int percent;
	CFStringRef status = (CFStringRef)CFDictionaryGetValue(dict, CFSTR("Status"));
	CFNumberGetValue((CFNumberRef)CFDictionaryGetValue(dict, CFSTR("PercentComplete")), kCFNumberSInt32Type, &percent);

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
			const char * str_key = (const char *)CFStringGetCStringPtr(key, CFStringGetSystemEncoding());
			printf("%-15s\t%s\n", str_app_name, str_key);
		}
	}

	CFRelease(apps);
	return 0;
}

int Device::listDirectory(const char *dir_path)
{
	service_conn_t fd_afc;
	if (startService(AMSVC_AFC, &fd_afc) != 0) {
		return -1;
	}

	int ret = 0;
	FileAccess *fa = new FileAccess(fd_afc);
	if (fa->connect() != 0) {
		ret = -1;
		goto LISTDIR_END;
	}

	if (fa->listDirectory(dir_path) != 0) {
		ret = -1;
		goto LISTDIR_END;
	}

LISTDIR_END:
	delete fa;
	stopService(fd_afc);

	return ret;
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


