
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
	CFMutableDictionaryRef getFileInfo(const char *path);
private:
	service_conn_t _fd;
	struct afc_connection *_afc;
};

#endif	//__FILEACCESS_H__
