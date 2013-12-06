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

sqlite3 *db;

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
   int  rc;

   /* Open database */
   rc = sqlite3_open("sorbetDB.db", &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }else{
      printf("Opened database successfully\n");
   }
   
   initDB(rc);
   return MOOI;
}

int initDB(){
    char *zErrMsg = 0;
    char *sql;
    int rc;
    
    /* Create SQL statement */
   sql = "CREATE TABLE IF NOT EXISTS USERS("  \
          "ID INTEGER PRIMARY KEY   AUTOINCREMENT," \
          "NAME           TEXT    NOT NULL UNIQUE," \
          "PASSWORD       TEXT    NOT NULL);";

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      printf("Table created successfully\n");
   }
   return MOOI;
}

sqlite3_stmt* selectQuery(char *query){
  sqlite3_stmt *res;
  const char *tail;
  
  if(sqlite3_prepare_v2(db, query, strlen(query), &res, &tail) != SQLITE_OK){
    //sqlite3_close(db);
    return NULL;
  }
  
  return res;
}

void printRes(sqlite3_stmt *res){
  int count = 0, result, i;
  printf("Reading data...\n");
  for ( ;; ){
      result = sqlite3_step(res);
      if (result == SQLITE_ROW){
          for (i=0; i<sqlite3_column_count(res); i++){
             printf("%s\n", sqlite3_column_text(res, i));
          }
          puts("");
        count++;
      } else {
          break;
      }
  }
  printf("Rows count: %d\n", count);
}

int closeDB(){
    printf("sqlresult: %i\n", sqlite3_close_v2(db));
    return MOOI;
}

/*
 * Functies die voor de serverapplicatie bedoeld zijn.
 */
int createUser(char **args, int amount){
    //updateDB(rc);
    char *zErrMsg = 0;
    char *sql = malloc(100);
    int rc;
    
    if (amount <= 2){
        puts("Create user failed!");
        return STUK;
    }
    
    strcpy(sql, "INSERT INTO USERS (NAME,PASSWORD) VALUES ('");
    strcat (sql, args[1]);
    strcat (sql, "', '");
    strcat (sql, args[2]);
    strcat (sql, "');");
    
    /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
       puts("Create user failed!");
       return STUK;
   } 
   printf("Sucessfully created user %s\n", args[1]);
   
   return MOOI;
}

int userExists(char* name){
    char *sql = malloc(100);
    int result = MOOI;
    
    strcpy(sql, "SELECT * FROM USERS WHERE NAME = '");
    strcat(sql, name);
    strcat(sql, "';");
    
    sqlite3_stmt* res = selectQuery(sql);
    if (res == NULL) {
        result = STUK;
    } else if(sqlite3_step(res) != SQLITE_ROW){
        result = STUK;
    }
    sqlite3_finalize(res);
    return result;
}

char* getPassWord(char *name){
    char *password = malloc(50);
    strcpy(password, "");
    
    if (userExists(name) == MOOI){
        puts("User exists");
        char *sql = malloc(100);
        strcpy(sql, "SELECT PASSWORD FROM USERS WHERE NAME = '");
        strcat(sql, name);
        strcat(sql, "';");
        sqlite3_stmt* res = selectQuery(sql);
        if (res != NULL) {
            printf("0\n");
            if (sqlite3_step(res) == SQLITE_ROW){
                strcpy(password, (char *)sqlite3_column_text(res, 0));
            }
        }
        sqlite3_finalize(res);
    }
    //shouldnt happen
    return password;
}

int removeUser(char **args, int amount){
    //updateDB(rc);
    if (amount < 2){
        return STUK;
    }
    
    //remove user args[2]
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