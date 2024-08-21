/*
 * Copyright (C) [2022-2024] [nyaaaww]
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "include/ruri.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define REU_SIZE 4096
typedef struct {
  int status_code;         // HTTP/1.1 '200' OK
  char content_type[128];  // Content-Type: application/gzip
  long content_length;     // Content-Length: 11683079
  char file_name[256];
} resp_header_def;

resp_header_def resp;

static int get_resp_header(const char *response, resp_header_def *resp) {
  char *pos = strstr(response, "HTTP/");
  if (pos) {
    sscanf(pos, "%*s %d", &resp->status_code);
  }
  pos = strstr(response, "Content-Type:");
  if (pos) {
    sscanf(pos, "%*s %s", resp->content_type);
  }
  pos = strstr(response, "Content-Length:");
  if (pos) {
    sscanf(pos, "%*s %ld", &resp->content_length);
  }
  return 0;
}

// 更新进度条的函数
void update_progress_bar(int downloaded, int total_size) {
  float progress =
      (float)downloaded / total_size * 100.0f;
  int bar_length = 50;
  int pos = (int)(progress / 100.0f * bar_length);
  printf("\033[?25l");
  printf("\r下载中... [");
  for (int i = 0; i < bar_length; i++) {
    if (i < pos) {
      printf("=");
    } else if (i == pos) {
      printf(">");
    } else {
      printf(" ");
    }
  }
  printf("] %d%%", (int)progress);
  if (downloaded >= total_size) {
    printf("\033[?25h");
  } else {
    printf("\r");
  }

  fflush(stdout);
  usleep(100);
}

static int getHttpHead(int fd, char *buf, int bufLen) {
  char tmp[1] = {0};
  int i = 0;
  int offset = 0;
  int nbytes = 0;

  while ((nbytes = recv(fd, tmp, 1, 0)) == 1) {
    if (offset > bufLen - 1) {
      return bufLen;
    }

    if (i < 4) {
      if (tmp[0] == '\r' || tmp[0] == '\n') {
        i++;
      } else {
        i = 0;

        strncpy(buf + offset, tmp, 1);
        offset++;
      }
    }
    if (4 == i) {
      return offset;
    }
  }

  return -1;
}

static int geturl(char *url) {
  int cfd;
  struct sockaddr_in cadd;
  struct hostent *pURL = NULL;
  char host[BUFSIZE], GET[BUFSIZE];
  char request[REU_SIZE];
  char text[BUFSIZE] = {0};

  sscanf(url, "%*[^//]//%[^/]%s", host, GET);
  //  printf("Host = %s\n", host);
  //  printf("GET = %s\n", GET);

  // const char *red_color = "\033[31m";
  const char *green_color = "\033[32m";
  const char *reset_color = "\033[0m";

  printf("%sUrl%s :  %s\n", green_color, reset_color, url);

  if (-1 == (cfd = socket(AF_INET, SOCK_STREAM, 0))) {
    printf("create socket failed of client!\n");
    return (-1);
  }

  if ((pURL = gethostbyname(host)) == 0) {
    printf("Invalid host!\n");
    return -1;
  }

  memset(&cadd, 0x00, sizeof(struct sockaddr_in));
  cadd.sin_family = AF_INET;
  cadd.sin_addr.s_addr = *((unsigned long *)pURL->h_addr_list[0]);
  cadd.sin_port = htons(80);

  char UA[BUFSIZE] = "curl/8.6.0";
  snprintf(request, REU_SIZE,
           "GET %s HTTP/1.1\r\n"
           "HOST: %s\r\n"
           "User-Agent: %s\r\n"
           "Cache-Control: no-cache\r\n"
           "Connection: close\r\n\r\n",
           GET, host, UA);

  if (-1 == connect(cfd, (struct sockaddr *)&cadd, (socklen_t)sizeof(cadd))) {
    printf("connect failed of client!\n");
    return -1;
  }

  if (-1 == send(cfd, request, strlen(request), 0)) {
    printf("Send request failed!\n");
    return -1;
  }

  getHttpHead(cfd, text, sizeof(text));
  printf("%sHead%s :  %s\n", green_color, reset_color, text);
  get_resp_header(text, &resp);
  printf(
      "%sResult%s :\n Content_length:%ld\n Status_code:%d\n Content-Type:%s\n",
      green_color, reset_color, resp.content_length, resp.status_code,
      resp.content_type);

  if (resp.status_code == 404) {
    fprintf(stderr, "Error: 404 No such file\n");
    return -1;
  }

  return cfd;
}

int downloader(char addr[], char *save_nam) {
  int sockfd = -1;
  char buf[BUFSIZE] = {0};
  char fileName[BUFSIZE] = {0};
  int fd = -1;

  snprintf(fileName, BUFSIZE, "%s", save_nam);
  if ((sockfd = geturl(addr)) < 0) {
    return -1;
  }

  printf("file = %s\n", fileName);
  remove(fileName);

  fd = open(fileName, O_WRONLY | O_CREAT, 777);
  if (fd == -1) {
    perror("Open error");
    goto __END;
  }
  int len = 0;
  while (1) {
    memset(buf, 0, BUFSIZE);
    int cr;
    cr = recv(sockfd, buf, BUFSIZE, 0);
    len += cr;
    if (resp.content_length != 0) {
      printf("\r[%d/%ld]", len, resp.content_length);
      update_progress_bar(len, resp.content_length);
      fflush(stdout);
    }
    if (cr <= 0) {
      printf("\n");
      break;
    }
    if (write(fd, buf, cr) < 0) {
      perror("Err");
      goto __END;
    }
  }
__END:
  close(fd);
  close(sockfd);
  return 0;
}

int check_sha256(const char *rfs_dir) {


  FILE *fp = fopen("./SHA256SUMS", "r");
  if (!fp) {
    fprintf(stderr, "No SHA256SUMS file!\n");
    return 0;
  }

  char data[100];
  char *temp_p = fgets(data, sizeof(data), fp);
  fclose(fp);
  if (!temp_p) {
    return 0;
  }

  char *digest = calculate_file_sha256(rfs_dir);
  if (!digest) {
    return -1;
  }

  int is_valid = (memcmp(digest, data, 64) == 0);

  if (!is_valid) {
    printf("\033[0m\033[1;31mCheck sha256 failed!\033[0m\n");
  }

  free(digest);

  return is_valid;
}


static char *tok_html(char *tmp_file) {
  char *ret1, *ret2;
  char *ret3 = calloc(sizeof(char) * 50, 1);
  FILE *fp;
  int file_size, retfd;
  char *tmp;
  fp = fopen(tmp_file, "rw");
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  tmp = (char *)calloc(1, file_size * sizeof(char) + 10);
  fseek(fp, 0, SEEK_SET);
  retfd = fread(tmp, sizeof(char), file_size, fp);
  if (retfd != file_size) {
    printf("%d\n", retfd);
    printf("Read Error!\n");
    exit(-1);
  }

  char b[1000][1000];
  int i = 0;
  char *ptr = NULL;
  char *token = strtok_r(tmp, "/", &ptr);
  while (token != NULL) {
    strcpy(b[i++], token);
    token = strtok_r(NULL, " ", &ptr);
  }

  for (int j = 0; j < i; j++) {
    ret1 = strstr(b[j], "title");
    if (ret1 == NULL) {
      continue;
    } else {
      ret2 = strstr(b[j - 1], "href");
      if (ret2 == NULL) {
        continue;
      } else {
        sscanf(ret2, "%*[^\"]\"%[^\"]", ret3);
        break;
      }
    }
  }
  fclose(fp);
  free(tmp);
  return ret3;
}



int install(const char *pod_name, const char *pod_ver,
                   const char *pod_arch) {
  if (pod_name == 0 && pod_ver == 0) {
    return -1;
  }

char *htm_file = "./default";
char *sha_file = "./SHA256SUMS";

  const char *sou_link =
      "https://mirrors.tuna.tsinghua.edu.cn/lxc-images/images/";
  char def_link[PATH_MAX] = {0}, tok_file[PATH_MAX] = {0};
  snprintf(def_link,
           strlen(sou_link) + strlen(pod_name) + strlen(pod_ver) +
               strlen(pod_arch) + 12,
           "%s%s/%s/%s/default/", sou_link, pod_name, pod_ver, pod_arch);
  const char *home = getenv("HOME");
  char tmp[PATH_MAX] = {0};
//  snprintf(tmp, strlen(home) + 11, "%s/tmp", home);
  //printf("%s\n", tmp);
  printf("%s\n", def_link);
//  snprintf(tok_file, strlen(tmp) + 10, "%s/default", tmp);
  if (downloader(def_link, htm_file) < 0) {
    return -1;
  }
  char *ret = tok_html(htm_file);
  printf("%s\n", ret);
  char pod_url[PATH_MAX] = {0}, pod_sha[PATH_MAX] = {0};
  snprintf(pod_url, strlen(def_link) + strlen(ret) + 100, "%s%srootfs.tar.xz",
           def_link, ret);
  snprintf(pod_sha, strlen(def_link) + strlen(ret) + 100, "%s%sSHA256SUMS",
           def_link, ret);

  char rfs_n[PATH_MAX]={0};
  snprintf(rfs_n,strlen(pod_name)+strlen(pod_arch)+strlen(pod_ver)+20,"%s_%s+%s.tar.xz",pod_name,pod_ver,pod_arch);
  if (downloader(pod_url, rfs_n) < 0) {
    return -1;
  }
  if (downloader(pod_sha, sha_file) < 0) {
    return -1;
  }
  if (check_sha256(rfs_n) == -1) {
    return -1;
  }

    if (unlink(htm_file)==0 && unlink(sha_file)== 0) {
return 0;
    } else {
        perror("Clear failed!");
	return -1;
    }

}
