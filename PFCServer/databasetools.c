/* 
 * File:   databasetools.c
 * Author: Dave van Hooren
 *
 * Deze file bevat code die door zowel de client als server gebruikt kan worden.
 * Deze file zal later worden samengevoegd met de utils.c file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

/*
 * Functies die in zowel de client- als serverapplicatie gelijk zijn.
 */

#include "pfc.h"

// ADDED PSEUDOCODE FOR LATER EDITING.

int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int connectDB(){
   sqlite3 *db;
   char *zErrMsg = 0;
   int  rc;
   char *sql;

   /* Open database */
   rc = sqlite3_open("test.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }else{
      printf("Opened database successfully\n");
   }

   /* Create SQL statement */
   sql = "CREATE TABLE USERS("  \
          "ID INT PRIMARY KEY     NOT NULL," \
          "NAME           TEXT    NOT NULL," \
          "PASSWORD       TEXT    NOT NULL);";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      printf("Table created successfully\n");
   }
   sqlite3_close(db);
   return 0;
}

int selectDB(int rc){
    return 0;
}

int closeDB(sqlite3 *db){
    //sqlite3_close(db);
    return 0;
}

int hashPassword(){
    char *password;
    //Crypto
    return 0;
}

// De onderstaande functies worden later verplaast

/*
 * Functies die voor de serverapplicatie bedoeld zijn.
 */

int createUser(int rc){
    //updateDB(rc);
    return 0;
}

int removeUser(int rc){
    //updateDB(rc);
    return 0;
}

int checkCredentials(){
    // Password == selectDB();
    return 0;
}

/*
 * Functies die voor de clientapplicatie bedoeld zijn.
 */

int sendCredentials(){
    return 0;
}

int writePasswordToLocalDB(int rc){
    //updateDB(rc);
    return 0;
}