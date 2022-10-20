#include <stdio.h>
#include <strings.h>
#include "csapp.h"
#include "sbuf.h"


/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 32
#define NTHREADS 8

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

sbuf_t sbuf; /* Shared buffer of connected descriptors */


int handle_uri(char *uri, char *hostname, char *path, int *port);
void handle_proxy(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

int main(int argc, char **argv) {
  int i, listenfd, connfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
  char client_hostname[MAXLINE], client_port[MAXLINE];
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }
  listenfd = Open_listenfd(argv[1]);
  printf(">Server started listening port %s\n", argv[1]);
  sbuf_init(&sbuf, SBUFSIZE);
  printf(">Shared buffer initialized\n");
  for (i = 0; i < NTHREADS; i++){     /* Create worker threads */
    int *id = Malloc(sizeof(int));
    *id = i;
    Pthread_create(&tid, NULL, thread, (void *)id);
  }
  printf(">Worker threads created\n");

  Signal(SIGPIPE, SIG_IGN);  // Ignore SIGPIPE
  while (1) {
    clientlen = sizeof(struct sockaddr_storage);  /* Important! */
    connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    sbuf_insert(&sbuf, connfd);  /* Insert connfd in buffer */
    printf(">cilentfd %d in queue \n", connfd);
    printf(">Accepted connection from (%s, %s)\n", client_hostname, client_port);
  }
  sbuf_deinit(&sbuf);
  exit(0);
}

void *thread(void *vargp) {
  Pthread_detach(pthread_self());
  int id = *((int *)vargp);
  Free(vargp);
  while(1){
    int connfd = sbuf_remove(&sbuf);
    printf("Working thread[%d] >>>>>>>>>>>>>>>>>>>>>>>>> handling cilentfd[%d]\n", id, connfd);
    handle_proxy(connfd);
    Close(connfd);
    printf("Working thread[%d] <<<<<<<<<<<<<<<<<<<<<<<<<< cilentfd[%d] closed!\n\n", id, connfd);
  }
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

  rio_readinitb(&rio_cilent, fd);
  rio_readlineb(&rio_cilent, buf, MAXLINE);
//  if (strcmp(buf, "") == 0){
//    return;
//  }
  sscanf(buf, "%s %s %s", method, uri, version);
  printf("Request line:\n");
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
    serverfd = open_clientfd(hostname, port);
    if (serverfd < 0) {
      clienterror(fd, hostname, "404", "Not Found", "Proxy could not connect to this server");
      return;
    }

    // send request to server
    rio_readinitb(&rio_server, serverfd);

    sprintf(server_buf, "GET %s HTTP/1.0\r\n", pathname);
    rio_writen(serverfd, server_buf, strlen(server_buf));
    sprintf(server_buf, "Host: %s\r\n", hostname);
    rio_writen(serverfd, server_buf, strlen(server_buf));
    rio_writen(serverfd, user_agent_hdr, strlen(user_agent_hdr));
    rio_writen(serverfd, connection_hdr, strlen(connection_hdr));
    rio_writen(serverfd, proxy_connection_hdr, strlen(proxy_connection_hdr));
    rio_writen(serverfd, "\r\n", 2);

    // read response from server
    //TODO Cache
    obj[0] = '\0';
    while ((n = rio_readlineb(&rio_server, server_buf, MAXLINE)) != 0) {
      obj_len += n;
      if (obj_len <= MAX_OBJECT_SIZE) {
        sprintf(obj, "%s%s", obj, server_buf);
        obj[obj_len] = '\0';
      }
      rio_writen(fd, server_buf, n); // send response to client
    }
    printf("Respond %d bytes object:\n", obj_len);
    Close(serverfd);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE];

  /* Print the HTTP response headers */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n\r\n");
  rio_writen(fd, buf, strlen(buf));

  /* Print the HTTP response body */
  sprintf(buf, "<html><title>Proxy Error</title>");
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "<hr><em>My proxy server</em>\r\n");
  rio_writen(fd, buf, strlen(buf));
}