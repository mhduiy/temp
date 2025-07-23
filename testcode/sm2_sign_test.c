#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/err.h>

int main() {
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;
    unsigned char message[] = "Hello SM2!";
    size_t message_len = strlen((char *)message);
    unsigned char *signature = NULL;
    size_t signature_len;
    int ret = 0;
    
    printf("Testing SM2 signature with OpenSSL 3.0...\n");
    
    // 初始化OpenSSL
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    
    // 生成SM2密钥对 - 使用正确的API
    printf("1. Generating SM2 key pair...\n");
    pctx = EVP_PKEY_CTX_new_from_name(NULL, "SM2", NULL);
    if (pctx == NULL) {
        printf("Failed to create SM2 PKEY context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_keygen_init(pctx) <= 0) {
        printf("Failed to init SM2 keygen\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        printf("Failed to generate SM2 key pair\n");
        goto cleanup;
    }
    
    printf("   ✓ SM2 key pair generated successfully\n");
    
    // 签名测试
    printf("2. Testing signature...\n");
    EVP_PKEY_CTX *sign_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (sign_ctx == NULL) {
        printf("Failed to create signature context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_sign_init(sign_ctx) <= 0) {
        printf("Failed to init signature\n");
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    // 设置SM2签名算法
    if (EVP_PKEY_CTX_set_signature_md(sign_ctx, EVP_sm3()) <= 0) {
        printf("Failed to set SM3 digest for SM2 signature\n");
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    // 计算SM3哈希
    unsigned char hash[32];  // SM3输出长度为32字节
    unsigned int hash_len = 0;
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        printf("Failed to create digest context\n");
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    if (EVP_DigestInit_ex(md_ctx, EVP_sm3(), NULL) <= 0) {
        printf("Failed to init SM3 digest\n");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    if (EVP_DigestUpdate(md_ctx, message, message_len) <= 0) {
        printf("Failed to update digest\n");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    if (EVP_DigestFinal_ex(md_ctx, hash, &hash_len) <= 0) {
        printf("Failed to finalize digest\n");
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    EVP_MD_CTX_free(md_ctx);
    printf("   ✓ SM3 hash calculated: %u bytes\n", hash_len);
    
    // 获取签名长度
    if (EVP_PKEY_sign(sign_ctx, NULL, &signature_len, hash, hash_len) <= 0) {
        printf("Failed to get signature length\n");
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Signature length: %zu bytes\n", signature_len);
    
    signature = malloc(signature_len);
    if (signature == NULL) {
        printf("Failed to allocate signature buffer\n");
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    if (EVP_PKEY_sign(sign_ctx, signature, &signature_len, hash, hash_len) <= 0) {
        printf("Failed to sign\n");
        // 打印详细的OpenSSL错误信息
        unsigned long err;
        while ((err = ERR_get_error()) != 0) {
            char err_buf[256];
            ERR_error_string_n(err, err_buf, sizeof(err_buf));
            printf("OpenSSL error: %s\n", err_buf);
        }
        EVP_PKEY_CTX_free(sign_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Signature successful\n");
    EVP_PKEY_CTX_free(sign_ctx);
    
    // 验证签名测试
    printf("3. Testing signature verification...\n");
    EVP_PKEY_CTX *verify_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (verify_ctx == NULL) {
        printf("Failed to create verification context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_verify_init(verify_ctx) <= 0) {
        printf("Failed to init verification\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    // 设置SM2验证算法
    if (EVP_PKEY_CTX_set_signature_md(verify_ctx, EVP_sm3()) <= 0) {
        printf("Failed to set SM3 digest for SM2 verification\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    if (EVP_PKEY_verify(verify_ctx, signature, signature_len, hash, hash_len) <= 0) {
        printf("Failed to verify signature\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Signature verification successful\n");
    EVP_PKEY_CTX_free(verify_ctx);
    
    // 测试错误消息的验证
    printf("4. Testing signature verification with wrong message...\n");
    unsigned char wrong_message[] = "Wrong message!";
    size_t wrong_message_len = strlen((char *)wrong_message);
    
    // 计算错误消息的SM3哈希
    unsigned char wrong_hash[32];
    unsigned int wrong_hash_len = 0;
    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        printf("Failed to create digest context for wrong message\n");
        goto cleanup;
    }
    
    if (EVP_DigestInit_ex(md_ctx, EVP_sm3(), NULL) <= 0) {
        printf("Failed to init SM3 digest for wrong message\n");
        EVP_MD_CTX_free(md_ctx);
        goto cleanup;
    }
    
    if (EVP_DigestUpdate(md_ctx, wrong_message, wrong_message_len) <= 0) {
        printf("Failed to update digest for wrong message\n");
        EVP_MD_CTX_free(md_ctx);
        goto cleanup;
    }
    
    if (EVP_DigestFinal_ex(md_ctx, wrong_hash, &wrong_hash_len) <= 0) {
        printf("Failed to finalize digest for wrong message\n");
        EVP_MD_CTX_free(md_ctx);
        goto cleanup;
    }
    
    EVP_MD_CTX_free(md_ctx);
    
    verify_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (verify_ctx == NULL) {
        printf("Failed to create verification context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_verify_init(verify_ctx) <= 0) {
        printf("Failed to init verification\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    // 设置SM2验证算法
    if (EVP_PKEY_CTX_set_signature_md(verify_ctx, EVP_sm3()) <= 0) {
        printf("Failed to set SM3 digest for SM2 verification\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    if (EVP_PKEY_verify(verify_ctx, signature, signature_len, wrong_hash, wrong_hash_len) > 0) {
        printf("   ✗ Signature verification should have failed!\n");
        EVP_PKEY_CTX_free(verify_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Signature verification correctly rejected wrong message\n");
    EVP_PKEY_CTX_free(verify_ctx);
    
    ret = 1;
    printf("\n🎉 All tests passed! SM2 signature working correctly.\n");
    printf("   ✓ Original message: %s\n", message);
    printf("   ✓ Signature length: %zu bytes\n", signature_len);
    
cleanup:
    if (pctx) EVP_PKEY_CTX_free(pctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (signature) free(signature);
    
    if (!ret) {
        printf("\n❌ Tests failed!\n");
        // 打印OpenSSL错误信息
        unsigned long err;
        while ((err = ERR_get_error()) != 0) {
            char err_buf[256];
            ERR_error_string_n(err, err_buf, sizeof(err_buf));
            printf("OpenSSL error: %s\n", err_buf);
        }
    }
    
    return ret ? 0 : 1;
} 