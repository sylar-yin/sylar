#include "crypto_util.h"
#include <stdio.h>
#include <iostream>

namespace sylar {

namespace {

//struct CryptoInit {
//    CryptoInit() {
//        OpenSSL_add_all_ciphers();
//    }
//};

}

int32_t CryptoUtil::AES256Ecb(const void* key
                                ,const void* in
                                ,int32_t in_len
                                ,void* out
                                ,bool encode) {
    int32_t len = 0;
    return Crypto(EVP_aes_256_ecb(), encode, (const uint8_t*)key
                    ,nullptr, (const uint8_t*)in, in_len
                    ,(uint8_t*)out, &len);
}

int32_t CryptoUtil::AES128Ecb(const void* key
                                ,const void* in
                                ,int32_t in_len
                                ,void* out
                                ,bool encode) {
    int32_t len = 0;
    return Crypto(EVP_aes_128_ecb(), encode, (const uint8_t*)key
                    ,nullptr, (const uint8_t*)in, in_len
                    ,(uint8_t*)out, &len);

}

int32_t CryptoUtil::AES256Cbc(const void* key, const void* iv
                                ,const void* in, int32_t in_len
                                ,void* out, bool encode) {
    int32_t len = 0;
    return Crypto(EVP_aes_256_cbc(), encode, (const uint8_t*)key
                    ,(const uint8_t*)iv, (const uint8_t*)in, in_len
                    ,(uint8_t*)out, &len);

}

int32_t CryptoUtil::AES128Cbc(const void* key, const void* iv
                                ,const void* in, int32_t in_len
                                ,void* out, bool encode) {
    int32_t len = 0;
    return Crypto(EVP_aes_128_cbc(), encode, (const uint8_t*)key
                    ,(const uint8_t*)iv, (const uint8_t*)in, in_len
                    ,(uint8_t*)out, &len);
}

int32_t CryptoUtil::Crypto(const EVP_CIPHER* cipher, bool enc
                           ,const void* key, const void* iv
                           ,const void* in, int32_t in_len
                           ,void* out, int32_t* out_len) {
    int tmp_len = 0;
    bool has_error = false;

    std::shared_ptr<EVP_CIPHER_CTX> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    do {
        //static CryptoInit s_crypto_init;
        EVP_CIPHER_CTX_init(ctx.get());
        EVP_CipherInit_ex(ctx.get(), cipher, nullptr, (const uint8_t*)key
                ,(const uint8_t*)iv, enc);
        if(EVP_CipherUpdate(ctx.get(), (uint8_t*)out, &tmp_len, (const uint8_t*)in, in_len) != 1) {
            has_error = true;
            break;
        }
        *out_len = tmp_len;
        if(EVP_CipherFinal_ex(ctx.get(), (uint8_t*)out + tmp_len, &tmp_len) != 1) {
            has_error = true;
            break;
        }
        *out_len += tmp_len;
    } while(0);
    EVP_CIPHER_CTX_cleanup(ctx.get());
    if(has_error) {
        return -1;
    }
    return *out_len;
}

int32_t RSACipher::GenerateKey(const std::string& pubkey_file
                               ,const std::string& prikey_file
                               ,uint32_t length) {
    int rt = 0;
    FILE* fp = nullptr;
    RSA* rsa = nullptr;
    do {
        rsa = RSA_generate_key(length, RSA_F4, NULL, NULL);
        //rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);
        if(!rsa) {
            rt = -1;
            break;
        }

        fp = fopen(pubkey_file.c_str(), "w+");
        if(!fp) {
            rt = -2;
            break;
        }
        PEM_write_RSAPublicKey(fp, rsa);
        fclose(fp);
        fp = nullptr;

        fp = fopen(prikey_file.c_str(), "w+");
        if(!fp) {
            rt = -3;
            break;
        }
        PEM_write_RSAPrivateKey(fp, rsa, NULL, NULL, 0, NULL, NULL);
        fclose(fp);
        fp = nullptr;
    } while(false);
    if(fp) {
        fclose(fp);
    }
    if(rsa) {
        RSA_free(rsa);
    }
    return rt;
}

RSACipher::ptr RSACipher::Create(const std::string& pubkey_file
                      ,const std::string& prikey_file) {
    FILE* fp = nullptr;
    do {
        RSACipher::ptr rt = std::make_shared<RSACipher>();
        fp = fopen(pubkey_file.c_str(), "r+");
        if(!fp) {
            break;
        }
        rt->m_pubkey = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL);
        if(!rt->m_pubkey) {
            break;
        }

        RSA_print_fp(stdout, rt->m_pubkey, 0);
        std::cout << "====" << std::endl;

        std::string tmp;
        tmp.resize(1024);

        int len = 0;
        fseek(fp, 0, 0);
        do {
            len = fread(&tmp[0], 1, tmp.size(), fp);
            if(len > 0) {
                rt->m_pubkeyStr.append(tmp.c_str(), len);
            }
        } while(len > 0);
        fclose(fp);
        fp = nullptr;

        fp = fopen(prikey_file.c_str(), "r+");
        if(!fp) {
            break;
        }
        rt->m_prikey = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
        if(!rt->m_prikey) {
            break;
        }

        RSA_print_fp(stdout, rt->m_prikey, 0);
        std::cout << "====" << std::endl;
        fseek(fp, 0, 0);
        do {
            len = fread(&tmp[0], 1, tmp.size(), fp);
            if(len > 0) {
                rt->m_prikeyStr.append(tmp.c_str(), len);
            }
        } while(len > 0);
        fclose(fp);
        fp = nullptr;
        return rt;
    } while(false);
    if(fp) {
        fclose(fp);
    }
    return nullptr;
}

RSACipher::RSACipher()
    :m_pubkey(nullptr)
    ,m_prikey(nullptr) {
}

RSACipher::~RSACipher() {
    if(m_pubkey) {
        RSA_free(m_pubkey);
    }
    if(m_prikey) {
        RSA_free(m_prikey);
    }
}

int32_t RSACipher::privateEncrypt(const void* from, int flen,
                       void* to, int padding) {
    return RSA_private_encrypt(flen, (const uint8_t*)from
                                ,(uint8_t*)to, m_prikey, padding);
}

int32_t RSACipher::publicEncrypt(const void* from, int flen,
                       void* to, int padding) {
    return RSA_public_encrypt(flen, (const uint8_t*)from
                                ,(uint8_t*)to, m_pubkey, padding);
}

int32_t RSACipher::privateDecrypt(const void* from, int flen,
                       void* to, int padding) {
    return RSA_private_decrypt(flen, (const uint8_t*)from
                                ,(uint8_t*)to, m_prikey, padding);
}

int32_t RSACipher::publicDecrypt(const void* from, int flen,
                       void* to, int padding) {
    return RSA_public_decrypt(flen, (const uint8_t*)from
                                ,(uint8_t*)to, m_pubkey, padding);
}

int32_t RSACipher::privateEncrypt(const void* from, int flen,
                       std::string& to, int padding) {
    //TODO resize
    int32_t len = privateEncrypt(from, flen, &to[0], padding);
    if(len >= 0) {
        to.resize(len);
    }
    return len;
}

int32_t RSACipher::publicEncrypt(const void* from, int flen,
                       std::string& to, int padding) {
    //TODO resize
    int32_t len = publicEncrypt(from, flen, &to[0], padding);
    if(len >= 0) {
        to.resize(len);
    }
    return len;
}

int32_t RSACipher::privateDecrypt(const void* from, int flen,
                       std::string& to, int padding) {
    //TODO resize
    int32_t len = privateDecrypt(from, flen, &to[0], padding);
    if(len >= 0) {
        to.resize(len);
    }
    return len;
}

int32_t RSACipher::publicDecrypt(const void* from, int flen,
                       std::string& to, int padding) {
    //TODO resize
    int32_t len = publicDecrypt(from, flen, &to[0], padding);
    if(len >= 0) {
        to.resize(len);
    }
    return len;
}

int32_t RSACipher::getPubRSASize() {
    if(m_pubkey) {
        return RSA_size(m_pubkey);
    }
    return -1;
}

int32_t RSACipher::getPriRSASize() {
    if(m_prikey) {
        return RSA_size(m_prikey);
    }
    return -1;
}

}
