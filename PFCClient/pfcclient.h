/* 
 * File:   pfcclient.h
 * Author: Bartjan Zondag
 *
 * Created on November 20, 2013, 11:26 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#define BUFFERGROOTE 4092

int pfcClient(char** argv);