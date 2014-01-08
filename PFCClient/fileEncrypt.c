/***
* This implements the aes256cbc from *OpenSSL* crypto library to encrypt/decrypt a file
* @author dZONE
* DATE 04-29-2012 v1.0
***/

#include "pfc.h"


#define SIZE 1024
EVP_CIPHER_CTX *enc, *dec;

int aes_init(unsigned char* pwd, unsigned int pwd_len, unsigned char * salt){
        
    enc = malloc(sizeof(EVP_CIPHER_CTX));
    dec = malloc(sizeof(EVP_CIPHER_CTX));
    
        int i, rounds =5;                                         /* rounds */
        unsigned char key[32], iv[32];
        
        i = EVP_BytesToKey(EVP_aes_256_cbc(),EVP_sha1(),salt,pwd,pwd_len,rounds,key,iv);
        if(i != 32)
        {
                printf("\n Error,Incorrect key size generated:%d:\n",i);
                return -1;
        }
        
        EVP_CIPHER_CTX_init(enc);
        EVP_EncryptInit_ex(enc, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_CIPHER_CTX_init(dec);
        EVP_DecryptInit_ex(dec, EVP_aes_256_cbc(), NULL, key, iv);
        return 0;
}

int aes_encrypt(char* input, char* returnChar)         /* this function encryptes the  file:fd is passed as parameter */
{
        char outbuf[SIZE+AES_BLOCK_SIZE];        
        int inlen = strlen(input+1),outlen =0;
        
                
        if(!EVP_EncryptInit_ex(enc, NULL, NULL, NULL, NULL))                                 /* allows reusing of e for multiple cipher cycles */
        {
                perror("\n Error,ENCRYPR_INIT:");
                return 1;
        }
        //while((inlen = read(in,inbuf,SIZE)) > 0)
        //{
                if(!EVP_EncryptUpdate(enc,(unsigned char*) outbuf, &outlen,(unsigned char*) input,inlen))                 /* Update cipher text */
                {
                        perror("\n ERROR,ENCRYPR_UPDATE:");
                        return 1;
                }
                //if(write(out,outbuf,outlen) != outlen)        
                //{
                  //      perror("\n ERROR,Cant write encrypted bytes to outfile:");
                  //      return 1;
                //}        
        //}
        
//        if(!EVP_EncryptFinal_ex(enc, (unsigned char*) outbuf, &))                         /* updates the remaining bytes */
//        {
//                perror("\n ERROR,ENCRYPT_FINAL:");
//                return 1;
//        }
        
//        if(write(out,outbuf,flen) != flen)
//        {
//                perror("\n ERROR,Wriring final bytes of data:");
//                return 1;
//        }
        memcpy(returnChar, outbuf, outlen);
        printf("outbuf = %s\n", returnChar);
        return 0;    
}

int aes_decrypt(char* outbuf, char* returnChar){
//        int inlen=0,flen=0,outlen=0;
//        char inbuf[SIZE+AES_BLOCK_SIZE];                                /****** CHECK ???? ****/
//        char outbuf[SIZE+AES_BLOCK_SIZE];
//
//        if(!EVP_DecryptInit_ex(d, NULL, NULL, NULL, NULL)) 
//        {
//                perror("\n Eror in DECinit:");
//                return 1;
//        }    
//        
//        while((inlen = read(in,inbuf,SIZE)) >0)
//        {
//                if(!EVP_DecryptUpdate(d,(unsigned char*)outbuf, &outlen,(unsigned char*)inbuf,inlen)) 
//                {
//                        perror("\n Error,DECRYPT_UPDATE:");
//                        return 1;
//                }
//                if((write(out,outbuf,outlen)) != outlen)
//                {
//                        perror("\n ERROR,Writing dec bytes:");
//                        return 1;
//                }        
//        
//        }
//        
//        if(!EVP_DecryptFinal_ex(d,(unsigned char*)outbuf,&flen)) 
//        {
//                perror("\n Error,DECRYPT_FINAL:");
//                return 1;
//        }
//        if((write(out,outbuf,flen)) != flen)
//        {
//                perror("\n ERROR,Writng FINAL dec bytes:");
//                return 1;
//        }
        return 0;

}

//int aes_init(unsigned char* pwd, unsigned int pwd_len, unsigned char * salt, EVP_CIPHER_CTX *e_ctx){
//	int i, rounds =5; 					/* rounds */
//	unsigned char key[32], iv[32];
//	
//	i = EVP_BytesToKey(EVP_aes_256_cbc(),EVP_sha1(),salt,pwd,pwd_len,rounds,key,iv);
//	if(i != 32) {
//		printf("\n Error,Incorrect key size generated:%d:\n",i);
//		return STUK;
//	}
//	
//	EVP_CIPHER_CTX_init(e_ctx);
//	EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
//	return MOOI;
//}
//
//int encryptfile(char* pwd, char *salt, int in, int out){
//    EVP_CIPHER_CTX e;
//    
//    if(aes_init((unsigned char*)pwd, strlen(pwd),(unsigned char*) salt,&e)){
//        perror("\n Error, "
//                "Cant initialize key and IV:");
//	return STUK;
//    }
//    
//	char inbuf [SIZE];
//	char outbuf[SIZE+AES_BLOCK_SIZE];	
//	int inlen = 0,flen=0,outlen =0;
//	
//		
//	if(!EVP_EncryptInit_ex(&e, NULL, NULL, NULL, NULL)){
//		perror("\n Error,ENCRYPR_INIT:");
//		return PERFECT;
//	}
//	while((inlen = read(in,inbuf,SIZE)) > 0){
//		if(!EVP_EncryptUpdate(&e,(unsigned char*) outbuf, &outlen,(unsigned char*) inbuf,inlen)) { 		/* Update cipher text */
//			perror("\n ERROR,ENCRYPR_UPDATE:");
//			return PERFECT;
//		}
//		if(write(out,outbuf,outlen) != outlen){
//			perror("\n ERROR,Cant write encrypted bytes to outfile:");
//			return PERFECT;
//		}	
//	}
//	if(!EVP_EncryptFinal_ex(&e, (unsigned char*) outbuf, &flen)){
//		perror("\n ERROR,ENCRYPT_FINAL:");
//		return PERFECT;
//	}
//	if(write(out,outbuf,flen) != flen){
//		perror("\n ERROR,Wriring final bytes of data:");
//		return PERFECT;
//	}
//        
//        EVP_CIPHER_CTX_cleanup(&e);
//	return MOOI;	
//}
//
//int decrytfile(char* pwd, char *salt, int in, int out){
//    EVP_CIPHER_CTX d;
//    if(aes_init((unsigned char*)pwd, strlen(pwd),(unsigned char*) salt,&d)){
//        perror("\n Error, Cant initialize key and IV:");
//	return STUK;
//    }
//    
//	int inlen=0,flen=0,outlen=0;
//	char inbuf[SIZE+AES_BLOCK_SIZE];				/****** CHECK ???? ****/
//	char outbuf[SIZE+AES_BLOCK_SIZE];
//
//	if(!EVP_DecryptInit_ex(&d, NULL, NULL, NULL, NULL)) {
//		perror("\n Eror in DECinit:");
//		return PERFECT;
//	}    
//	
//	while((inlen = read(in,inbuf,SIZE)) >0)
//	{
//		if(!EVP_DecryptUpdate(&d,(unsigned char*)outbuf, &outlen,(unsigned char*)inbuf,inlen)) {
//			perror("\n Error,DECRYPT_UPDATE:");
//			return PERFECT;
//		}
//		if((write(out,outbuf,outlen)) != outlen){
//			perror("\n ERROR,Writing dec bytes:");
//			return PERFECT;
//		}	
//	}
//        
//	if(EVP_DecryptFinal_ex(&d,(unsigned char*)outbuf,&flen)<0) {
//		perror("\n Error,DECRYPT_FINAL:");
//		return PERFECT;
//	}
//	if((write(out,outbuf,flen)) != flen) {
//		perror("\n ERROR,Writng FINAL dec bytes:");
//		return PERFECT;
//	}
//        
//        EVP_CIPHER_CTX_cleanup(&d);
//        return MOOI;
//}

