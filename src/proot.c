#include "include/ruri.h"

int check_proot()
{
	if (execute_command("proot >/dev/null 2>&1") == 127) {
	error("{red}You have not install proot.{clear}\n")
		return -1;
	}
	return 0;
}

void run_proot_container(const char *container_dir)
{
	if (check_proot()== -1){goto EXIT;}
	execute_command("unset LD_PRELOAD");
	char proot_cmd[2048] = { 0 };
	strncat(proot_cmd, "proot", 5);
	strncat(proot_cmd, " --link2symlink", 15);
	strncat(proot_cmd, " -0", 3);
	strncat(proot_cmd, " -r ", 4);
	strncat(proot_cmd, container_dir, strlen(container_dir));
	strncat(proot_cmd, " -b /dev", 8);
	strncat(proot_cmd, " -b /proc", 9);
	strncat(proot_cmd, " -b ", 4);
	strncat(proot_cmd, container_dir, strlen(container_dir));
	strncat(proot_cmd, "/root:/dev/shm", 14);
	strncat(proot_cmd, " -w /root /usr/bin/env", 22);
	strncat(proot_cmd, " -i HOME=/root", 14);
	strncat(proot_cmd, " PATH=/usr/local/sbin:/usr/local/bin:/bin:/sbin:/usr/sbin:/usr/games:/usr/local/games", 86);
	strncat(proot_cmd, " SHELL=/bin/bash", 16);
	strncat(proot_cmd, " TERM=$TERM", 11);
	strncat(proot_cmd, " LANG=C.UTF-8", 13);
	strncat(proot_cmd, " /bin/bash --login", 18);
	if (execute_command(proot_cmd) == -1) {
EXIT:
		error("{red}Failed to execute `%s`\nexecv() returned: %d\nerror reason: %s\nNote: unset $LD_PRELOAD before running ruri might fix this{clear}\n", proot_cmd, errno, strerror(errno));
	}
}
