/* 
 * File:   databasetools.c
 * Author: Dave van Hooren
 *
 * Deze file bevat code die door zowel de client als server gebruikt kan worden.
 * Deze file zal later worden samengevoegd met de utils.c file.
 */

/*
 * Functies die in zowel de client- als serverapplicatie gelijk zijn.
 */

#include "pfc.h"

int connectDB();
int updateDB();
int closeDB();

int hashPassword();

// De onderstaande functies worden later verplaast

/*
 * Functies die voor de serverapplicatie bedoeld zijn.
 */

int createUser();
int removeUser();
int checkCredentials();

/*
 * Functies die voor de clientapplicatie bedoeld zijn.
 */

int sendCredentials();
int writePasswordToLocalDB();