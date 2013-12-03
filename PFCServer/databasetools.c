/* 
 * File:   databasetools.c
 * Author: Dave van Hooren
 *
 * Deze file bevat code die door zowel de client als server gebruikt kan worden.
 * Deze file zal later worden samengevoegd met de utils.c file.
 */

#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

/*
 * Functies die in zowel de client- als serverapplicatie gelijk zijn.
 */

#include "pfc.h"

// ADDED PSEUDOCODE FOR LATER EDITING.

int main()
{
    sqlite3 *db;
    char *ErrMsg = 0;
    int rec;
    
    char *password;
    
    connectDB(rec);
    //......
    closeDB(db);
}

int connectDB(int rec, sqlite3 *db)
{
    rec = sqlite3_open(&db);
}

int updateDB(int rec)
{
    rec = sqlite3_exec();
}

int selectDB(int rec)
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

int createUser(int rec)
{
    updateDB(rec);
}

int removeUser(int rec)
{
    updateDB(rec);
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

int writePasswordToLocalDB(int rec)
{
    updateDB(rec);
}