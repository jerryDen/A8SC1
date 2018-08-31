#include <stdio.h>
#include <string.h>
#include <unistd.h>   
#include <fcntl.h> 

#include "httpClient.h"
#include "md5.h"
#include "common/cJSON.h"
#include "common/Utils.h"
#include "YJbackground.h"
#include "localDevice.h"
#include "common/Utils.h"
#include "common/sqlite3.h" 
#include "cardDatabase.h"
#include "common/debugLog.h"



#define  _DEBUG_

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





void testCardDatabase(void)
{
	pCardDataBaseOps server = createCardDataBaseServer();
	
	char disabledate[36];
	int untitd;
	char state[3];
	int cellid;
	int rid;
	int blockid;
	char type[3];
	int districtid;  
	char cardid[16];
	CardInfo array[2] = {
		{"2018-08-18 20:00",112,"T",19154,9999,41210,"C",1111,"12345678"},
		{"2018-09-18 31:00",11,"T",129154,8888,91210,"C",12121,"123145678"}
	};
	CardInfo find;
//	server->rebuild(server);
	server->addData(server,array,2);
	server->findCaidId(server,"12345678",&find);
	LOGD("%s %d %s %d %d %d %s %d %s\n",find.disabledate,find.untitd,find.state,find.cellid,
										find.rid,find.blockid,find.type,find.districtid,find.cardid);
	
  	destroyCardDataBaseServer(&server);


	
}


void testSqlite(void)
{
	/*********************************************************************************
	 *		Copyright:	(C) 2017 zoulei
	 *					All rights reserved.
	 *
	 *		 Filename:	insert.c
	 *	  Description:	This file i
	 *
	 *		  Version:	1.0.0(2017年06月22日)
	 *		   Author:	zoulei <zoulei121@gmail.com>
	 *		ChangeLog:	1, Release initial version on "2017年06月22日 19时31分12秒"
	 *
	 ********************************************************************************/
	 
		 sqlite3 *db=NULL;
		 int len;
		 int i=0;
		 int nrow=0;
		 int ncolumn = 0;
		 char *zErrMsg =NULL;
		 char **azResult=NULL; //二维数组存放结果
		 /* 打开数据库 */
		 len = sqlite3_open("user",&db);
		 if( len )
		 {
			/*	fprintf函数格式化输出错误信息到指定的stderr文件流中	*/
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
			sqlite3_close(db);
			exit(1);
		 }
		 else printf("You have opened a sqlite3 database named user successfully!\n");
	 
		 /* 创建表 */
		 char *sql = " CREATE TABLE SensorData(\
			 ID INTEDER PRIMARY KEY,\
			 SensorID INTEGER,\
			 siteNum INTEGER,\
			 Time VARCHAR(12),\
			 SensorParameter REAL\
			 );" ;
	 	 
		  sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
#ifdef _DEBUG_
		//  printf("%s\n",zErrMsg);
		  sqlite3_free(zErrMsg);
#endif
		  /*插入数据  */
		  char*sql1 ="INSERT INTO 'SensorData'VALUES(NULL,1,2,201430506201,13.5);";
		  sqlite3_exec(db,sql1,NULL,NULL,&zErrMsg);
		  char*sql2 ="INSERT INTO 'SensorData'VALUES(NULL,3,4,201530506302,14.5);";
		  sqlite3_exec(db,sql2,NULL,NULL,&zErrMsg);
		  char*sql3 ="INSERT INTO 'SensorData'VALUES(NULL,5,6,201630506413,18.6);";
		  sqlite3_exec(db,sql3,NULL,NULL,&zErrMsg);
	 
		  /* 查询数据 */
		  sql="select *from SensorData";
		  sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );
		  printf("nrow=%d ncolumn=%d\n",nrow,ncolumn);
		  printf("the result is:\n");
		  for(i=0;i<(nrow+1)*ncolumn;i++)
			{
			  printf("azResult[%d]=%s\n",i,azResult[i]);
			}
	 
		 /* 删除某个特定的数据 */
		  sql="delete from SensorData where SensorID = 1 ;";
		  sqlite3_exec( db , sql , NULL , NULL , &zErrMsg );
#ifdef _DEBUG_
		  printf("zErrMsg = %s \n", zErrMsg);
		  sqlite3_free(zErrMsg);
#endif
	 
		  /* 查询删除后的数据 */
		  sql = "SELECT * FROM SensorData ";
		  sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );
		  printf( "row:%d column=%d\n " , nrow , ncolumn );
		  printf( "After deleting , the result is : \n" );
		  for( i=0 ; i<( nrow + 1 ) * ncolumn ; i++ )
		  {
				printf( "azResult[%d] = %s\n", i , azResult[i] );
		  }
		  sqlite3_free_table(azResult);
#ifdef _DEBUG_
	   printf("zErrMsg = %s \n", zErrMsg);
	   sqlite3_free(zErrMsg);
#endif
	 
		  sqlite3_close(db);
		  return 0;

}

void testSqlite1(void)
{
	/*********************************************************************************
	 *		Copyright:	(C) 2017 zoulei
	 *					All rights reserved.
	 *
	 *		 Filename:	insert.c
	 *	  Description:	This file i
	 *
	 *		  Version:	1.0.0(2017年06月22日)
	 *		   Author:	zoulei <zoulei121@gmail.com>
	 *		ChangeLog:	1, Release initial version on "2017年06月22日 19时31分12秒"
	 *
	 ********************************************************************************/
	 #define FILE_PATH  "./card"
		 sqlite3 *db=NULL;
		 int len;
		 int ret;
		 int i=0;
		 int nrow=0;
		 int ncolumn = 0;
		 char *zErrMsg =NULL;
		 char **azResult=NULL; //二维数组存放结果
		 /* 打开数据库 */
		 if((access(FILE_PATH,F_OK))!=-1)   
		  {   
		        printf("文件 test.c 存在.\n");   
				 remove(FILE_PATH);
		  } 
		 
		 len = sqlite3_open(FILE_PATH,&db);
		 if( len )
		 {
			/*	fprintf函数格式化输出错误信息到指定的stderr文件流中	*/
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
			sqlite3_close(db);
			exit(1);
		 }
		 else printf("You have opened a sqlite3 database named user successfully!\n");
	 	/*
	 		int rid;                INTEDER
			char disabledate[36];   VARCHAR(36)
			int untitd;             INTEDER
			char state[3];          VARCHAR(3)
			int cellid;             INTEDER
			int blockid;            INTEDER
			char type[3];           VARCHAR(3)
			int districtid;         INTEDER
			char cardid[16];        VARCHAR(16)
		 */
		 /* 创建表 */
		
		 char *sql = " CREATE TABLE CardData(\
			 RID INTEDER PRIMARY KEY,\
			 DISABLEDATE VARCHAR(36),\
			 UNTITD   	 INTEGER,\
			 STATE 		 VARCHAR(3),\
			 CELLID 	 INTEGER,\
			 BLOCKID 	 INTEDER,\
			 TYPE        VARCHAR(3),\
			 DISTRICTID  INTEDER,\
			 CARDID      VARCHAR(16)\
			 );" ;
			 char *sql1 = " CREATE TABLE CardData(\
			 RID INTEGER PRIMARY KEY,\
			 DISABLEDATE VARCHAR(36)\
			 );" ;

		 ret =  sqlite3_exec(db,sql1,NULL,NULL,&zErrMsg);
		 printf("ret = %d\n",ret);
#ifdef _DEBUG_
		//printf("%s\n",zErrMsg);
		  sqlite3_free(zErrMsg);
#endif
		  /*插入数据  */
		  char*sqladd1 ="INSERT INTO 'CardData' VALUES(2203,'aaaa');";
		  
		  ret  = sqlite3_exec(db,sqladd1,NULL,NULL,&zErrMsg);
		  printf("ret = %d\n",ret);
		
	 
		  /* 查询数据 */
		  sql="select *from CardData";
		  sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );
		  printf("nrow=%d ncolumn=%d\n",nrow,ncolumn);
		  printf("the result is:\n");
		  for(i=0;i<(nrow+1)*ncolumn;i++)
		  {
			  printf("azResult[%d]=%s\n",i,azResult[i]);
		  }
	 
		 /* 删除某个特定的数据 */
		  sql="delete from CardData where DISABLEDATE = 'aaaa' ;";
		  sqlite3_exec( db , sql , NULL , NULL , &zErrMsg );
#ifdef _DEBUG_
		//  printf("zErrMsg = %s \n", zErrMsg);
		  sqlite3_free(zErrMsg);
#endif
	 
		  /* 查询删除后的数据 */
		  sql = "SELECT * FROM CardData ";
		  sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );
		  printf( "row:%d column=%d\n " , nrow , ncolumn );
		  printf( "After deleting , the result is : \n" );
		  for( i=0 ; i<( nrow + 1 ) * ncolumn ; i++ )
		  {
				printf( "azResult[%d] = %s\n", i , azResult[i] );
		  }
		  sqlite3_free_table(azResult);
#ifdef _DEBUG_
	   printf("zErrMsg = %s \n", zErrMsg);
	   sqlite3_free(zErrMsg);
#endif
	 
		  sqlite3_close(db);
		  return 0;

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

#if 1
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


#endif


	testCardDatabase();


	
	
	return 0;
}






















