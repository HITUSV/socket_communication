//
// Created by wumode on 2019/9/13.
//

#include <openssl/md5.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>
#define KEY_LENGTH  2048               // 密钥长度
#define PUB_KEY_FILE "pubkey.pem"    // 公钥路径
#define PRI_KEY_FILE "prikey.pem"    // 私钥路径

int md5_encrypt(const void* data, size_t len, unsigned char* md5)
{
    if (data == NULL || len <= 0 || md5 == NULL) {
        printf("Input param invalid!\n");
        return -1;
    }
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data, len);
    MD5_Final(md5, &ctx);
    return 0;
}

int aes_encrypt(const unsigned char* data, const size_t len, const unsigned char* key, unsigned char* ivec,
        unsigned char* encrypt_data)
{
    AES_KEY aes_key;
    memset(&aes_key, 0x00, sizeof(AES_KEY));
    if (AES_set_encrypt_key(key, 128, &aes_key) < 0)
    {
        fprintf(stderr, "Unable to set encryption key in AES...\n");
        return -1;
    }
    AES_cbc_encrypt(data, encrypt_data, len, &aes_key, ivec, AES_ENCRYPT);
    return 0;
}

int aes_decrypt(const unsigned char* encrypt_data, const size_t len, const unsigned char* key, unsigned char* ivec,
        unsigned char* decrypt_data)
{
    AES_KEY aes_key;
    memset(&aes_key, 0x00, sizeof(AES_KEY));
    if (AES_set_decrypt_key(key, 128, &aes_key) < 0)
    {
        fprintf(stderr, "Unable to set decryption key in AES...\n");
        return -1;
    }
    AES_cbc_encrypt(encrypt_data, decrypt_data, len, &aes_key, ivec, AES_DECRYPT);
    return 0;
}

// 函数方法生成密钥对
void generateRSAKey(std::string strKey[2])
{
    // 公私密钥对
    size_t pri_len;
    size_t pub_len;
    char *pri_key = NULL;
    char *pub_key = NULL;

    // 生成密钥对
    RSA *keypair = RSA_generate_key(KEY_LENGTH, RSA_F4, NULL, NULL);

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    // 获取长度
    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    // 密钥对读取到字符串
    pri_key = (char *)malloc(pri_len + 1);
    pub_key = (char *)malloc(pub_len + 1);

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    // 存储密钥对
    strKey[0] = pub_key;
    strKey[1] = pri_key;

    // 存储到磁盘（这种方式存储的是begin rsa public key/ begin rsa private key开头的）
    FILE *pubFile = fopen(PUB_KEY_FILE, "w");
    if (pubFile == NULL)
    {
        //assert(false);
        return;
    }
    fputs(pub_key, pubFile);
    fclose(pubFile);

    FILE *priFile = fopen(PRI_KEY_FILE, "w");
    if (priFile == NULL)
    {
        //assert(false);
        return;
    }
    fputs(pri_key, priFile);
    fclose(priFile);

    // 内存释放
    RSA_free(keypair);
    BIO_free_all(pub);
    BIO_free_all(pri);

    free(pri_key);
    free(pub_key);
}

int main(int argc, char* argv[]){
    unsigned char data[50] = "abc";
    unsigned char key[3];
    key[0] = 'a';
    key[1] = 'b';
    key[2] = 'c';
    unsigned char iv[17] = "0123456789012345";
    uint8_t res[2048];
    unsigned char md5[2048];
    md5_encrypt(data, 50, md5);
    aes_encrypt(data, 5, key, iv, res);
    unsigned char* i = res;
    while (*i!='\0'){
        std::cout<<std::hex<<(int)*i<<" ";
        i++;
    }
    aes_decrypt(res, 16, key, iv, data);
    std::cout<<data<<std::endl;

}