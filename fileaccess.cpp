
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
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
		if (strcmp(d, ".") == 0 || strcmp(d, "..") == 0) {
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", dir_path, d);
		CFMutableDictionaryRef file_dict = getFileInfo(path);
		if (file_dict == NULL) {
			continue;
		}
		//CFShow(file_dict);
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
	return copy(local, remote, false, true);
}

int FileAccess::pull(const char *remote, const char *local)
{
	return copy(remote, local, true, false);
}

int FileAccess::copy(const char *from, const char *to, bool isfromdevice, bool istodevice)
{
	if (_afc == NULL) {
		return -1;
	}

	FileType from_file_type = getFileType(from, isfromdevice);
	if (from_file_type == F_NOEXIST) {
		printf("From path is no exist!\n");
		return -1;
	}
	FileType to_file_type = getFileType(to, istodevice);
	if (to_file_type == F_NOEXIST) {
		if (getFileType(dirname((char *)to), istodevice) != F_DIR) {
			printf("To path is not valid!\n");
			return -1;
		}
	}

	char *basepath = basename((char *)from);
	char tmp[256];
	if (from_file_type == F_FILE) {
		if (to_file_type == F_DIR) {
			snprintf(tmp, sizeof(tmp), "%s/%s", to, basepath);
			return copyFile(from, tmp, isfromdevice, istodevice);
		}
		return copyFile(from, to, isfromdevice, istodevice);
	} else {
		if (to_file_type == F_FILE) {
			printf("To path is file!\n");
			return -1;
		}
		char *newtopath = (char *)to;
		if (to_file_type == F_DIR) {
			snprintf(tmp, sizeof(tmp), "%s/%s", to, basepath);
			newtopath = tmp;;
		}
		if (this->mkdir(newtopath, istodevice) < 0) {
			printf("mkdir <%s> fail!\n", newtopath);
			return -1;
		}
		return copyDirectory(from, newtopath, isfromdevice, istodevice);
	}

	return 0;
}

int FileAccess::remove(const char *path)
{
	if (_afc == NULL) {
		return -1;
	}

	FileType file_type = getFileType(path, true);
	if (file_type == F_NOEXIST) {
		printf("Path is no exist!\n");
		return -1;
	}
	int ret = 0;
	if (file_type == F_DIR) {
		char newpath[256];
		char *d = NULL;
		FDIR *dir = this->opendir(path, true);
		if (dir == NULL) {
			return -1;
		}
		while (true) {
			d = this->readir(dir, true);
			if (d == NULL) {
				break;
			}
			if (strcmp(d, ".") == 0 || strcmp(d, "..") == 0) {
				continue;
			}
			snprintf(newpath, sizeof(newpath), "%s/%s", path, d);
			if (this->remove(newpath) != 0) {
				ret = -1;
				break;
			}
		}
		this->closedir(dir, true);
	}
	if (ret == 0) {
		return AFCRemovePath(_afc, path);
	}
	return ret;
}

int FileAccess::copyDirectory(const char *from, const char *to, bool isfromdevice, bool istodevice)
{
	FDIR *dir = this->opendir(from, isfromdevice);
	if (dir == NULL) {
		return -1;
	}

	char nfrom[256], nto[256];

	int ret = 0;
	char *d = NULL;
	while (true) {
		d = this->readir(dir, isfromdevice);
		if (d == NULL) {
			break;
		}
		if (strcmp(d, ".") == 0 || strcmp(d, "..") == 0) {
			continue;
		}
		snprintf(nfrom, sizeof(nfrom), "%s/%s", from, d);
		FileType type = getFileType(nfrom, isfromdevice);
		if (type == F_NOEXIST) {
			continue;
		}
		snprintf(nto, sizeof(nto), "%s/%s", to, d);
		if (type == F_DIR) {
			if (this->mkdir(nto, istodevice) < 0) {
				printf("mkdir <%s> fail!\n", nto);
				ret = -1;
				break;
			}
			if (copyDirectory(nfrom, nto, isfromdevice, istodevice) != 0) {
				ret = -1;
				break;
			}
		} else {
			if (copyFile(nfrom, nto, isfromdevice, istodevice) != 0) {
				ret = -1;
				break;
			}
		}
	}
	this->closedir(dir, isfromdevice);
	return ret;
}

int FileAccess::copyFile(const char *from, const char *to, bool isfromdevice, bool istodevice)
{
	FFILE *from_fd = openfile(from, FM_READ, isfromdevice);
	if (from_fd == NULL) {
		printf("Open \"%s\" fail!\n", from);
		return -1;
	}

	FFILE *to_fd = openfile(to, FM_WRITE, istodevice);
	if (to_fd == NULL) {
		printf("Open \"%s\" fail!\n", to);
		closefile(from_fd, isfromdevice);
		return -1;
	}
	int bufsize = 4096;
	char *buf = (char *)malloc(bufsize);
	if (buf == NULL) {
		closefile(to_fd, istodevice);
		closefile(from_fd, isfromdevice);
		return -1;
	}
	int ret = 0;
	while (true) {
		int readed = readfile(from_fd, buf, bufsize, isfromdevice);
		if (readed == 0) {
			break;
		}
		if (writefile(to_fd, buf, readed, istodevice) == 0) {
			ret = -1;
			break;
		}
	}
	free(buf);
	closefile(to_fd, istodevice);
	closefile(from_fd, isfromdevice);

	printf("Copy \"%s\" done\n", from);

	return ret;
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

FileAccess::FileType FileAccess::getFileType(const char *path, bool isdevice)
{
	FileType type = F_ERROR;
	if (isdevice) {
		CFMutableDictionaryRef file_dict = getFileInfo(path);
		if (file_dict == NULL) {
			return F_NOEXIST;
		}
		CFStringRef ifmt = (CFStringRef)CFDictionaryGetValue(file_dict, CFSTR("st_ifmt"));
		type = (CFStringCompare(ifmt, CFSTR("S_IFDIR"), kCFCompareLocalized) == kCFCompareEqualTo) ? F_DIR : F_FILE;
		CFRelease(file_dict);
	} else {
		struct stat s;
		if (stat(path, &s) != 0) {
			return F_NOEXIST;
		}
		type = (s.st_mode & S_IFDIR) ? F_DIR : F_FILE;
	}
	return type;
}

int FileAccess::mkdir(const char *path, bool isdevice)
{
	if (isdevice) {
		if (AFCDirectoryCreate(_afc, path) != MDERR_OK) {
			return -1;
		}
	} else {
		if (::mkdir(path, 0755) < 0) {
			return -1;
		}
	}
	return 0;
}

FileAccess::FDIR * FileAccess::opendir(const char *path, bool isdevice)
{
	if (isdevice) {
		struct afc_directory *dir;
		if (AFCDirectoryOpen(_afc, path, &dir) == MDERR_OK) {
			return (FDIR *)dir;
		}
	} else {
		return (FDIR *)::opendir(path);
	}
	return NULL;
}

char * FileAccess::readir(FDIR *dir, bool isdevice)
{
	if (isdevice) {
		char *d = NULL;
		struct afc_directory *ndir = (struct afc_directory *)dir;
		if (AFCDirectoryRead(_afc, ndir, &d) != MDERR_OK) {
			return NULL;
		}
		return d;
	} else {
		DIR *ndir = (DIR *)dir;
		struct dirent* dp = ::readdir(ndir);
		if (dp == NULL) {
			return NULL;
		}
		return dp->d_name;
	}
	return NULL;
}

int FileAccess::closedir(FDIR *dir, bool isdevice)
{
	if (isdevice) {
		return AFCDirectoryClose(_afc, (struct afc_directory *)dir);
	} else {
		return ::closedir((DIR *)dir);
	}
	return 0;
}

FileAccess::FFILE * FileAccess::openfile(const char *path, int mode, bool isdevice)
{
	if (isdevice) {
		afc_file_ref rfd;
		int r = AFCFileRefOpen(_afc, path, mode, &rfd);
		if (r != 0) {
			return NULL;
		}
		return (FFILE *)rfd;
	} else {
		const char *m = (mode == FM_READ) ? "r" : "w";
		FILE *fp = fopen(path, m);
		return (FFILE *)fp;
	}
	return NULL;
}

size_t FileAccess::readfile(FFILE *fd, char *buf, size_t bufsize, bool isdevice)
{
	if (isdevice) {
		unsigned int readed = bufsize;
		int ret = AFCFileRefRead(_afc, (afc_file_ref)fd, buf, &readed);
		if (ret != MDERR_OK) {
			return 0;
		}
		return readed;
	} else {
		return fread(buf, 1, bufsize, (FILE *)fd);
	}
	return 0;
}

size_t FileAccess::writefile(FFILE *fd, char *buf, size_t bufsize, bool isdevice)
{
	if (isdevice) {
		int ret = AFCFileRefWrite(_afc, (afc_file_ref)fd, buf, bufsize);
		if (ret != MDERR_OK) {
			return 0;
		}
		return 1;
	} else {
		return fwrite(buf, bufsize, 1, (FILE *)fd);
	}
	return 0;
}

int FileAccess::closefile(FFILE *fd, bool isdevice)
{
	if (fd == NULL) {
		return 0;
	}
	if (isdevice) {
		return AFCFileRefClose(_afc, (afc_file_ref)fd);
	} else {
		return fclose((FILE *)fd);
	}
	return 0;
}

