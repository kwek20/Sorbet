/* 
 * File:   pfcclient.h
 * Author: Dave van Hooren
 *
 * Created on November 22, 2013, 11:04 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <memory.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

#define PORT 2200
#define MAX_CLI 10
