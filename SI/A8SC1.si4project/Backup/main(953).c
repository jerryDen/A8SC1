#include <stdio.h>
#include "curl/curl.h"


#define true  0
#define false -1
typedef char bool;

static size_t write_data(void *buf, size_t size, size_t nmemb, void *userp);

static size_t write_data(void *buf, size_t size, size_t nmemb, void *userp)
{

   
   printf("buf:%s write_data: %d, \n", buf,nmemb);

   return 0;

}

bool getUrl(char *filename)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: Agent-007");
    curl = curl_easy_init();    // 初始化
    if (curl)
    {
        //curl_easy_setopt(curl, CURLOPT_PROXY, "10.99.60.201:8080");// 代理
        
		
        //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);         // 改协议头
        curl_easy_setopt(curl, CURLOPT_URL,"https://api.yunjiangkj.com/appVilla/login");
		//设置接收回调函数
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); 
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000);
		res = curl_easy_perform(curl);	 // 执行申请
		if (res != 0) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		}
		printf("----------------\n");
		return true;
    }
}

int main(void)
{
    getUrl("get.html");
	while(1){sleep(1);};
	


}


































