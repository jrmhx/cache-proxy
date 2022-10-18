#include <stdio.h>
#include <strings.h>
#include "csapp.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

int handle_uri(char *uri, char *hostname, char *path, int *port);
void handle_proxy(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

int main(int argc, char **argv) {
  //printf("%s", user_agent_hdr);
  int listenfd, *connfdp;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    Pthread_create(&tid, NULL, thread, connfdp);
  }
  exit(0);
}

void *thread(void *vargp) {
  int connfd = *((int *) vargp);
  printf(">>>>>>>>>>>>>>>>>>>>>>>>> new cilentfd %d\n", connfd);
  Pthread_detach(pthread_self());
  Free(vargp);
  handle_proxy(connfd);
  Close(connfd);
  printf("<<<<<<<<<<<<<<<<<<<<<<<<<< close cilentfd %d\n", connfd);
  return NULL;
}

int handle_uri(char *uri, char *hostname, char *path, int *port) {
  const char *temp_head, *temp_tail;
  char scheme[10], port_str[10];
  int temp_len;

  temp_head = uri;

  // get scheme
  temp_tail = strstr(temp_head, "://");
  if (temp_tail == NULL) {
    return 0;
  }
  temp_len = temp_tail - temp_head;
  strncpy(scheme, temp_head, temp_len);
  scheme[temp_len] = '\0';
  if (strcasecmp(scheme, "http")) {
    return 0;
  }

  // get hostname
  temp_head = temp_tail + 3;
  temp_tail = temp_head;
  while (*temp_tail != '\0') {
    if (*temp_tail == ':' || *temp_tail == '/') {
      break;
    }
    temp_tail++;
  }
  temp_len = temp_tail - temp_head;
  strncpy(hostname, temp_head, temp_len);
  hostname[temp_len] = '\0';

  // get port
  temp_head = temp_tail;
  if (*temp_head == ':') {
    temp_head++;
    temp_tail = temp_head;
    while (*temp_tail != '\0') {
      if (*temp_tail == '/') {
        break;
      }
      temp_tail++;
    }
    temp_len = temp_tail - temp_head;
    strncpy(port_str, temp_head, temp_len);
    port_str[temp_len] = '\0';
    *port = atoi(port_str);
  } else {
    *port = 80;
  }

  // get path
  temp_head = temp_tail;
  if (*temp_head == '/') {
    temp_tail = temp_head;
    while (*temp_tail != '\0') {
      temp_tail++;
    }
    temp_len = temp_tail - temp_head;
    strncpy(path, temp_head, temp_len);
    path[temp_len] = '\0';
    return 1;
  } else {
    path[0] = '/';
    path[1] = '\0';
    return 1;
  }
}

void handle_proxy(int fd) {
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
  char server_buf[MAXLINE], obj[MAX_OBJECT_SIZE];
  rio_t rio_cilent, rio_server;
  int serverfd;
  int port_int;
  size_t n, obj_len = 0;

  Rio_readinitb(&rio_cilent, fd);
  Rio_readlineb(&rio_cilent, buf, MAXLINE);
//  if (strcmp(buf, "") == 0){
//    return;
//  }
  sscanf(buf, "%s %s %s", method, uri, version);
  printf("Request headers:\n");
  printf("%s", buf);
  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not Implemented", "Proxy does not implement this method");
    return;
  }
  if (!handle_uri(uri, hostname, pathname, &port_int)) {
    clienterror(fd, uri, "400", "Bad Request", "Proxy could not parse the request");
    return;
  } else {
    // connect to server
    printf("hostname: %s, url: %s, port: %d\n", hostname, pathname, port_int);
    sprintf(port, "%d", port_int);
    serverfd = Open_clientfd(hostname, port);

    // send request to server
    Rio_readinitb(&rio_server, serverfd);

    sprintf(server_buf, "GET %s HTTP/1.0\r\n", pathname);
    Rio_writen(serverfd, server_buf, strlen(server_buf));
    sprintf(server_buf, "Host: %s\r\n", hostname);
    Rio_writen(serverfd, server_buf, strlen(server_buf));
    Rio_writen(serverfd, user_agent_hdr, strlen(user_agent_hdr));
    Rio_writen(serverfd, connection_hdr, strlen(connection_hdr));
    Rio_writen(serverfd, proxy_connection_hdr, strlen(proxy_connection_hdr));
    Rio_writen(serverfd, "\r\n", 2);

    // read response from server
    obj[0] = '\0';
    while ((n = Rio_readlineb(&rio_server, server_buf, MAXLINE)) != 0) {
      obj_len += n;
      if (obj_len <= MAX_OBJECT_SIZE) {
        sprintf(obj, "%s%s", obj, server_buf);
        obj[obj_len] = '\0';
      }
      Rio_writen(fd, server_buf, n); // send response to client
//      if (strcmp(server_buf, "\r\n") == 0) {
//        break;
//      }
    }
    printf("Respond %d bytes object:\n", obj_len);
    Close(serverfd);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Proxy Error</title>");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>The my proxy server</em>\r\n");
  Rio_writen(fd, buf, strlen(buf));
}