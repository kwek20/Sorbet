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

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

int main()
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    char *password;
    
    connectDB(rc);
    //......
    closeDB(db);
}

int connectDB(int rc, sqlite3 *db)
{
    /* Open database */
    rc = sqlite3_open("sorbetDB.db", &db);
    
    if( rc )
    {
      fprintf(stderr, "Kan de database niet openen: %s\n", sqlite3_errmsg(db));
      exit(0);
    }
    else
    {
      fprintf(stderr, "Database succesvol geopend.\n");
    }
    
    //
}

int createDB()
{
    char *sql;
    
    /* Create SQL statement */
   sql = "CREATE TABLE USERS("  \
         "ID INT PRIMARY KEY     NOT NULL AUTOINCREMENT," \
         "NAME           TEXT    NOT NULL," \
         "PASSWORD       TEXT    NOT NULL);" \
         
         "CREATE TABLE PATHS("   \
         "ID INT PRIMARY KEY     NOT NULL,"  \
         "PATH        CHAR(30)   NOT NULL)";
}

int updateDB(int rc, sqlite3 *db, char *sql, char *zErrMsg)
{
    /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
   if( rc != SQLITE_OK )
   {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   else
   {
      fprintf(stdout, "Tabel gemaakt\n");
   }
}

int selectDB(int rc)
{
    
}

int closeDB(sqlite3 *db)
{
    sqlite3_close(db);
}

int hashPassword(char *password)
{
    //Crypto
}

// De onderstaande functies worden later verplaast

/*
 * Functies die voor de serverapplicatie bedoeld zijn.
 */

int createUser(int rc)
{
    updateDB(rc);
}

int removeUser(int rc)
{
    updateDB(rc);
}

int checkCredentials()
{
    // Password == selectDB();
}

/*
 * Functies die voor de clientapplicatie bedoeld zijn.
 */

int sendCredentials()
{
    
}

int writePasswordToLocalDB(int rc)
{
    updateDB(rc);
}