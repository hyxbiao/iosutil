#include <stdio.h>
#include "manager.h"

#define	VERSION	"0.9.1"

void usage() {
    printf(
        "iosutil version "VERSION"\n\n"
        "  -s <device id>                                      specific device\n"
        "  devices                                             list all connected devices\n"
		"\n"
        "device commands:\n"
        "  install <.app/.ipa path>                            install app\n"
		"  uninstall <bundle id>                               uninstall app\n"
        "  listapp                                             list all installed apps\n"
        "  logcat                                              tail syslog\n"
        "  ls [-b <bundle id|crash>] <path>                    ls directory\n"
        "  push [-b <bundle id|crash>] <local> <remote>        copy file/dir to device\n"
        "  pull [-b <bundle id|crash>] <remote> <local>        copy file/dir from device\n"
		"\n"
		"example:\n"
		"  iosutil ls /\n"
		"  iosutil ls -b crash /\n"
		"  iosutil ls -b com.baidu.BaiduMobile /Documents\n"
		"\n"
	);
	exit(0);
}

int main(int argc, char *argv[])
{
	Manager *manager = Manager::getInstance();

	if (manager->parse(argc, argv) < 0) {
		usage();
	}

	manager->run();

	return 0;
}

