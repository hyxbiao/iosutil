
#ifndef	__FILEACCESS_H__
#define	__FILEACCESS_H__

#include "MobileDevice.h"

class FileAccess
{
public:
	typedef	void FDIR;
	typedef void FFILE;

	enum FileType {
		F_ERROR		=	0x00,
		F_NOEXIST	=	0x01,
		F_FILE		=	0x02,
		F_DIR		=	0x03
	};
	enum FileMode {
		FM_READ			=	0x01,
		FM_WRITE		=	0x02,
		FM_READWRITE	=	0x03
	};
public:
	FileAccess(service_conn_t fd);
	~FileAccess();

	int connect();
	int listDirectory(const char *dir_path);
	int push(const char *local, const char *remote);
	int pull(const char *remote, const char *local);
	int copy(const char *from, const char *to, bool isfromdevice, bool istodevice);
private:
	int copyDirectory(const char *from, const char *to, bool isfromdevice, bool istodevice);
	int copyFile(const char *from, const char *to, bool isfromdevice, bool istodevice);

	CFMutableDictionaryRef getFileInfo(const char *path);
	FileType getFileType(const char *path, bool isdevice);

	int mkdir(const char *path, bool isdevice);

	FDIR *opendir(const char *path, bool isdevice);
	char *readir(FDIR *dir, bool isdevice);
	int closedir(FDIR *dir, bool isdevice);

	FFILE *openfile(const char *path, int mode, bool isdevice);
	size_t readfile(FFILE *fd, char *buf, size_t bufsize, bool isdevice);
	size_t writefile(FFILE *fd, char *buf, size_t bufsize, bool isdevice);
	int closefile(FFILE *fd, bool isdevice);

private:
	service_conn_t _fd;
	struct afc_connection *_afc;
};

#endif	//__FILEACCESS_H__
