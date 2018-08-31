#include <stdio.h>
#include <string.h>
#include "httpClient.h"
#include "md5.h"
#include "common/cJSON.h"
#include "common/Utils.h"
#include "YJbackground.h"
#include "localDevice.h"
#include "common/Utils.h"
#include "common/sqlite3.h" 
//#include "common/sqlite3ext.h"

static int print_jsom(char *json_string)  
{  
    char *out;  
    cJSON *jsonroot = cJSON_Parse(json_string);  
    out=cJSON_Print(jsonroot);  
	if(out != NULL)
    	printf("%s\n",out);  
  
    cJSON_Delete(jsonroot);  
    free(out);  
    return 1;  
}  
pYJbackgroundOps YJbackground;

int wf_callback(void *arg, int nr, char **values, char **names)  
{  
    int i;  
    FILE *fd;  
    char str[128];  
    fd = (FILE *)arg; //将void *的参数强制转换为FILE *   
    for (i=0; i<nr; i++) { //将查询结果写入到文件中   
        memset(str, '\0', 128);  
        sprintf(str, "\t%s\t", values[i]);  
        fwrite(str, sizeof(char), sizeof(str), fd);  
    }  
    memset(str, '\0', 128);  
    fwrite("\n", sizeof(char), 2, fd); //写完一条记录换行   
    return 0;  //callback函数正常返回0   
}  


void testSqlite(void)
{
	char sql[128];  
    sqlite3 *db;  
    FILE *fd;  
  
    sqlite3_open("test.db", &db);  //打开（或新建）一个数据库   
    memset(sql, '\0', 128);  
    /* 新建一张表 */  
    strcpy(sql, "create table tb(id INTEGER PRIMARY KEY, data TEXT)");  
    sqlite3_exec(db, sql, NULL, NULL, NULL);  
  
    /* 新建一个文件，把数据库的查询结果保存在该文件中 */  
    fd = fopen("test", "w");  
    fwrite("Result: \n", sizeof(char), 10, fd);  
    memset(sql, '\0', 128);  
    strcpy(sql, "select * from tb");  
    sqlite3_exec(db, sql, wf_callback, fd, NULL);  
    fclose(fd);  

    sqlite3_close(db); //关闭数据库

}



int main(void)
{
	char buffer[1024] = { 0};
	int ret = 0;
	//MD5(000c29d3d6db1533723497000ed98aABEWQ#&9I0@QE)
	//MAC = 00:0c:29:d3:d6:db
	//time=1533723497000
	//ver=1000

#if 0	
	char *url  = "https://api.yunjiangkj.com/appVilla/login?FKEY=cae3ec28b8fd371dc3738b514f2dfce0&TIMESTAMP=1533723497000&LOCKMAC=00:0c:29:d3:d6:db&VERSION=1000";
	pHttpClientOps httpClass = createHttpClientServer();
	if(httpClass == NULL)
	{
		return -1;
	}
	ret = httpClass->readurl(httpClass,url,buffer,sizeof(buffer)-1,10);
	if(ret > 0)
	{
		printf("%s\n",buffer);
	}
	printf("\nlen = %d\n",ret);
    destroyHttpClientServer(&httpClass);
#endif


	//
	//{"code":"101","msg":"获取成功","timestamp":1533962561468,
	//"data":{"COMMUNITYID":150,"UPGRADE":null,"LOCKNAME":"一期D一","LOCKID":1200}}
	
	YJbackground =  createYJbackgroundServer();
	
	ret  = YJbackground->login(YJbackground);
	if(ret  == 0)
	{
		printf("登录成功\n");
	}else {
		printf("登录失败\n");
	}
	
	pCardInfo cardPack = malloc(400*sizeof(CardInfo));
	int getsize;
	YJbackground->getICcard(YJbackground,1,cardPack,400,&getsize);
	printf("getsize = %d\n",getsize);
	printf("cardid:%s\n",cardPack->cardid);
	cardPack++;
	printf("cardid:%s\n",cardPack->cardid);

	int verId;
	char url[48] = {0};
	char md5[48];
	YJbackground->getUpdateRecord(YJbackground,&verId,url,sizeof(url),md5,sizeof(md5));


	
	YJbackground->upOpendoorRecord(YJbackground,"1234567",120,"S","T");
	char *jpgurl = "http://image.baidu.com/search/down?tn=download&ipn=dwnl&word=download&ie=utf8&fr=result&url=http%3A%2F%2Fpic.58pic.com%2F58pic%2F13%2F26%2F26%2F81k58PIC38E_1024.jpg&thumburl=http%3A%2F%2Fimg2.imgtn.bdimg.com%2Fit%2Fu%3D832007111%2C3382564677%26fm%3D27%26gp%3D0.jpg";
	pHttpClientOps HTTP = createHttpClientServer();

	//HTTP->download(jpgurl,"./test.jpg");
	char filemd5[48] = {0};
	getUtilsOps()->md5FileTransform("./file",filemd5);
	
	printf("md5:%s\n",filemd5);


	

	testSqlite();



	
	
	return 0;
}






















