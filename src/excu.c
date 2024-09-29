#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// 函数：在子进程中执行shell命令
int execute_command(const char *cmd) {
  pid_t pid = fork();  // 创建子进程

  if (pid == -1) {
    // fork失败
    perror("fork");
    return -1;
  } else if (pid == 0) {
    execlp("sh", "sh", "-c", cmd, NULL);
    // 如果exec成功，这里不会执行
    perror("execlp");
    return -1;
  } else {
    // 父进程
    int status;
    // 等待子进程结束
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
   //   printf("Command executed successfully, exit status: %d\n",WEXITSTATUS(status));
             return WEXITSTATUS(status);
    } else {
      printf("Command terminated abnormally.\n");
      return -1;
    }
  }
}
