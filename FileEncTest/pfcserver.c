/***
* This implements the aes256cbc from *OpenSSL* crypto library to encrypt/decrypt a file
* @author dZONE
* DATE 04-29-2012 v1.0
***/

# include <stdio.h>
# include <stdlib.h>
# include <openssl/evp.h>
# include <openssl/aes.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
# include <string.h>
# include <assert.h>
# include <error.h>

# define SIZE 32

int initEncrypt();
char* getRandom(int size);
char* newPwd(int pwdSize);

EVP_CIPHER_CTX *e;  
EVP_CIPHER_CTX *d;
int aes_init(unsigned char* pwd, unsigned int pwd_len, unsigned char * salt)
{
        int i, rounds =5;                                         /* rounds */
        unsigned char key[32], iv[32];
        
        i = EVP_BytesToKey(EVP_aes_256_cbc(),EVP_sha1(),salt,pwd,pwd_len,rounds,key,iv);
        if(i != 32)
        {
                printf("\n Error,Incorrect key size generated:%d:\n",i);
                return -1;
        }
        
        e = malloc(sizeof(EVP_CIPHER_CTX));
        d = malloc(sizeof(EVP_CIPHER_CTX));
        
        EVP_CIPHER_CTX_init(e);
        EVP_EncryptInit_ex(e, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_CIPHER_CTX_init(d);
        EVP_DecryptInit_ex(d, EVP_aes_256_cbc(), NULL, key, iv);
        return 0;
}

int aes_encrypt(int in,int out )         /* this function encryptes the file:fd is passed as parameter */
{
        char inbuf [SIZE];
        char outbuf[SIZE+AES_BLOCK_SIZE];        
        int inlen = 0,flen=0,outlen =0;
        
                
        if(!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL))                                 /* allows reusing of e for multiple cipher cycles */
        {
                perror("\n Error,ENCRYPR_INIT:");
                return 1;
        }
        while((inlen = read(in,inbuf,SIZE)) > 0)
        {
                if(!EVP_EncryptUpdate(e,(unsigned char*) outbuf, &outlen,(unsigned char*) inbuf,inlen))                 /* Update cipher text */
                {
                        perror("\n ERROR,ENCRYPR_UPDATE:");
                        return 1;
                }
                if(write(out,outbuf,outlen) != outlen)        
                {
                        perror("\n ERROR,Cant write encrypted bytes to outfile:");
                        return 1;
                }        
        }
        if(!EVP_EncryptFinal_ex(e, (unsigned char*) outbuf, &flen))                         /* updates the remaining bytes */
        {
                perror("\n ERROR,ENCRYPT_FINAL:");
                return 1;
        }
        if(write(out,outbuf,flen) != flen)
        {
                perror("\n ERROR,Wriring final bytes of data:");
                return 1;
        }
        return 0;        
}

int aes_decrypt(int in,int out)
{
        int inlen=0,flen=0,outlen=0, totlen=0;
        char inbuf[SIZE+AES_BLOCK_SIZE];                                /****** CHECK ???? ****/
        char outbuf[SIZE+AES_BLOCK_SIZE];

        if(!EVP_DecryptInit_ex(d, NULL, NULL, NULL, NULL))
        {
                perror("\n Eror in DECinit:");
                return 1;
        }
        
        while((inlen = read(in,inbuf,SIZE)) >0)
        {
                if(!EVP_DecryptUpdate(d,(unsigned char*)outbuf, &outlen,(unsigned char*)inbuf,inlen))
                {
                        perror("\n Error,DECRYPT_UPDATE:");
                        return 1;
                }
                if((write(out,outbuf,outlen)) != outlen)
                {
                        perror("\n ERROR,Writing dec bytes:");
                        return 1;
                }        
                totlen+=inlen;
        }
        
        if(!EVP_DecryptFinal_ex(d,(unsigned char*)outbuf,&flen))
        {
                perror("\n Error,DECRYPT_FINAL:");
                return 1;
        }
        if((write(out,outbuf,flen)) != flen)
        {
                perror("\n ERROR,Writng FINAL dec bytes:");
                return 1;
        }
        return totlen;
}

char* getRandom(int size){
    char *buff = malloc(size+1);
    strcpy(buff, "");
    
    int randomfd, readSize;
    
    if((randomfd = open("/dev/urandom", O_RDONLY)) == -1){
        perror("Error,Opening /dev/random");
        return buff;
    } else {
        if((readSize = read(randomfd,buff,size)) == -1) {
            perror("Error,reading from /dev/random");
            return buff;
        }
        close(randomfd);
    }
    return buff;    
}

char* newPwd(int pwdSize){
    int keyfd;
    char* pwd = malloc(pwdSize+1);
    
    if((keyfd = open("secret.key",O_RDWR|O_CREAT, 0777)) == -1){
        perror("\n Could not create secret.key");
        return "";
    } else {
        pwd = getRandom(pwdSize);
        keyfd = write(keyfd, pwd, pwdSize);
        close(keyfd);
    }
    return pwd;
}

int initEncrypt(){
    int readSize, keyfd, pwdSize = 256;
    char buffer[pwdSize+1];
    char *salt = "SoRbEt";
    char *pwd = malloc(pwdSize+1);
    strcpy(pwd, "");
    
    if((keyfd = open("secret.key",O_RDWR)) == -1){
        pwd = newPwd(pwdSize);
    } else {
        while ((readSize = read(keyfd, buffer, pwdSize)) > 0){
            strcat(pwd, buffer);
        }
        
        if (strlen(buffer) < pwdSize){
            pwd = newPwd(pwdSize);
        }
        close(keyfd);
    }
                                      
    unsigned int pwd_len = strlen(pwd);

    if(aes_init((unsigned char*)pwd,pwd_len,(unsigned char*) salt)){                /* Generating Key and IV and initializing the EVP struct */
        perror("\n Error, Cant initialize key and IV");
        return -1;
    }
    
    return 0;
}

int main(int argc,char **argv)
{
        if(argc != 2){
                perror("\n Error:\nCorrect Usage: Enter Password to be used");
                exit(-1);
        }

                                              /* The EVP structure which keeps track of all crypt operations see evp.h for details */
        int in, out, dec;                                        /* fd for input and output files and random dev*/

        if((in = open("plain.txt",O_RDONLY)) == -1) {
                perror("\n Error,Opening file for reading::");
                exit(-1);
        }
        
        initEncrypt();

        if((out = open("encrypt.txt",O_RDWR|O_CREAT,0400|0200)) == -1){
                perror("\n Error,Opening the file to be written::");
                exit(-1);
        }       
        
        if((dec = open("dec22.txt",O_RDWR|O_CREAT,0400|0200)) == -1){
                perror("\n ERROR,Opening the file to write decrypted bytes::");
                exit(-1);
        }
        
        int retb;
        if((retb = aes_encrypt(in,out)) < 0){
                perror("\n ERROR,ENCRYPTING:");
                exit(-1);
        }
        //} else {
                if((lseek(out,0,SEEK_SET)) != 0){
                        perror("\n ERROR,lseek:");
                        exit(-1);
                }
                if(aes_decrypt(out,dec)){
                        perror("\n ERROR,DECRYPTING DATA:");
                        exit(-1);
                }
        //}

        close(in);
        close(out);
        close(dec);
        return 0;
}

