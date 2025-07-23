#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/pem.h>

int main() {
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;
    unsigned char message[] = "Hello SM2!";
    size_t message_len = strlen((char *)message);
    unsigned char *ciphertext = NULL;
    unsigned char *plaintext = NULL;
    size_t ciphertext_len, plaintext_len;
    int ret = 0;
    
    printf("Testing SM2 encryption/decryption with OpenSSL 3.0 (Fixed)...\n");
    
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
    
    // 测试SM2加密功能
    printf("2. Testing SM2 encryption...\n");
    EVP_PKEY_CTX *enc_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (enc_ctx == NULL) {
        printf("Failed to create encryption context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_encrypt_init(enc_ctx) <= 0) {
        printf("Failed to init encryption\n");
        EVP_PKEY_CTX_free(enc_ctx);
        goto cleanup;
    }
    
    printf("   ✓ SM2 encryption context created successfully\n");
    
    // 获取密文长度
    if (EVP_PKEY_encrypt(enc_ctx, NULL, &ciphertext_len, message, message_len) <= 0) {
        printf("Failed to get ciphertext length\n");
        EVP_PKEY_CTX_free(enc_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Ciphertext length: %zu bytes\n", ciphertext_len);
    
    ciphertext = malloc(ciphertext_len);
    if (ciphertext == NULL) {
        printf("Failed to allocate ciphertext buffer\n");
        EVP_PKEY_CTX_free(enc_ctx);
        goto cleanup;
    }
    
    if (EVP_PKEY_encrypt(enc_ctx, ciphertext, &ciphertext_len, message, message_len) <= 0) {
        printf("Failed to encrypt\n");
        EVP_PKEY_CTX_free(enc_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Encryption successful\n");
    EVP_PKEY_CTX_free(enc_ctx);
    
    // 解密测试
    printf("3. Testing decryption...\n");
    EVP_PKEY_CTX *dec_ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (dec_ctx == NULL) {
        printf("Failed to create decryption context\n");
        goto cleanup;
    }
    
    if (EVP_PKEY_decrypt_init(dec_ctx) <= 0) {
        printf("Failed to init decryption\n");
        EVP_PKEY_CTX_free(dec_ctx);
        goto cleanup;
    }
    
    // 获取明文长度
    if (EVP_PKEY_decrypt(dec_ctx, NULL, &plaintext_len, ciphertext, ciphertext_len) <= 0) {
        printf("Failed to get plaintext length\n");
        EVP_PKEY_CTX_free(dec_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Plaintext length: %zu bytes\n", plaintext_len);
    
    plaintext = malloc(plaintext_len);
    if (plaintext == NULL) {
        printf("Failed to allocate plaintext buffer\n");
        EVP_PKEY_CTX_free(dec_ctx);
        goto cleanup;
    }
    
    if (EVP_PKEY_decrypt(dec_ctx, plaintext, &plaintext_len, ciphertext, ciphertext_len) <= 0) {
        printf("Failed to decrypt\n");
        EVP_PKEY_CTX_free(dec_ctx);
        goto cleanup;
    }
    
    printf("   ✓ Decryption successful\n");
    EVP_PKEY_CTX_free(dec_ctx);
    
    // 验证结果
    printf("4. Verifying results...\n");
    if (plaintext_len != message_len) {
        printf("   ✗ Length mismatch: expected %zu, got %zu\n", message_len, plaintext_len);
        goto cleanup;
    }
    
    if (memcmp(plaintext, message, message_len) != 0) {
        printf("   ✗ Content mismatch\n");
        goto cleanup;
    }
    
    printf("   ✓ Verification passed!\n");
    printf("   ✓ Original message: %s\n", message);
    printf("   ✓ Decrypted message: %.*s\n", (int)plaintext_len, plaintext);
    
    ret = 1;
    printf("\n🎉 All tests passed! SM2 encryption/decryption working correctly.\n");
    
cleanup:
    if (pctx) EVP_PKEY_CTX_free(pctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (ciphertext) free(ciphertext);
    if (plaintext) free(plaintext);
    
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