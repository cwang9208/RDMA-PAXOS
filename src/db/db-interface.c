#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <db.h>
#include "../include/db/db-interface.h"
#include "../include/util/debug.h"

const char* db_dir="./.db";

u_int32_t pagesize = 32 * 1024;
u_int cachesize = 32 * 1024 * 1024;

struct db_t{
    DB* bdb_ptr;
};

db* initialize_db(const char* db_name,uint32_t flag){
    db* db_ptr=NULL;
    DB* b_db;
    int ret;
    /* Initialize the DB handle */
    if((ret = db_create(&b_db,NULL,flag))!=0){
        err_log("DB : %s.\n",db_strerror(ret));
        goto db_init_return;
    }
    
    if((ret = b_db->set_pagesize(b_db,pagesize))!=0){
        goto db_init_return;
    }
    if((ret = b_db->set_cachesize(b_db, 0, cachesize, 1))!=0){
        goto db_init_return;
    }

    if((ret = b_db->open(b_db,NULL,db_name,NULL,DB_RECNO,DB_THREAD|DB_CREATE,0))!=0){
        //b_db->err(b_db,ret,"%s","test.db");
        goto db_init_return;
    }
    db_ptr = (db*)(malloc(sizeof(db)));
    db_ptr->bdb_ptr = b_db;

db_init_return:
    if(db_ptr!=NULL){
        //debug_log("DB Initialization Finished\n");
        ;
    }
    return db_ptr;
}

void close_db(db* db_p,uint32_t mode){
    if(db_p!=NULL){
        if(db_p->bdb_ptr!=NULL){
            db_p->bdb_ptr->close(db_p->bdb_ptr,mode);
            db_p->bdb_ptr=NULL;
        }
        free(db_p);
        db_p = NULL;
    }
    return;
}

int store_record(db* db_p,size_t data_size,void* data){
    int ret = 1;
    if((NULL==db_p)||(NULL==db_p->bdb_ptr)){
        if(db_p == NULL){
          err_log("DB store_record : db_p is null.\n");
        } else{
          err_log("DB store_recor : db_p->bdb_ptr is null.\n");
        }
        goto db_store_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key,db_data;
    memset(&db_data,0,sizeof(db_data));
    db_data.data = data;
    db_data.size = data_size;

    memset(&key,0,sizeof(key));
    db_recno_t recno;
    key.data = &recno;
    key.ulen = sizeof(recno);
    key.flags = DB_DBT_USERMEM;
    if ((ret=b_db->put(b_db,NULL,&key,&db_data,DB_AUTO_COMMIT|DB_APPEND))==0){
        //debug_log("db : %ld record stored. \n",*(uint64_t*)key_data);
        //b_db->sync(b_db,0);
        //debug_log("new record number is %lu\n", (u_long)recno);
    }
    else{
        err_log("DB : %s.\n",db_strerror(ret));
        //debug_log("db : can not save record %ld from database.\n",*(uint64_t*)key_data);
        //b_db->err(b_db,ret,"DB->Put");
    }
db_store_return:
    return ret;
}

int retrieve_record(db* db_p,size_t key_size,void* key_data,size_t* data_size,void** data){
    int ret=1;
    if(NULL==db_p || NULL==db_p->bdb_ptr){
        if(db_p == NULL){
          err_log("DB retrieve_record : db_p is null.\n");
        } else{
          err_log("DB retrieve_record : db_p->bdb_ptr is null.\n");
        }
        goto db_retrieve_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key,db_data;
    memset(&key,0,sizeof(key));
    memset(&db_data,0,sizeof(db_data));
    key.data = key_data;
    key.size = key_size;
    db_data.flags = DB_DBT_MALLOC;
    if((ret=b_db->get(b_db,NULL,&key,&db_data,0))==0){
        //debug_log("db : get record %ld from database.\n",*(uint64_t*)key_data);
    }else{
        //debug_log("db : can not get record %ld from database.\n",*(uint64_t*)key_data);
        err_log("DB : %s.\n",db_strerror(ret));
        //b_db->err(b_db,ret,"DB->Get");
        goto db_retrieve_return;
    }
    if(!db_data.size){
        goto db_retrieve_return;
    }
    *data = db_data.data;
    *data_size = db_data.size;
db_retrieve_return:
    return ret;
}


int delete_record(db* db_p,size_t key_size,void* key_data){
    int ret=1;
    if(NULL==db_p || NULL==db_p->bdb_ptr){
        goto db_delete_return;
    }
    DB* b_db = db_p->bdb_ptr;
    DBT key;
    memset(&key,0,sizeof(key));
    key.data = key_data;
    key.size = key_size;
    ret=b_db->del(b_db,NULL,&key,0);
    if(ret!=0){
        //b_db->err(b_db,ret,"DB->Delete");
        err_log("DB : %s.\n",db_strerror(ret));
    }
db_delete_return:
    return ret;
}
