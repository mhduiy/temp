#include <gtest/gtest.h>

#include "decode/aes.h"
#include "decode/b64.h"
#include "decode/evp.h"
#include "decode/rsa.h"
#include "decode/sm2.h"
#include "decode/sm4.h"
#include "global_test_env.h"

#include <openssl/rsa.h>

#include <tuple>
#include <vector>

#include <stdbool.h>

class TestServiceDecode : public testing::Test
{
public:
    void SetUp() override { ASSERT_NE(TestEnv::testWorkDir, ""); }

    void TearDown() override { }
};

TEST_F(TestServiceDecode, SM4)
{
    using TestData = std::vector<std::tuple<std::string, std::string, std::string, bool>>;
    TestData tests = { { "test_sm4", "1234567890abcdef", "aaaabbbbbccc", true },
                       { "test_sm4", "1111111111111111", "", false },
                       { "test_sm4", "asdfasfasdfvvfdf", "1", true },
                       { "test_sm4", "asdfasfasdfvvfdf", "123456722", true },
                       { "test_sm4", "asdfsaf2131fds33", "1234567890abcdef", true },
                       { "test_sm4", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef", true },
                       { "test_sm4", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef", true },
                       { "test_sm4", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef", true },
                       { "test_sm4", "asdfsaf2131fds33", "aaaabbbbbccc", true } };

    auto testFunc = [](const auto &data) {
        std::string key = std::get<1>(data);
        std::string in = std::get<2>(data);
        bool success = std::get<3>(data);
        unsigned char *encData = nullptr;
        int encDataLen = 0;
        unsigned char *decData = nullptr;
        int decDatalen = 0;
        int ret = 0;

        do {
            ret = dp_sm4_ecb_encrypt((unsigned char *)key.c_str(), (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            ret = dp_sm4_ecb_decrypt((unsigned char *)key.c_str(), encData, encDataLen, &decData, &decDatalen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, AES)
{
    using TestData = std::vector<std::tuple<std::string, std::string, std::string, bool>>;
    TestData tests = { { "test_aes", "1234567890abcdef", "aaaabbbbbccc", true },
                       { "test_aes", "1111111111111111", "", false },
                       { "test_aes", "asdfasfasdfvvfdf", "1", true },
                       { "test_aes", "asdfasfasdfvvfdf", "123456722", true },
                       { "test_aes", "asdfsaf2131fds33", "1234567890abcdef", true },
                       { "test_aes", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef", true },
                       { "test_aes", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef", true },
                       { "test_aes", "asdfsaf2131fds33", "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef", true },
                       { "test_aes", "asdfsaf2131fds33", "aaaabbbbbccc", true } };

    auto testFunc = [](const auto &data) {
        std::string key = std::get<1>(data);
        std::string in = std::get<2>(data);
        bool success = std::get<3>(data);
        unsigned char *encData = nullptr;
        int encDataLen = 0;
        unsigned char *decData = nullptr;
        int decDataLen = 0;
        int ret = 0;

        do {
            ret = dp_aes_ecb128_encrypt((unsigned char *)key.c_str(), (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            ret = dp_aes_ecb128_decrypt((unsigned char *)key.c_str(), encData, encDataLen, &decData, &decDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, RSAEnc1)
{
    using TestData = std::vector<std::tuple<std::string, int, int, int, std::string, bool>>;
    TestData tests = { { "test_rsa_enc", 1024, RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, "asfas", false },
                       { "test_rsa_enc", 1024, RSA_PKCS1_PADDING, RSA_PKCS1_PADDING, "safasdf3dsf", true },
                       { "test_rsa_enc", 1024, RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, "23423423423423sdgfsdgsdg32423dgf3fdgg3443fdgdfgdf", false },
                       { "test_rsa_enc", 1024, RSA_PKCS1_PADDING, RSA_PKCS1_PADDING, "23423423423423sdgfsdgsdg32423dgf3fdgg3443fdgdfgdf", true },
                       { "test_rsa_enc", 2048, RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, "fsadfasdf", false },
                       { "test_rsa_enc", 2048, RSA_PKCS1_PADDING, RSA_PKCS1_PADDING, "dfasdf23443", true },
                       { "test_rsa_enc", 2048, RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, "adgsdfgsdfg3dvjndfg7df6gdf876g8d7f6g8dfg87dfg6df", false },
                       { "test_rsa_enc", 2048, RSA_PKCS1_PADDING, RSA_PKCS1_PADDING, "fasdkjfaisdfasd87f678asd76f8sdfsd7f6hsd6h78fsd786hfds76hf", true } };

    auto testFunc = [](const auto &data) {
        int keySize = std::get<1>(data);
        int paddingEnc = std::get<2>(data);
        int paddingDec = std::get<3>(data);
        std::string in = std::get<4>(data);
        bool success = std::get<5>(data);
        int ret = 0;

        EVP_PKEY *key = NULL;
        EVP_PKEY *key2 = NULL;
        unsigned char *publicKey = nullptr;
        unsigned char *publicKey2 = nullptr;
        unsigned char *privateKey = nullptr;
        ///
        unsigned char *encData = nullptr;
        size_t encDataLen = 0;
        unsigned char *decData = nullptr;
        size_t decDataLen = 0;

        do {
            ret = dp_rsa_key_generate(keySize, &key);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_rsa_get_public_key(key, &publicKey);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_rsa_get_private_key(key, &privateKey);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_private_key_create_by_string(privateKey, &key2);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_rsa_get_public_key(key2, &publicKey2);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            // 比较公钥
            EXPECT_STREQ((char *)publicKey, (char *)publicKey2);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_encrypt_ex(key2, paddingEnc, (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_decrypt_ex(key2, paddingDec, encData, encDataLen, &decData, &decDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            // 比较经过加解密后的数据
            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
        if (key != NULL) {
            EVP_PKEY_free(key);
        }
        if (key2 != NULL) {
            EVP_PKEY_free(key2);
        }
        if (publicKey != nullptr) {
            free(publicKey);
        }
        if (publicKey2 != nullptr) {
            free(publicKey2);
        }
        if (privateKey != nullptr) {
            free(privateKey);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, RSAEnc2)
{
    using TestData = std::vector<std::tuple<std::string, std::string, std::string, std::string, bool>>;
    TestData tests = { { "test_rsa_enc2",
                         R"(-----BEGIN RSA PRIVATE KEY-----
MIICeQIBADANBgkqhkiG9w0BAQEFAASCAmMwggJfAgEAAoGBAOuMy5nCtrjFtrJU
mi3Ul4Kx/BkxXWYlUmaqvLg7+97Aseb9QVdeHZxBStoKEr6OjredMmtwU4485ASt
WMRiHzWhhZLtaXoy4aBMXgfA+tM6YrvNFdRGFiD1VRcfr+toWBu0o4T/JssIy0nK
o5aP9dQNjU6IQLnSwecbiVoZuDO1AgMBAAECgYEAqwCqBtSfBoEy40AcNk1yOQWx
uTBnV/KcYcBuBWo2rj5CaSlOuHixEnEsQCKz/PdNkOtkYS9I2ahG+UZHfTqm4Eny
mh5tBZYLAF5542tducFR5NsneJhk8ozxz5sDbr6DV/st+uS7br1aKYGNibv+sS+V
RJbp/YqYFMykHCOJJ3kCQQD4YYO8y3ZMoASY3k3h/oKBvM5V43ywMoEn2T06v8Q+
FRREQ9DUF6KsRvgv1yJg27STmVFOMHu6DbztdPSFhfdvAkEA8saFn3wKtKIEKedj
1rF5sdEOzyWUoowANqmdq3TfhE0LjsZP6Y22joJHv7cIL7PPJFg9g4GLq+CD51SE
3+UVGwJBAOtQL2fI9u3HWDOfwPePkQLq5iy21eV0I6qmBH1JOtl6TPYTrZ+SONqw
beedEQPkgmKniUDjbASriUcKW7K4Zd8CQQDXpevvvJkt1nRoXHdcr4zvnEnRSwp6
UG2oIIZDVK9ur5lCm3lEdnLgp2zgKM8GwGuRVyhc9eeBJw9Dntngn3ZdAkEAsyXf
fl8aMK6yQrq0HUwqxvLSYSI0Hoy1gygeDXcn3E2JDn8Vo//bSSHjU4L6EiLC6Wo5
sbBHNYmMVubHYimr8A==
-----END RSA PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDrjMuZwra4xbayVJot1JeCsfwZ
MV1mJVJmqry4O/vewLHm/UFXXh2cQUraChK+jo63nTJrcFOOPOQErVjEYh81oYWS
7Wl6MuGgTF4HwPrTOmK7zRXURhYg9VUXH6/raFgbtKOE/ybLCMtJyqOWj/XUDY1O
iEC50sHnG4laGbgztQIDAQAA
-----END PUBLIC KEY-----
)", // 末尾QAB改为QAA了
                         "dgfdgsdgsdfgdfgsdgf",
                         false },
                       { "test_rsa_enc2",
                         R"(-----BEGIN RSA PRIVATE KEY-----
MIICeQIBADANBgkqhkiG9w0BAQEFAASCAmMwggJfAgEAAoGBAOuMy5nCtrjFtrJU
mi3Ul4Kx/BkxXWYlUmaqvLg7+97Aseb9QVdeHZxBStoKEr6OjredMmtwU4485ASt
WMRiHzWhhZLtaXoy4aBMXgfA+tM6YrvNFdRGFiD1VRcfr+toWBu0o4T/JssIy0nK
o5aP9dQNjU6IQLnSwecbiVoZuDO1AgMBAAECgYEAqwCqBtSfBoEy40AcNk1yOQWx
uTBnV/KcYcBuBWo2rj5CaSlOuHixEnEsQCKz/PdNkOtkYS9I2ahG+UZHfTqm4Eny
mh5tBZYLAF5542tducFR5NsneJhk8ozxz5sDbr6DV/st+uS7br1aKYGNibv+sS+V
RJbp/YqYFMykHCOJJ3kCQQD4YYO8y3ZMoASY3k3h/oKBvM5V43ywMoEn2T06v8Q+
FRREQ9DUF6KsRvgv1yJg27STmVFOMHu6DbztdPSFhfdvAkEA8saFn3wKtKIEKedj
1rF5sdEOzyWUoowANqmdq3TfhE0LjsZP6Y22joJHv7cIL7PPJFg9g4GLq+CD51SE
3+UVGwJBAOtQL2fI9u3HWDOfwPePkQLq5iy21eV0I6qmBH1JOtl6TPYTrZ+SONqw
beedEQPkgmKniUDjbASriUcKW7K4Zd8CQQDXpevvvJkt1nRoXHdcr4zvnEnRSwp6
UG2oIIZDVK9ur5lCm3lEdnLgp2zgKM8GwGuRVyhc9eeBJw9Dntngn3ZdAkEAsyXf
fl8aMK6yQrq0HUwqxvLSYSI0Hoy1gygeDXcn3E2JDn8Vo//bSSHjU4L6EiLC6Wo5
sbBHNYmMVubHYimr8A==
-----END RSA PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDrjMuZwra4xbayVJot1JeCsfwZ
MV1mJVJmqry4O/vewLHm/UFXXh2cQUraChK+jo63nTJrcFOOPOQErVjEYh81oYWS
7Wl6MuGgTF4HwPrTOmK7zRXURhYg9VUXH6/raFgbtKOE/ybLCMtJyqOWj/XUDY1O
iEC50sHnG4laGbgztQIDAQAB
-----END PUBLIC KEY-----
)",
                         "asfas",
                         true },
                       { "test_rsa_enc2",
                         R"(-----BEGIN RSA PRIVATE KEY-----
MIICeQIBADANBgkqhkiG9w0BAQEFAASCAmMwggJfAgEAAoGBAOuMy5nCtrjFtrJU
mi3Ul4Kx/BkxXWYlUmaqvLg7+97Aseb9QVdeHZxBStoKEr6OjredMmtwU4485ASt
WMRiHzWhhZLtaXoy4aBMXgfA+tM6YrvNFdRGFiD1VRcfr+toWBu0o4T/JssIy0nK
o5aP9dQNjU6IQLnSwecbiVoZuDO1AgMBAAECgYEAqwCqBtSfBoEy40AcNk1yOQWx
uTBnV/KcYcBuBWo2rj5CaSlOuHixEnEsQCKz/PdNkOtkYS9I2ahG+UZHfTqm4Eny
mh5tBZYLAF5542tducFR5NsneJhk8ozxz5sDbr6DV/st+uS7br1aKYGNibv+sS+V
RJbp/YqYFMykHCOJJ3kCQQD4YYO8y3ZMoASY3k3h/oKBvM5V43ywMoEn2T06v8Q+
FRREQ9DUF6KsRvgv1yJg27STmVFOMHu6DbztdPSFhfdvAkEA8saFn3wKtKIEKedj
1rF5sdEOzyWUoowANqmdq3TfhE0LjsZP6Y22joJHv7cIL7PPJFg9g4GLq+CD51SE
3+UVGwJBAOtQL2fI9u3HWDOfwPePkQLq5iy21eV0I6qmBH1JOtl6TPYTrZ+SONqw
beedEQPkgmKniUDjbASriUcKW7K4Zd8CQQDXpevvvJkt1nRoXHdcr4zvnEnRSwp6
UG2oIIZDVK9ur5lCm3lEdnLgp2zgKM8GwGuRVyhc9eeBJw9Dntngn3ZdAkEAsyXf
fl8aMK6yQrq0HUwqxvLSYSI0Hoy1gygeDXcn3E2JDn8Vo//bSSHjU4L6EiLC6Wo5
sbBHNYmMVubHYimr8A==
-----END RSA PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDrjMuZwra4xbayVJot1JeCsfwZ
MV1mJVJmqry4O/vewLHm/UFXXh2cQUraChK+jo63nTJrcFOOPOQErVjEYh81oYWS
7Wl6MuGgTF4HwPrTOmK7zRXURhYg9VUXH6/raFgbtKOE/ybLCMtJyqOWj/XUDY1O
iEC50sHnG4laGbgztQIDAQAB
-----END PUBLIC KEY-----
)",
                         "afasdfasdfmasdkfjsdakihfiasudhfiu34y984r7394fy3498fy349fy347fy437y4f7",
                         true } };

    auto testFunc = [](const auto &data) {
        std::string privateKey = std::get<1>(data);
        std::string publicKey = std::get<2>(data);
        std::string in = std::get<3>(data);
        bool success = std::get<4>(data);
        int ret = 0;

        EVP_PKEY *keyPri = NULL;
        EVP_PKEY *keyPub = NULL;
        unsigned char *publicKey2 = nullptr;

        unsigned char *encData = nullptr;
        size_t encDataLen = 0;
        unsigned char *decData = nullptr;
        size_t decDataLen = 0;

        do {
            ret = dp_rsa_private_key_create_by_string((unsigned char *)privateKey.c_str(), &keyPri);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_rsa_public_key_create_by_string((unsigned char *)publicKey.c_str(), &keyPub);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_rsa_get_public_key(keyPri, &publicKey2);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_encrypt(keyPub, (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_decrypt(keyPri, encData, encDataLen, &decData, &decDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            // 比较经过加解密后的数据
            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
        if (keyPub != NULL) {
            EVP_PKEY_free(keyPub);
        }
        if (keyPri != NULL) {
            EVP_PKEY_free(keyPri);
        }
        if (publicKey2 != nullptr) {
            free(publicKey2);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, SM2EncTest1)
{
    using TestData = std::vector<std::tuple<std::string, std::string, bool>>;
    TestData tests = {
        { "test_sm2_enc1", "asfas", true },
        { "test_sm2_enc1", "asfasdfasdfasdfasdfasdfasdfasdfasdfsadfasfs", true },
    };

    auto testFunc = [](const auto &data) {
        std::string in = std::get<1>(data);
        bool success = std::get<2>(data);
        int ret = 0;

        EVP_PKEY *key = NULL;
        EVP_PKEY *key2 = NULL;
        unsigned char *publicKey = nullptr;
        unsigned char *publicKey2 = nullptr;
        unsigned char *privateKey = nullptr;
        ///
        unsigned char *encData = nullptr;
        size_t encDataLen = 0;
        unsigned char *decData = nullptr;
        size_t decDataLen = 0;

        do {
            ret = dp_sm2_key_generate(&key);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_sm2_get_public_key(key, &publicKey);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_sm2_get_private_key(key, &privateKey);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_private_key_create_by_string(privateKey, &key2);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_encrypt(key2, (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_decrypt(key, encData, encDataLen, &decData, &decDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

            // 比较经过加解密后的数据
            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
        if (key != NULL) {
            EVP_PKEY_free(key);
        }
        if (key2 != NULL) {
            EVP_PKEY_free(key2);
        }
        if (publicKey != nullptr) {
            free(publicKey);
        }
        if (publicKey2 != nullptr) {
            free(publicKey2);
        }
        if (privateKey != nullptr) {
            free(privateKey);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, SM2EncTest2)
{
    using TestData = std::vector<std::tuple<std::string, std::string, std::string, std::string, bool>>;
    TestData tests = { { "test_sm2_enc2",
                         R"(-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIHv6Vv/sAA7ZJ9EH/hIXdWD2ZNJxmJFtXK4Z1kFQgLfYoAoGCCqBHM9V
AYItoUQDQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ2Qspwb1e8PzvjnOW3E4yJw1O
aUk1E8a+1DYn3YNYmOT9n49MIwwEfSq5ng==
-----END EC PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoEcz1UBgi0DQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ
2Qspwb1e8PzvjnOW3E4yJw1OaUk1E8a+1DYn3YNYmOT9n49MIwwEfSq6ng==
-----END PUBLIC KEY-----
)", // 末尾5ng==改为6ng==了
                         "dgfdgsdgsdfgdfgsdgf",
                         false },
                       { "test_sm2_enc2",
                         R"(-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIHv6Vv/sAA7ZJ9EH/hIXdWD2ZNJxmJFtXK4Z1kFQgLfYoAoGCCqBHM9V
AYItoUQDQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ2Qspwb1e8PzvjnOW3E4yJw1O
aUk1E8a+1DYn3YNYmOT9n49MIwwEfSq5ng==
-----END EC PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoEcz1UBgi0DQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ
2Qspwb1e8PzvjnOW3E4yJw1OaUk1E8a+1DYn3YNYmOT9n49MIwwEfSq5ng==
-----END PUBLIC KEY-----
)",
                         "asfas",
                         true },
                       { "test_sm2_enc2",
                         R"(-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIHv6Vv/sAA7ZJ9EH/hIXdWD2ZNJxmJFtXK4Z1kFQgLfYoAoGCCqBHM9V
AYItoUQDQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ2Qspwb1e8PzvjnOW3E4yJw1O
aUk1E8a+1DYn3YNYmOT9n49MIwwEfSq5ng==
-----END EC PRIVATE KEY-----
)",
                         R"(-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoEcz1UBgi0DQgAE1jn2+GVpGd3Ek27H8N8BEOS8t6kJ
2Qspwb1e8PzvjnOW3E4yJw1OaUk1E8a+1DYn3YNYmOT9n49MIwwEfSq5ng==
-----END PUBLIC KEY-----
)",
                         "afasdfasdfmasdkfjsdakihfiasudhfiu34y984r7394fy3498fy349fy347fy437y4f7",
                         true } };

    auto testFunc = [](const auto &data) {
        std::string privateKey = std::get<1>(data);
        std::string publicKey = std::get<2>(data);
        std::string in = std::get<3>(data);
        bool success = std::get<4>(data);
        int ret = 0;

        EVP_PKEY *keyPri = NULL;
        EVP_PKEY *keyPub = NULL;
        unsigned char *publicKey2 = nullptr;

        unsigned char *encData = nullptr;
        size_t encDataLen = 0;
        unsigned char *decData = nullptr;
        size_t decDataLen = 0;

        do {
            ret = dp_sm2_private_key_create_by_string((unsigned char *)privateKey.c_str(), &keyPri);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_sm2_public_key_create_by_string((unsigned char *)publicKey.c_str(), &keyPub);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
                break;
            }

            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_get_public_key(keyPri, &publicKey2);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_sm2_encrypt(keyPub, (unsigned char *)in.c_str(), in.length(), &encData, &encDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }
            ret = dp_sm2_decrypt(keyPri, encData, encDataLen, &decData, &decDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
                break;
            }

            // 比较经过加解密后的数据
            if (success) {
                EXPECT_STREQ(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_STRNE(in.c_str(), (char *)decData);
                if (HasFailure()) {
                    break;
                }
            }
        } while (0);
        if (encData != nullptr) {
            free(encData);
        }
        if (decData != nullptr) {
            free(decData);
        }
        if (keyPub != NULL) {
            EVP_PKEY_free(keyPub);
        }
        if (keyPri != NULL) {
            EVP_PKEY_free(keyPri);
        }
        if (publicKey2 != nullptr) {
            free(publicKey2);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}

TEST_F(TestServiceDecode, RsaSignTest1)
{
    using TestData = std::vector<std::tuple<std::string, int, std::string, bool>>;
    TestData tests = {
        { "test_rsa_sign1", 1024, "asfasdfasdfasdfasdfasdfasdfasdfasdfsadfasfs", true },
        { "test_rsa_sign1", 4096, "asfasdfasdfasdfasdfasdfasdfasdfasdfsadfasfs", true },
        { "test_rsa_sign1", 2048, "asfas", true },
        { "test_rsa_sign1", 2048, "asfasdfasdfasdfasdfasdfasdfasdsadadasdsadsadsadsadfasdfsadfasfs", true },
    };

    auto testFunc = [](const auto &data) {
        int keySize = std::get<1>(data);
        std::string in = std::get<2>(data);
        bool success = std::get<3>(data);
        int ret = 0;

        EVP_PKEY *key = NULL;
        unsigned char *signData = nullptr;
        size_t signDataLen = 0;

        do {
            ret = dp_rsa_key_generate(keySize, &key);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_sign(key, (unsigned char *)in.c_str(), in.length(), &signData, &signDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_rsa_verify(key, (unsigned char *)in.c_str(), in.length(), signData, signDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

        } while (0);
        if (signData != nullptr) {
            free(signData);
        }
        if (key != NULL) {
            EVP_PKEY_free(key);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}


TEST_F(TestServiceDecode, SM2SignTest1)
{
    using TestData = std::vector<std::tuple<std::string, std::string, bool>>;
    TestData tests = {
        { "test_rsa_sign1", "asfasdfasdfasdfasdfasdfasdfasdfasdfsadfasfs", true },
        { "test_rsa_sign1", "asfasdfasdfasdfasdfasdfasdfasdfasdfsadfasfs", true },
        { "test_rsa_sign1", "asfas", true },
        { "test_rsa_sign1", "asfasdfasdfasdfasdfasdfasdfasdsadadasdsadsadsadsadfasdfsadfasfs", true },
    };

    auto testFunc = [](const auto &data) {
        std::string in = std::get<1>(data);
        bool success = std::get<2>(data);
        int ret = 0;

        EVP_PKEY *key = NULL;
        unsigned char *signData = nullptr;
        size_t signDataLen = 0;

        do {
            ret = dp_sm2_key_generate(&key);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_sign(key, (unsigned char *)in.c_str(), in.length(), &signData, &signDataLen);
            EXPECT_EQ(ret, 0);
            if (HasFailure()) {
                break;
            }

            ret = dp_sm2_verify(key, (unsigned char *)in.c_str(), in.length(), signData, signDataLen);
            if (success) {
                EXPECT_EQ(ret, 0);
                if (HasFailure()) {
                    break;
                }
            } else {
                EXPECT_EQ(ret, -1);
                if (HasFailure()) {
                    break;
                }
            }

        } while (0);
        if (signData != nullptr) {
            free(signData);
        }
        if (key != NULL) {
            EVP_PKEY_free(key);
        }
    };

    for (const auto &test : tests) {
        testFunc(test);
    }
}
