gcc pfcserver.c pfcSSL.c switch.c utils.c databasetools.c -lssl -lcrypto -lpthread -lsqlite3 -o server -Wall

