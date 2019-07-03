/**
 * @file crypto_util.h
 * @brief 加解密工具函数
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-07-02
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_UTIL_CRYPTO_UTIL_H__
#define __SYLAR_UTIL_CRYPTO_UTIL_H__

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <memory>
#include <string>

namespace sylar {

class CryptoUtil {
public:
    //key 32字节
    static int32_t AES256Ecb(const void* key
                            ,const void* in
                            ,int32_t in_len
                            ,void* out
                            ,bool encode);

    //key 16字节
    static int32_t AES128Ecb(const void* key
                            ,const void* in
                            ,int32_t in_len
                            ,void* out
                            ,bool encode);

    //key 32字节
    //iv 16字节
    static int32_t AES256Cbc(const void* key, const void* iv
                            ,const void* in, int32_t in_len
                            ,void* out, bool encode);

    //key 16字节
    //iv 16字节
    static int32_t AES128Cbc(const void* key, const void* iv
                            ,const void* in, int32_t in_len
                            ,void* out, bool encode);

    static int32_t Crypto(const EVP_CIPHER* cipher, bool enc
                          ,const void* key, const void* iv
                          ,const void* in, int32_t in_len
                          ,void* out, int32_t* out_len);
};


class RSACipher {
public:
    typedef std::shared_ptr<RSACipher> ptr;

    static int32_t GenerateKey(const std::string& pubkey_file
                               ,const std::string& prikey_file
                               ,uint32_t length = 1024);

    static RSACipher::ptr Create(const std::string& pubkey_file
                                ,const std::string& prikey_file);

    RSACipher();
    ~RSACipher();

    int32_t privateEncrypt(const void* from, int flen,
                           void* to, int padding = RSA_NO_PADDING);
    int32_t publicEncrypt(const void* from, int flen,
                           void* to, int padding = RSA_NO_PADDING);
    int32_t privateDecrypt(const void* from, int flen,
                           void* to, int padding = RSA_NO_PADDING);
    int32_t publicDecrypt(const void* from, int flen,
                           void* to, int padding = RSA_NO_PADDING);
    int32_t privateEncrypt(const void* from, int flen,
                           std::string& to, int padding = RSA_NO_PADDING);
    int32_t publicEncrypt(const void* from, int flen,
                           std::string& to, int padding = RSA_NO_PADDING);
    int32_t privateDecrypt(const void* from, int flen,
                           std::string& to, int padding = RSA_NO_PADDING);
    int32_t publicDecrypt(const void* from, int flen,
                           std::string& to, int padding = RSA_NO_PADDING);


    const std::string& getPubkeyStr() const { return m_pubkeyStr;}
    const std::string& getPrikeyStr() const { return m_prikeyStr;}

    int32_t getPubRSASize();
    int32_t getPriRSASize();
private:
    RSA* m_pubkey;
    RSA* m_prikey;
    std::string m_pubkeyStr;
    std::string m_prikeyStr;
};


}

#endif
