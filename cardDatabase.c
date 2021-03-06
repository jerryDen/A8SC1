#include <unistd.h>   
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


#include "cardDatabase.h"
#include "common/sqlite3.h" 
#include "common/debugLog.h"


typedef struct  CardDataBaseServer{
	
	CardDataBaseOps ops;
	sqlite3 *db;
	pthread_mutex_t _mutex;
	char path[128];

}CardDataBaseServer,*pCardDataBaseServer;


static int rebuild(struct CardDataBaseOps * ops);
static int addData(struct CardDataBaseOps* ops,pCardInfo cardArray,int cardNum);
static int findCaidId(struct CardDataBaseOps* ops,char cardid[16],pCardInfo cardPack);
static int copyDataBase(const struct CardDataBaseOps* sDataBase,struct CardDataBaseOps* dDataBase);


static int  caeateCardDataTable(sqlite3 *db);

static CardDataBaseOps ops = {
		.rebuild = rebuild,
		.addData = addData,
		.findCaidId = findCaidId,
		.copyDataBase = copyDataBase
};

/*
** This function is used to load the contents of a database file on disk 
** into the "main" database of open database connection pInMemory, or
** to save the current contents of the database opened by pInMemory into
** a database file on disk. pInMemory is probably an in-memory database, 
** but this function will also work fine if it is not.
**
** Parameter zFilename points to a nul-terminated string containing the
** name of the database file on disk to load from or save to. If parameter
** isSave is non-zero, then the contents of the file zFilename are 
** overwritten with the contents of the database opened by pInMemory. If
** parameter isSave is zero, then the contents of the database opened by
** pInMemory are replaced by data loaded from the file zFilename.
**
** If the operation is successful, SQLITE_OK is returned. Otherwise, if
** an error occurs, an SQLite error code is returned.
*/
static int loadOrSaveDb(sqlite3 *pInMemory, const char *zFilename, int isSave){
  int rc;                   /* Function return code */
  sqlite3 *pFile;           /* Database connection opened on zFilename */
  sqlite3_backup *pBackup;  /* Backup object used to copy data */
  sqlite3 *pTo;             /* Database to copy to (pFile or pInMemory) */
  sqlite3 *pFrom;           /* Database to copy from (pFile or pInMemory) */

  /* Open the database file identified by zFilename. Exit early if this fails
  ** for any reason. */
  rc = sqlite3_open(zFilename, &pFile);
  if( rc==SQLITE_OK ){

    /* If this is a 'load' operation (isSave==0), then data is copied
    ** from the database file just opened to database pInMemory. 
    ** Otherwise, if this is a 'save' operation (isSave==1), then data
    ** is copied from pInMemory to pFile.  Set the variables pFrom and
    ** pTo accordingly. */
    pFrom = (isSave ? pInMemory : pFile);
    pTo   = (isSave ? pFile     : pInMemory);

    /* Set up the backup procedure to copy from the "main" database of 
    ** connection pFile to the main database of connection pInMemory.
    ** If something goes wrong, pBackup will be set to NULL and an error
    ** code and  message left in connection pTo.
    **
    ** If the backup object is successfully created, call backup_step()
    ** to copy data from pFile to pInMemory. Then call backup_finish()
    ** to release resources associated with the pBackup object.  If an
    ** error occurred, then  an error code and message will be left in
    ** connection pTo. If no error occurred, then the error code belonging
    ** to pTo is set to SQLITE_OK.
    */
    pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
    if( pBackup ){
      (void)sqlite3_backup_step(pBackup, -1);
      (void)sqlite3_backup_finish(pBackup);
    }
    rc = sqlite3_errcode(pTo);
  }

  /* Close the database connection opened on database file zFilename
  ** and return the result of this function. */
  (void)sqlite3_close(pFile);
  return rc;
}



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
		LOGE("fail to server is NULL\n");
		return -1;
	}
	if((access(server->path,F_OK)) != -1) { 
		remove(server->path);
	} 
	sqlite3_close(server->db);
	server->db = NULL;
	
	ret = sqlite3_open(server->path,&server->db);
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
		LOGE("fail to server is NULL\n");
		return -1;
	}

	for(i = 0; i< cardNum;i++){
		bzero(sql,sizeof(sql));
		sprintf(sql,"INSERT INTO 'CardData' VALUES(%d,'%s',%d,'%s',%d,%d,'%s',%d,'%s');",\
				cardArray->rid,cardArray->disabledate,cardArray->untitd,cardArray->state, \
				cardArray->cellid,cardArray->blockid,cardArray->type,cardArray->districtid,cardArray->cardid);
	//	LOGD("sql : %s  \ndb = %p\n",sql,server->db);
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
	return 0;
	
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
static int copyDataBase(const struct CardDataBaseOps* sDataBase,struct CardDataBaseOps* dDataBase)
{

		pCardDataBaseServer sServer = (pCardDataBaseServer)sDataBase;
		pCardDataBaseServer dServer = (pCardDataBaseServer)dDataBase;
		char systemCmd[128] = {0};
		int ret;
		if(sServer == NULL||dServer == NULL)
		{
			
			return -1;
		}
		return loadOrSaveDb(sServer->db,dServer->path,1);	
}

pCardDataBaseOps createCardDataBaseServer(const char *path)
{
		 int ret;
		 pCardDataBaseServer server = (pCardDataBaseServer)malloc(sizeof(CardDataBaseServer));
		 if(server == NULL)
		 {
		 	goto fail0;
		 }
		 memset(server,0,sizeof(CardDataBaseServer));

		 ret = sqlite3_open(path,&server->db);
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
		 strcpy(server->path,path);
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





















