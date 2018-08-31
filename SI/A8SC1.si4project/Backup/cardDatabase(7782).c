#include <unistd.h>   
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


#include "cardDatabase.h"
#include "common/sqlite3.h" 
#include "common/debugLog.h"


#define DB_PATCH    ".carddb"
typedef struct  CardDataBaseServer{
	
	CardDataBaseOps ops;
	sqlite3 *db;

}CardDataBaseServer,*pCardDataBaseServer;


static int rebuild(struct CardDataBaseOps * ops);
static int addData(struct CardDataBaseOps* ops,pCardInfo cardArray,int cardNum);
static int findCaidId(struct CardDataBaseOps* ops,char cardid[16],pCardInfo cardPack);

static int  caeateCardDataTable(sqlite3 *db);

static CardDataBaseOps ops = {

		.rebuild = rebuild,
		.addData = addData,
		.findCaidId = findCaidId
};

static int tableHasBeenCreated(sqlite3 *db,char *tableName)
{
	char sql[1024] = {0};
	
	int azResult,nrow,ncolumn,ret;
	sprintf(sql,"SELECT * FROM '%s' ",tableName);
	ret  = sqlite3_get_table(db , sql , &azResult , &nrow , &ncolumn , NULL );
	if(ret == SQLITE_OK){
		return 1;
	}else {
		return 0;
	}

	
}
static int rebuild(struct CardDataBaseOps * ops)
{
	int ret;
	pCardDataBaseServer server = (pCardDataBaseServer)ops;
	if(server == NULL)
	{
		return -1;
	}
	if((access(DB_PATCH,F_OK)) != -1) { 
		remove(DB_PATCH);
		sqlite3_close(server->db);
		server->db = NULL;
	} 
	ret = sqlite3_open(DB_PATCH,&server->db);
	if( ret != SQLITE_OK )
	{
		LOGE("Can't open database: %s\n", sqlite3_errmsg(server->db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
		sqlite3_close(server->db);
		return -1;
    }
	ret = caeateCardDataTable(server->db);
	if(ret != SQLITE_OK)
	{
		 	LOGE("fail to sqlite3_exec\n");
			return -1;
	}
	return 0;
}
static int addData(struct CardDataBaseOps* ops,pCardInfo cardArray,int cardNum)
{
	char sql[1024] = {0};
	int i;
	int ret;
	pCardDataBaseServer server = (pCardDataBaseServer)ops;
	if(server == NULL)
	{
		return -1;
	}

	for(i = 0; i< cardNum;i++){
		bzero(sql,sizeof(sql));
		sprintf(sql,"INSERT INTO 'CardData' VALUES(%d,'%s',%d,'%s',%d,%d,'%s',%d,'%s');",\
				cardArray->rid,cardArray->disabledate,cardArray->untitd,cardArray->state, \
				cardArray->cellid,cardArray->blockid,cardArray->type,cardArray->districtid,cardArray->cardid);
		LOGD("sql : %s  \ndb = %p\n",sql,server->db);
		ret = sqlite3_exec(server->db,sql,NULL,NULL,NULL);
		if(ret != SQLITE_OK){

			LOGE("fail to INSERT INTO CardData\n");
		}
		cardArray++;
	}	
}


static int findCaidId(struct CardDataBaseOps* ops,char cardid[16],pCardInfo cardPack)
{
	char sql[1024] = {0};
	int nrow,ncolumn,i,ret;
	char **azResult;
	pCardDataBaseServer server = (pCardDataBaseServer)ops;
	if(server == NULL||cardPack == NULL)
	{
		return -1;
	}
	sprintf(sql,"select * from 'CardData' where CARDID = '%s' ",cardid);
	ret  = sqlite3_get_table(server->db , sql , &azResult , &nrow , &ncolumn , NULL );
	if(ret != SQLITE_OK || ncolumn != 9){
		return -1;
	}
	bzero(cardPack,sizeof(CardInfo));
	cardPack->rid = atoi(azResult[ncolumn++]);
	strcpy(cardPack->disabledate,azResult[ncolumn++]);
	cardPack->untitd = atoi(azResult[ncolumn++]);
	strcpy(cardPack->state,azResult[ncolumn++]);
	cardPack->cellid = atoi(azResult[ncolumn++]);
	cardPack->blockid = atoi(azResult[ncolumn++]);
	strcpy(cardPack->type,azResult[ncolumn++]);
	cardPack->districtid = atoi(azResult[ncolumn++]);
	strcpy(cardPack->cardid,azResult[ncolumn++]);
	
}


static int  caeateCardDataTable(sqlite3 *db)
{
		int ret;
		char *sql = " CREATE TABLE CardData(\
			 RID INTEGER PRIMARY KEY,\
			 DISABLEDATE VARCHAR(36),\
			 UNTITD   	 INTEGER,\
			 STATE 		 VARCHAR(3),\
			 CELLID 	 INTEGER,\
			 BLOCKID 	 INTEGER,\
			 TYPE        VARCHAR(3),\
			 DISTRICTID  INTEGER,\
			 CARDID      VARCHAR(16)\
			 );" ;

	//	 LOGD("%s\n",sql);
		 return sqlite3_exec(db,sql,NULL,NULL,NULL);
}
pCardDataBaseOps createCardDataBaseServer(void)
{
		 int ret;
		 pCardDataBaseServer server = (pCardDataBaseServer)malloc(sizeof(CardDataBaseServer));
		 if(server == NULL)
		 {
		 	goto fail0;
		 }
		 memset(server,0,sizeof(CardDataBaseServer));

		 ret = sqlite3_open(DB_PATCH,&server->db);
		 if( ret != SQLITE_OK )
		 {
			LOGE("Can't open database: %s\n", sqlite3_errmsg(server->db));//sqlite3_errmsg(db)用以获得数据库打开错误码的英文描述。
			sqlite3_close(server->db);
			goto fail1;
		 }
		 else LOGD("You have opened a sqlite3 database named user successfully!\n");
	 
		 /* 创建表 */
		 if(tableHasBeenCreated(server->db,"CardData") == 0){
				 LOGD("caeateCardDataTable\n");
				 ret = caeateCardDataTable(server->db);
				 if(ret != SQLITE_OK)
				 {
				 	LOGE("fail to sqlite3_exec\n");
					goto fail2;
				 }

		 }
		 server->ops = ops;
		 return server;
fail2:
	sqlite3_close(server->db);
fail1:
	free(server);
fail0:
	return NULL;

}







void destroyCardDataBaseServer(pCardDataBaseOps *ops)
{
	if(ops == NULL || *ops == NULL)
		return ;
	pCardDataBaseServer server = (pCardDataBaseServer)*ops;

	sqlite3_close(server->db);
	free(server);
	*ops = NULL;
	


}





















