/* 
 * File:   switch.c
 * Author: Bartjan Zondag & Kevin Rosendaal
 *
 */

/*
 * Switch voor het maken van een structuur voor status-codes
 */

#include "pfcclient.h"

int switchResult(char statusCode){
    switch(statusCode) {
        case OK:     recv_ok(); return 0;
        case EOF:     end_of_file(); return 0;
        case CR:     FileNaarServer(); return 0; 
    }
}