iosutil
=======

iosutil is used for connect with unjailbreak ios device and pc, now is available only mac.


### Usage
	iosutil version 0.9.0
	
	  -s <device id>                                      specific device
	  devices                                             list all connected devices
	
	device commands:
	  install <.app/.ipa path>                            install app
	  uninstall <bundle id>                               uninstall app
	  listapp                                             list all installed apps
	  logcat                                              tail syslog
	  ls [-b <bundle id|crash>] <path>                    ls directory
	  push [-b <bundle id|crash>] <local> <remote>        copy file/dir to device
	  pull [-b <bundle id|crash>] <remote> <local>        copy file/dir from device
	
	example:
	  iosutil ls -b crash /
	  iosutil ls -b com.baidu.BaiduMobile /Documents
