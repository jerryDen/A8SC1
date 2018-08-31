#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <curl/multi.h>
 
static const char *urls[] = {
  "http://www.baidu.com",
};
 
#define MAX 8 /* number of simultaneous transfers */
#define CNT sizeof(urls)/sizeof(char*) /* total number of transfers to do */
 
/*此函数读取libcurl发送数据后的返回信息，如果不设置此函数，
那么返回值将会输出到控制台，影响程序性能*/
static size_t cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}
 
//设置单个easy handler的属性添加单个easy handler到multi handler中，
static void init(CURLM *cm, int i)
{
  CURL *eh = curl_easy_init();
 
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt(eh, CURLOPT_URL, urls[i]);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, urls[i]);
  curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
 
  //添加easy handler 到multi handler中
  curl_multi_add_handle(cm, eh);
}
 
int main(void)
{
  CURLM *cm;
  CURLMsg *msg;
  long curl_timeo;
  unsigned int C=0;
  int max_fd, msgs_left, still_running = -1;//still_running判断multi handler是否传输完毕
  fd_set fd_read, fd_write, fd_except;
  struct timeval T;
 
  curl_global_init(CURL_GLOBAL_ALL);
 
  cm = curl_multi_init();
 
  //现在multi handler的最大连接数
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX);
 
  for(C = 0; C < MAX; ++C) {
    init(cm, C);
  }
 
  
  do{
    curl_multi_perform(cm, &still_running);
 
    if(still_running) {
      FD_ZERO(&fd_read);
      FD_ZERO(&fd_write);
      FD_ZERO(&fd_except);
 
	  //获取multi curl需要监听的文件描述符集合 fd_set
      if(!curl_multi_fdset(cm, &fd_read, &fd_write, &fd_except, &max_fd)) {
        fprintf(stderr, "E: curl_multi_fdset\n");
        return EXIT_FAILURE;
      }
 
      if(!curl_multi_timeout(cm, &curl_timeo)) {
        fprintf(stderr, "E: curl_multi_timeout\n");
        return EXIT_FAILURE;
      }
      if(curl_timeo == -1)
        curl_timeo = 100;
 
	  //如果max_fd返回-1，休眠一段时间后继续执行curl_multi_perform
      if(max_fd == -1) {
        sleep((unsigned int)curl_timeo / 1000);
      }
      else {
        T.tv_sec = curl_timeo/1000;
        T.tv_usec = (curl_timeo%1000)*1000;
 
		/* 执行监听，当文件描述符状态发生改变的时候返回
         * 返回0，程序调用curl_multi_perform通知curl执行相应操作
         * 返回-1，表示select错误
		 */
        if(0 > select(max_fd+1, &fd_read, &fd_write, &fd_except, &T)) {
          fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
              max_fd+1, curl_timeo, errno, strerror(errno));
          return EXIT_FAILURE;
        }
      }
    }
 
    while((msg = curl_multi_info_read(cm, &msgs_left))) {
      if(msg->msg == CURLMSG_DONE) {
        char *url;
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
        fprintf(stderr, "R: %d - %s <%s>\n",
                msg->data.result, curl_easy_strerror(msg->data.result), url);
		/*当一个easy handler传输完成，此easy handler仍然仍然停留在multi stack中,
		调用curl_multi_remove_handle将其从multi stack中移除,然后调用curl_easy_cleanup将其关闭*/
        curl_multi_remove_handle(cm, e);
        curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
    }
  }while(still_running);
 
  //当multi stack中的所有传输都完成时，调用 curl_multi_cleanup关闭multi handler
  curl_multi_cleanup(cm);
  curl_global_cleanup();
 
  return EXIT_SUCCESS;
}
