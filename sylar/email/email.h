#ifndef __SYLAR_EMAIL_EMAIL_H__
#define __SYLAR_EMAIL_EMAIL_H__

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace sylar {

class EMailEntity {
public:
    typedef std::shared_ptr<EMailEntity> ptr;
    static EMailEntity::ptr CreateAttach(const std::string& filename);

    void addHeader(const std::string& key, const std::string& val);
    std::string getHeader(const std::string& key, const std::string& def = "");

    const std::string& getContent() const { return m_content;}
    void setContent(const std::string& v) { m_content = v;}

    std::string toString() const;
private:
    std::map<std::string, std::string> m_headers;
    std::string m_content;
};


class EMail {
public:
    typedef std::shared_ptr<EMail> ptr;
    static EMail::ptr Create(const std::string& from_address, const std::string& from_passwd
                            ,const std::string& title, const std::string& body
                            ,const std::vector<std::string>& to_address
                            ,const std::vector<std::string>& cc_address = {}
                            ,const std::vector<std::string>& bcc_address = {});

    const std::string& getFromEMailAddress() const { return m_fromEMailAddress;}
    const std::string& getFromEMailPasswd() const { return m_fromEMailPasswd;}
    const std::string& getTitle() const { return m_title;}
    const std::string& getBody() const { return m_body;}

    void setFromEMailAddress(const std::string& v) { m_fromEMailAddress = v;}
    void setFromEMailPasswd(const std::string& v) { m_fromEMailPasswd = v;}
    void setTitle(const std::string& v) { m_title = v;}
    void setBody(const std::string& v) { m_body = v;}

    const std::vector<std::string>& getToEMailAddress() const { return m_toEMailAddress;}
    const std::vector<std::string>& getCcEMailAddress() const { return m_ccEMailAddress;}
    const std::vector<std::string>& getBccEMailAddress() const { return m_bccEMailAddress;}

    void setToEMailAddress(const std::vector<std::string>& v) { m_toEMailAddress = v;}
    void setCcEMailAddress(const std::vector<std::string>& v) { m_ccEMailAddress = v;}
    void setBccEMailAddress(const std::vector<std::string>& v) { m_bccEMailAddress = v;}

    void addEntity(EMailEntity::ptr entity);
    const std::vector<EMailEntity::ptr>& getEntitys() const { return m_entitys;}
private:
    std::string m_fromEMailAddress;
    std::string m_fromEMailPasswd;
    std::string m_title;
    std::string m_body;
    std::vector<std::string> m_toEMailAddress;
    std::vector<std::string> m_ccEMailAddress;
    std::vector<std::string> m_bccEMailAddress;
    std::vector<EMailEntity::ptr> m_entitys;
};

}

#endif
