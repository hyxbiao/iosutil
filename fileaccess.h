
#ifndef	__FILEACCESS_H__
#define	__FILEACCESS_H__

#include "MobileDevice.h"

class FileAccess
{
public:
	FileAccess(service_conn_t fd);
	~FileAccess();

	int connect();
	int listDirectory(const char *dir_path);
	int push(const char *local, const char *remote);
	int pull(const char *remote, const char *local);
	CFMutableDictionaryRef getFileInfo(const char *path);
private:
	int copyFileFromDevice(const char *remote, const char *local);
private:
	service_conn_t _fd;
	struct afc_connection *_afc;
};

#endif	//__FILEACCESS_H__
