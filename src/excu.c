/*
 * Copyright (C) 2023-2024  nyaaaww
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_command(const char *cmd) {
  pid_t pid = fork();

  if (pid == -1) {
    perror("fork");
    return -1;
  } else if (pid == 0) {
    execlp("sh", "sh", "-c", cmd, NULL);
    perror("execlp");
    return -1;
  } else {
    int status;
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
