
#include "fileaccess.h"

FileAccess::FileAccess(service_conn_t fd)
	:_fd(fd), _afc(NULL)
{
}

FileAccess::~FileAccess()
{
}

int FileAccess::connect()
{
	mach_error_t status = AFCConnectionOpen(_fd, 0, &_afc);
	if (status != MDERR_OK) {
		return -1;
	}
	return 0;
}

int FileAccess::listDirectory(const char *dir_path)
{
	if (_afc == NULL) {
		return -1;
	}
	struct afc_directory *dir;
	if (AFCDirectoryOpen(_afc, dir_path, &dir) != MDERR_OK) {
		return -1;
	}
	char *d = NULL;
	char path[256];
	char tmbuf[64];
	while(true) {
		AFCDirectoryRead(_afc, dir, &d);
		if (d == NULL) {
			break;
		}
		snprintf(path, sizeof(path), "%s/%s", dir_path, d);
		CFMutableDictionaryRef file_dict = getFileInfo(path);
		if (file_dict == NULL) {
			continue;
		}
		CFStringRef ifmt = (CFStringRef)CFDictionaryGetValue(file_dict, CFSTR("st_ifmt"));
		const char *ifmt_cstr = (CFStringCompare(ifmt, CFSTR("S_IFDIR"), kCFCompareLocalized) == kCFCompareEqualTo) ? "d" : "-";
		CFStringRef size = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_size"));
		CFStringRef mtime = (CFStringRef)CFDictionaryGetValue(file_dict,CFSTR("st_mtime"));
		time_t mtimel = atoll(CFStringGetCStringPtr(mtime, kCFStringEncodingMacRoman)) / 1000000000L;

		struct tm *mtimetm = localtime(&mtimel);

		strftime(tmbuf, sizeof tmbuf, "%b %d %H:%M", mtimetm);

		printf("%s %16s %6s  %s\n", 
			ifmt_cstr, 
			tmbuf, 
			CFStringGetCStringPtr(size, kCFStringEncodingMacRoman),
			d
		);
		CFRelease(file_dict);
	}
	AFCDirectoryClose(_afc, dir);

	return 0;
}

int FileAccess::push(const char *local, const char *remote)
{
	return 0;
}

int FileAccess::pull(const char *remote, const char *local)
{
	if (_afc == NULL) {
		return -1;
	}
	CFMutableDictionaryRef file_dict = getFileInfo(remote);
	CFStringRef ifmt = (CFStringRef)CFDictionaryGetValue(file_dict, CFSTR("st_ifmt"));
	if (CFStringCompare(ifmt, CFSTR("S_IFDIR"), kCFCompareLocalized) != kCFCompareEqualTo) {
		int ret = copyFileFromDevice(remote, local);
		CFRelease(file_dict);
		return ret;
	}
	CFRelease(file_dict);

	struct afc_directory *dir;
	if (AFCDirectoryOpen(_afc, dir_path, &dir) != MDERR_OK) {
		return -1;
	}
	char *d = NULL;
	char path1[256], path2[256];
	while(true) {
		AFCDirectoryRead(_afc, dir, &d);
		if (d == NULL) {
			break;
		}
		if (strcmp(d, ".") == 0 || strcmp(d, "..") == 0) {
			continue;
		}
		snprintf(path1, sizeof(path1), "%s/%s", remote, d);
		file_dict = getFileInfo(path1);
		if (file_dict == NULL) {
			continue;
		}
		snprintf(path2, sizeof(path2), "%s/%s", local, d);
		CFStringRef ifmt = (CFStringRef)CFDictionaryGetValue(file_dict, CFSTR("st_ifmt"));
		if (CFStringCompare(ifmt, CFSTR("S_IFDIR"), kCFCompareLocalized) == kCFCompareEqualTo) {
			pull(path1, path2);
		} else {
			copyFileFromDevice(path1, path2);
		}
		CFRelease(file_dict);
	}
	AFCDirectoryClose(_afc, dir);

	return 0;
}

CFMutableDictionaryRef FileAccess::getFileInfo(const char *path)
{
    struct afc_dictionary *file_info;
	if (AFCFileInfoOpen(_afc, path, &file_info) != MDERR_OK) {
		return NULL;
	}
    CFMutableDictionaryRef file_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	char *key = NULL, *value = NULL;
	while(AFCKeyValueRead(file_info, &key, &value) == MDERR_OK) {
		if (key == NULL || value == NULL) {
			break;
		}
		CFStringRef ckey = CFStringCreateWithCString(NULL, key, kCFStringEncodingASCII);
		CFStringRef cval = CFStringCreateWithCString(NULL, value, kCFStringEncodingASCII);
		CFDictionarySetValue(file_dict, ckey, cval);
	} 
	AFCKeyValueClose(file_info);
	return file_dict;
}
