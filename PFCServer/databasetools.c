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
   return MOOI;
}

int connectDB(){
   sqlite3 *db;
   int  rc;

   /* Open database */
   rc = sqlite3_open("sorbetDB.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }else{
      printf("Opened database successfully\n");
   }
   
   initDB(rc, db);
   printDB(rc, db);
   createUser(rc, db);
   printDB(rc, db);
   closeDB(db);
   
   return MOOI;
}

int initDB(int rc, sqlite3 *db)
{
    char *zErrMsg = 0;
    char *sql;
    /* Create SQL statement */
   sql = "CREATE TABLE USERS("  \
          "ID INTEGER PRIMARY KEY   AUTOINCREMENT," \
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
   
   /* Create SQL statement */
   sql = "INSERT INTO USERS (NAME,PASSWORD) "  \
         "VALUES ('Dave', 'PaSSwoRd'); " \
         "INSERT INTO USERS (NAME,PASSWORD) "  \
         "VALUES ('Brord', 'BrordWorD'); " ;

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stdout, "Records created successfully\n");
   }
   
   return MOOI;
}

int printDB(int rc, sqlite3 *db){
    
    char *sql = malloc(100);
    char *zErrMsg = 0;
    const char* data = "Callback function called";
    
    /* Create SQL statement */
   sql = "SELECT * from USERS";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stdout, "Operation done successfully\n");
   }
   
    return MOOI;
}

int selectDB(int rc, sqlite3 *db)
{
    return MOOI;
}

int closeDB(sqlite3 *db){
    sqlite3_close(db);
    return MOOI;
}

// De onderstaande functies worden later verplaast

/*
 * Functies die voor de serverapplicatie bedoeld zijn.
 */

int createUser(int rc, sqlite3 *db){
    //updateDB(rc);
    char *zErrMsg = 0;
    char *sql = malloc(100);
    char *tempUser;
    char *tempPassword;
    
    printf("enter username:\n");
    tempUser = getInput(50);
    printf("enter password:\n");
    tempPassword = getInput(50);
    
    strcpy(sql, "INSERT INTO USERS (NAME,PASSWORD) ");
    strcat (sql, "VALUES ('");
    strcat (sql, tempUser);
    strcat (sql, "', '");
    strcat (sql, tempPassword);
    strcat (sql, "');");
    
    /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      fprintf(stdout, "Records created successfully\n");
   }
   
   return MOOI;
}

int removeUser(int rc){
    //updateDB(rc);
    return MOOI;
}

int checkCredentials(){
    // Password == selectDB();
    return MOOI;
}

/*
 * Functies die voor de clientapplicatie bedoeld zijn.
 */

int sendCredentials(){
    return MOOI;
}

int writePasswordToLocalDB(int rc){
    //updateDB(rc);
    return MOOI;
}