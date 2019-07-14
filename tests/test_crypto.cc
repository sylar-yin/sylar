#include "sylar/util.h"
#include <iostream>
#include <assert.h>
#include <openssl/err.h>
#include "sylar/sylar.h"

void test_aes256ecb() {
    for(int i = 0; i < 4097; ++i) {
        auto str = sylar::random_string(i);
        std::string encode;
        encode.resize(i+ 30);
        std::string decode;
        decode.resize(i+ 30);

        auto key = sylar::random_string(32);

        int32_t len = sylar::CryptoUtil::AES256Ecb(key.c_str(), str.c_str(), str.size(), &encode[0], true);
        std::cout << "encode_len: " << len << std::endl;
        if(len > 0) {
            encode.resize(len);
        }
        len = sylar::CryptoUtil::AES256Ecb(key.c_str(), encode.c_str(), encode.size(), &decode[0], false);
        std::cout << "decode_len: " << len << std::endl;
        if(len > 0) {
            decode.resize(len);
        }
        std::cout << (str == decode) << std::endl;
        assert(str == decode);
    }
}

void test_aes256cbc() {
    for(int i = 0; i < 4097; ++i) {
        auto str = sylar::random_string(i);
        std::string encode;
        encode.resize(i+ 30);
        std::string decode;
        decode.resize(i+ 30);

        auto key = sylar::random_string(32);
        auto iv = sylar::random_string(16);

        int32_t len = sylar::CryptoUtil::AES256Cbc(key.c_str(), iv.c_str(), str.c_str(), str.size(), &encode[0], true);
        std::cout << "encode_len: " << len << std::endl;
        if(len > 0) {
            encode.resize(len);
        }
        len = sylar::CryptoUtil::AES256Cbc(key.c_str(), iv.c_str(), encode.c_str(), encode.size(), &decode[0], false);
        std::cout << "decode_len: " << len << std::endl;
        if(len > 0) {
            decode.resize(len);
        }
        std::cout << (str == decode) << std::endl;
        assert(str == decode);
    }
}

std::string to_hex(const std::string& str) {
    std::stringstream ss;
    for(size_t i = 0; i < str.size(); ++i) {
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i];
    }
    return ss.str();
}

void test_one() {
    //std::string key = sylar::random_string(32);
    //std::string iv = sylar::random_string(16);
    //std::string str = sylar::random_string(125);
    
    std::string key = "123456789";
    std::string iv = "123456789";
    std::string str = "123456789";

    std::string encode;
    encode.resize(str.size() + 30);
    std::string decode;
    decode.resize(str.size() + 30);
    int32_t len = sylar::CryptoUtil::AES128Cbc(key.c_str(), iv.c_str(), str.c_str(), str.size(), &encode[0], true);
    if(len >= 0) {
        encode.resize(len);
    }

    int32_t len2 = sylar::CryptoUtil::AES128Cbc(key.c_str(), iv.c_str(), encode.c_str(), encode.size(), &decode[0], false);
    if(len2 >= 0) {
        decode.resize(len2);
    }

    std::cout << "key: " << key << " hex: " << to_hex(key) << std::endl;
    std::cout << " iv: " << iv << " hex: " << to_hex(iv) << std::endl;
    std::cout << "origin: " << str << " - len=" << str.size() << std::endl;
    std::cout << "encode: " << sylar::base64encode(encode) << " - len=" << encode.size() << std::endl;
    std::cout << "encode: " << to_hex(encode) << " - len=" << encode.size() << std::endl;
    std::cout << "decode: " << decode << " - len=" << decode.size() << std::endl;
    std::cout << " ==== " << std::endl;

    {
        EVP_CIPHER_CTX ctx;
        EVP_CIPHER_CTX_init(&ctx);
        EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), (const uint8_t*)key.c_str(), (const uint8_t*)iv.c_str());

        std::string encode;
        encode.resize(str.size() + 30);
        int olen = 0;
        EVP_EncryptUpdate(&ctx, (uint8_t*)&encode[0], &olen, (const uint8_t*)str.c_str(), str.length());
        int llen = 0;
        EVP_EncryptFinal(&ctx, (uint8_t*)(&encode[0] + olen), &llen);
        encode.resize(olen + llen);
        std::cout << "encode: " << sylar::base64encode(encode) << " - len=" << encode.size() << std::endl;
        std::cout << "encode: " << to_hex(encode) << " - len=" << encode.size() << std::endl;
    }
}

void test_rsa() {
    std::string pubkey_path = "/tmp/rsa.pub";
    std::string prikey_path = "/tmp/rsa.pri";

    //std::cout << "generate: " << sylar::RSACipher::GenerateKey(pubkey_path, prikey_path) << std::endl;
    sylar::RSACipher::ptr rsa = sylar::RSACipher::Create(pubkey_path, prikey_path);
    if(!rsa) {
        std::cout << "Create RSACipher error" << std::endl;
        return;
    }
    std::cout << "Public: " << std::endl;
    std::cout << rsa->getPubkeyStr() << std::endl;
    std::cout << "Private: " << std::endl;
    std::cout << rsa->getPrikeyStr() << std::endl;

    //std::string str = "hello, world!";
    std::string str = "hello, world!";
    std::string out;
    out.resize(rsa->getPubRSASize() + 1);
    std::string out2;
    out2.resize(rsa->getPriRSASize() + 1);

    std::cout << "rsa pubsize: " << rsa->getPubRSASize()
              << " prisize: " << rsa->getPriRSASize()
              << std::endl;

    int size = rsa->publicEncrypt(str.c_str(), rsa->getPubRSASize(), &out[0]);
    if(size > 0) {
        out.resize(size);
    } else {
        ERR_print_errors_fp(stdout);
    }

    std::cout << "origin: " << str << " - size=" << str.size() << std::endl;
    std::cout << "encrypt: " << to_hex(out) << " - size=" << out.size() << std::endl;

    size = rsa->privateDecrypt(out.c_str(), rsa->getPriRSASize(), &out2[0]);
    if(size > 0) {
        out2.resize(size);
    }
        ERR_print_errors_fp(stdout);
    std::cout << "decrypt: " << to_hex(out2) << " - size=" << out2.size() << std::endl;
    std::cout << " size: " << size << std::endl;
    std::cout << "decrypt: " << out2 << std::endl;


}

int main(int argc, char** argv) {
    //OpenSSL_add_all_ciphers();
    srand(time(0));
    //test_aes256ecb();
    //test_aes256cbc();
    //test_one();
    test_rsa();
    //for(int i = 0; i < 10000; ++i) {
    //    auto v = sylar::random_string(128, "0123456789");
    //    std::cout << "[" << v << "]" << std::endl;
    //    for(auto& n : v) {
    //        SYLAR_ASSERT(n);
    //    }
    //}
    //std::cout << "===" << std::endl;
    return 0;
}
