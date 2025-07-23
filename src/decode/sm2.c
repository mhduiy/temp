// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sm2.h"

#include "common/log.h"

#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/decoder.h>
#include <openssl/encoder.h>

// 添加OpenSSL错误打印函数
static void print_openssl_errors() {
    unsigned long err;
    while ((err = ERR_get_error()) != 0) {
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        LOG(LOG_WARNING, "OpenSSL Error: %s", err_buf);
    }
}

int dp_sm2_key_generate(EVP_PKEY **key)
{
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *newKey = NULL;
    int ret = -1;

    if (key == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    // 初始化OpenSSL
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);

    // 使用OpenSSL 3.0的新API生成SM2密钥
    pctx = EVP_PKEY_CTX_new_from_name(NULL, "SM2", NULL);
    if (pctx == NULL) {
        LOG(LOG_WARNING, "failed to create SM2 PKEY context.");
        goto end;
    }

    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        LOG(LOG_WARNING, "failed to init SM2 keygen.");
        goto end;
    }

    if (EVP_PKEY_keygen(pctx, &newKey) <= 0) {
        LOG(LOG_WARNING, "failed to generate SM2 key pair.");
        goto end;
    }

    *key = newKey;
    ret = 0;

end:
    if (pctx != NULL) {
        EVP_PKEY_CTX_free(pctx);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_sm2_get_public_key(EVP_PKEY *key, unsigned char **publicKey)
{
    BIO *bio = NULL;
    EC_KEY *ecKey = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || publicKey == NULL) {
        LOG(LOG_WARNING, "invalid param.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (PEM_write_bio_EC_PUBKEY(bio, ecKey) <= 0) {
        LOG(LOG_WARNING, "failed to call PEM_write_bio_EC_PUBKEY.");
        goto end;
    }

    size_t len = BIO_pending(bio);
    if (len == 0) {
        LOG(LOG_WARNING, "failed to call BIO_pending.");
        goto end;
    }
    pkey = (unsigned char *)malloc(len + 1);
    if (pkey == NULL) {
        LOG(LOG_WARNING, "failed to call malloc.");
        goto end;
    }
    if (BIO_read(bio, pkey, len) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_read.");
        goto end;
    }
    pkey[len] = '\0';

    *publicKey = pkey;
    LOG(LOG_WARNING, "public key:%s", pkey);
    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && pkey != NULL) {
        free(pkey);
    }
    return ret;
}

int dp_sm2_get_private_key(EVP_PKEY *key, unsigned char **privateKey)
{
    BIO *bio = NULL;
    EC_KEY *ecKey = NULL; // not need free
    unsigned char *pkey = NULL;
    int ret = -1;

    if (key == NULL || privateKey == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    ecKey = EVP_PKEY_get0_EC_KEY(key);
    if (ecKey == NULL) {
        LOG(LOG_WARNING, "failed to call EVP_PKEY_get0_EC_KEY.");
        goto end;
    }

    if (PEM_write_bio_ECPrivateKey(bio, ecKey, NULL, NULL, 0, NULL, NULL) <= 0) {
        LOG(LOG_WARNING, "failed to call PEM_write_bio_ECPrivateKey.");
        goto end;
    }

    size_t len = BIO_pending(bio);
    if (len == 0) {
        LOG(LOG_WARNING, "failed to call BIO_pending.");
        goto end;
    }
    pkey = (unsigned char *)malloc(len + 1);
    if (pkey == NULL) {
        LOG(LOG_WARNING, "failed to call malloc.");
        goto end;
    }
    if (BIO_read(bio, pkey, len) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_read.");
        goto end;
    }
    pkey[len] = '\0';

    *privateKey = pkey;

    ret = 0;
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (ret != 0 && pkey != NULL) {
        free(pkey);
    }
    return ret;
}

int dp_sm2_public_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    EVP_PKEY *newKey = NULL;
    OSSL_DECODER_CTX *decoder_ctx = NULL;
    int ret = -1;

    if (keyStr == NULL || key == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }
    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    if (BIO_puts(bio, (char *)keyStr) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_puts.");
        goto end;
    }

    ERR_clear_error();
    
    // 首先尝试传统的PEM_read_bio_PUBKEY
    newKey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (newKey != NULL) {
        LOG(LOG_WARNING, "Successfully read public key with PEM_read_bio_PUBKEY");
        goto check_key;
    }
    
    LOG(LOG_WARNING, "Failed to read with PEM_read_bio_PUBKEY");
    print_openssl_errors();
    
    // 重置BIO位置
    BIO_reset(bio);
    
    // 尝试使用现代的OSSL_DECODER API，指定SM2
    decoder_ctx = OSSL_DECODER_CTX_new_for_pkey(&newKey, "PEM", NULL, "SM2", 
                                                EVP_PKEY_PUBLIC_KEY, NULL, NULL);
    if (decoder_ctx == NULL) {
        LOG(LOG_WARNING, "Failed to create SM2 decoder context");
        goto end;
    }
    
    // 尝试解码公钥
    if (OSSL_DECODER_from_bio(decoder_ctx, bio) <= 0) {
        LOG(LOG_WARNING, "Failed to decode public key with OSSL_DECODER");
        goto end;
    }
    
    
check_key:    
    // 使用EVP_PKEY_is_a检查密钥类型
    if (EVP_PKEY_is_a(newKey, "SM2")) {
        LOG(LOG_WARNING, "Public key is SM2 type (confirmed by EVP_PKEY_is_a)");
    } else {
        LOG(LOG_WARNING, "Public key type not recognized by EVP_PKEY_is_a");
        EVP_PKEY_free(newKey);
        newKey = NULL;
        goto end;
    }
    
    *key = newKey;
    ret = 0;
    
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (decoder_ctx != NULL) {
        OSSL_DECODER_CTX_free(decoder_ctx);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_sm2_private_key_create_by_string(const unsigned char *keyStr, EVP_PKEY **key)
{
    BIO *bio = NULL;
    EVP_PKEY *newKey = NULL;
    OSSL_DECODER_CTX *decoder_ctx = NULL;
    int ret = -1;

    if (keyStr == NULL || key == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        LOG(LOG_WARNING, "failed to call BIO_new.");
        goto end;
    }

    if (BIO_puts(bio, (char *)keyStr) <= 0) {
        LOG(LOG_WARNING, "failed to call BIO_puts.");
        goto end;
    }

    ERR_clear_error();
    
    // 使用现代的OSSL_DECODER API
    decoder_ctx = OSSL_DECODER_CTX_new_for_pkey(&newKey, "PEM", NULL, "EC", 
                                                EVP_PKEY_KEYPAIR, NULL, NULL);
    if (decoder_ctx == NULL) {
        LOG(LOG_WARNING, "Failed to create decoder context");
        print_openssl_errors();
        goto end;
    }
    
    // 尝试解码私钥
    if (OSSL_DECODER_from_bio(decoder_ctx, bio) <= 0) {
        LOG(LOG_WARNING, "Failed to decode private key with OSSL_DECODER");
        goto end;
    }
    
    if (EVP_PKEY_is_a(newKey, "SM2")) {
        LOG(LOG_WARNING, "Key is SM2 type (confirmed by EVP_PKEY_is_a)");
    } else {
        LOG(LOG_WARNING, "Key type not recognized by EVP_PKEY_is_a");
        EVP_PKEY_free(newKey);
        newKey = NULL;
        goto end;
    }
    
    *key = newKey;
    ret = 0;
    
end:
    if (bio != NULL) {
        BIO_free(bio);
    }
    if (decoder_ctx != NULL) {
        OSSL_DECODER_CTX_free(decoder_ctx);
    }
    if (ret != 0 && newKey != NULL) {
        EVP_PKEY_free(newKey);
    }
    return ret;
}

int dp_sm2_encrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EVP_PKEY_CTX *enc_ctx = NULL;

    if (in == NULL || key == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    // 使用EVP API进行SM2加密
    enc_ctx = EVP_PKEY_CTX_new(key, NULL);
    if (enc_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create encryption context.");
        goto end;
    }

    if (EVP_PKEY_encrypt_init(enc_ctx) <= 0) {
        LOG(LOG_WARNING, "SM2 encryption not supported in this OpenSSL version. Error: operation not supported for this keytype");
        print_openssl_errors();
        goto end;
    }

    // 获取密文长度
    if (EVP_PKEY_encrypt(enc_ctx, NULL, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to get ciphertext length.");
        goto end;
    }

    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (EVP_PKEY_encrypt(enc_ctx, outData, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to encrypt data.");
        goto end;
    }

    *out = outData;
    *outLen = outDataLen;
    ret = 0;
end:
    if (enc_ctx != NULL) {
        EVP_PKEY_CTX_free(enc_ctx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_decrypt(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **out, size_t *outLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EVP_PKEY_CTX *dec_ctx = NULL;

    if (in == NULL || key == NULL || out == NULL || outLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    // 使用EVP API进行SM2解密
    dec_ctx = EVP_PKEY_CTX_new(key, NULL);
    if (dec_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create decryption context.");
        goto end;
    }

    if (EVP_PKEY_decrypt_init(dec_ctx) <= 0) {
        LOG(LOG_WARNING, "SM2 decryption not supported in this OpenSSL version. Error: operation not supported for this keytype");
        print_openssl_errors();
        goto end;
    }

    // 获取明文长度
    if (EVP_PKEY_decrypt(dec_ctx, NULL, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to get plaintext length.");
        goto end;
    }
    outData = OPENSSL_zalloc(outDataLen+1);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        goto end;
    }

    if (EVP_PKEY_decrypt(dec_ctx, outData, &outDataLen, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to decrypt data.");
        goto end;
    }

    *out = outData;
    *outLen = outDataLen;
    ret = 0;
    LOG(LOG_WARNING, "decrypt data:%s, len:%d", (char *)outData,outDataLen);
end:
    if (dec_ctx != NULL) {
        EVP_PKEY_CTX_free(dec_ctx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_sign(EVP_PKEY *key, const unsigned char *in, size_t inLen, unsigned char **sign, size_t *signLen)
{
    int ret = -1;
    unsigned char *outData = NULL;
    size_t outDataLen = 0;
    EVP_PKEY_CTX *sign_ctx = NULL;
    EVP_MD_CTX *md_ctx = NULL;
    unsigned char hash[32];  // SM3输出长度为32字节
    unsigned int hash_len = 0;

    if (in == NULL || key == NULL || sign == NULL || signLen == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    // 计算SM3哈希
    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create digest context.");
        goto end;
    }

    if (EVP_DigestInit_ex(md_ctx, EVP_sm3(), NULL) <= 0) {
        LOG(LOG_WARNING, "failed to init SM3 digest.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    if (EVP_DigestUpdate(md_ctx, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to update digest.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    if (EVP_DigestFinal_ex(md_ctx, hash, &hash_len) <= 0) {
        LOG(LOG_WARNING, "failed to finalize digest.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    EVP_MD_CTX_free(md_ctx);
    LOG(LOG_WARNING, "SM3 hash calculated: %u bytes", hash_len);

    // 使用EVP API进行SM2签名
    sign_ctx = EVP_PKEY_CTX_new(key, NULL);
    if (sign_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create signature context.");
        goto end;
    }

    if (EVP_PKEY_sign_init(sign_ctx) <= 0) {
        LOG(LOG_WARNING, "failed to init signature.");
        goto end;
    }

    // 设置SM2签名算法 - 使用SM3
    if (EVP_PKEY_CTX_set_signature_md(sign_ctx, EVP_sm3()) <= 0) {
        LOG(LOG_WARNING, "failed to set SM3 digest for SM2 signature.");
        EVP_PKEY_CTX_free(sign_ctx);
        goto end;
    }

    // 获取签名长度
    if (EVP_PKEY_sign(sign_ctx, NULL, &outDataLen, hash, hash_len) <= 0) {
        LOG(LOG_WARNING, "failed to get signature length.");
        EVP_PKEY_CTX_free(sign_ctx);
        goto end;
    }

    outData = OPENSSL_malloc(outDataLen);
    if (outData == NULL) {
        LOG(LOG_WARNING, "failed to call OPENSSL_malloc.");
        EVP_PKEY_CTX_free(sign_ctx);
        goto end;
    }

    if (EVP_PKEY_sign(sign_ctx, outData, &outDataLen, hash, hash_len) <= 0) {
        LOG(LOG_WARNING, "failed to sign data.");
        print_openssl_errors();
        EVP_PKEY_CTX_free(sign_ctx);
        goto end;
    }

    *sign = outData;
    *signLen = outDataLen;
    ret = 0;
    LOG(LOG_WARNING, "SM2 signature successful: %zu bytes", outDataLen);

end:
    if (sign_ctx != NULL) {
        EVP_PKEY_CTX_free(sign_ctx);
    }
    if (ret != 0 && outData != NULL) {
        OPENSSL_free(outData);
    }
    return ret;
}

int dp_sm2_verify(EVP_PKEY *key, const unsigned char *in, size_t inLen, const unsigned char *sign, size_t signLen)
{
    int ret = -1;
    EVP_PKEY_CTX *verify_ctx = NULL;
    EVP_MD_CTX *md_ctx = NULL;
    unsigned char hash[32];  // SM3输出长度为32字节
    unsigned int hash_len = 0;

    if (in == NULL || key == NULL || sign == NULL) {
        LOG(LOG_WARNING, "invalid params.");
        goto end;
    }

    // 计算SM3哈希
    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create digest context for verification.");
        goto end;
    }

    if (EVP_DigestInit_ex(md_ctx, EVP_sm3(), NULL) <= 0) {
        LOG(LOG_WARNING, "failed to init SM3 digest for verification.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    if (EVP_DigestUpdate(md_ctx, in, inLen) <= 0) {
        LOG(LOG_WARNING, "failed to update digest for verification.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    if (EVP_DigestFinal_ex(md_ctx, hash, &hash_len) <= 0) {
        LOG(LOG_WARNING, "failed to finalize digest for verification.");
        EVP_MD_CTX_free(md_ctx);
        goto end;
    }

    EVP_MD_CTX_free(md_ctx);
    LOG(LOG_WARNING, "SM3 hash calculated for verification: %u bytes", hash_len);

    // 使用EVP API进行SM2验证
    verify_ctx = EVP_PKEY_CTX_new(key, NULL);
    if (verify_ctx == NULL) {
        LOG(LOG_WARNING, "failed to create verification context.");
        goto end;
    }

    if (EVP_PKEY_verify_init(verify_ctx) <= 0) {
        LOG(LOG_WARNING, "failed to init verification.");
        goto end;
    }

    // 设置SM2验证算法 - 使用SM3
    if (EVP_PKEY_CTX_set_signature_md(verify_ctx, EVP_sm3()) <= 0) {
        LOG(LOG_WARNING, "failed to set SM3 digest for SM2 verification.");
        EVP_PKEY_CTX_free(verify_ctx);
        goto end;
    }

    if (EVP_PKEY_verify(verify_ctx, sign, signLen, hash, hash_len) <= 0) {
        LOG(LOG_WARNING, "failed to verify signature.");
        print_openssl_errors();
        EVP_PKEY_CTX_free(verify_ctx);
        goto end;
    }

    ret = 0;
    LOG(LOG_WARNING, "SM2 signature verification successful");

end:
    if (verify_ctx != NULL) {
        EVP_PKEY_CTX_free(verify_ctx);
    }
    return ret;
}