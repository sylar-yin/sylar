#include "uri.h"
#include <sstream>

namespace sylar {
%%{
    # See RFC 3986: http://www.ietf.org/rfc/rfc3986.txt

    machine uri_parser;

    gen_delims = ":" | "/" | "?" | "#" | "[" | "]" | "@";
    sub_delims = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=";
    reserved = gen_delims | sub_delims;
    unreserved = alpha | digit | "-" | "." | "_" | "~";
    pct_encoded = "%" xdigit xdigit;

    action marku { mark = fpc; }
    action markh { mark = fpc; }

    action save_scheme
    {
        uri->setScheme(std::string(mark, fpc - mark));
        mark = NULL;
    }

    scheme = (alpha (alpha | digit | "+" | "-" | ".")*) >marku %save_scheme;

    action save_port
    {
        if (fpc != mark) {
            uri->setPort(atoi(mark));
        }
        mark = NULL;
    }
    action save_userinfo
    {
        if(mark) {
            //std::cout << std::string(mark, fpc - mark) << std::endl;
            uri->setUserinfo(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }
    action save_host
    {
        if (mark != NULL) {
            //std::cout << std::string(mark, fpc - mark) << std::endl;
            uri->setHost(std::string(mark, fpc - mark));
        }
    }

    userinfo = (unreserved | pct_encoded | sub_delims | ":")*;
    dec_octet = digit | [1-9] digit | "1" digit{2} | 2 [0-4] digit | "25" [0-5];
    IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16 = xdigit{1,4};
    ls32 = (h16 ":" h16) | IPv4address;
    IPv6address = (                         (h16 ":"){6} ls32) |
                  (                    "::" (h16 ":"){5} ls32) |
                  ((             h16)? "::" (h16 ":"){4} ls32) |
                  (((h16 ":"){1} h16)? "::" (h16 ":"){3} ls32) |
                  (((h16 ":"){2} h16)? "::" (h16 ":"){2} ls32) |
                  (((h16 ":"){3} h16)? "::" (h16 ":"){1} ls32) |
                  (((h16 ":"){4} h16)? "::"              ls32) |
                  (((h16 ":"){5} h16)? "::"              h16 ) |
                  (((h16 ":"){6} h16)? "::"                  );
    IPvFuture = "v" xdigit+ "." (unreserved | sub_delims | ":")+;
    IP_literal = "[" (IPv6address | IPvFuture) "]";
    reg_name = (unreserved | pct_encoded | sub_delims)*;
    host = IP_literal | IPv4address | reg_name;
    port = digit*;

    authority = ( (userinfo %save_userinfo "@")? host >markh %save_host (":" port >markh %save_port)? ) >markh;

    action save_segment
    {
        mark = NULL;
    }

    action save_path
    {
            //std::cout << std::string(mark, fpc - mark) << std::endl;
        uri->setPath(std::string(mark, fpc - mark));
        mark = NULL;
    }


#    pchar = unreserved | pct_encoded | sub_delims | ":" | "@";
# add (any -- ascii) support chinese
    pchar         = ( (any -- ascii ) | unreserved | pct_encoded | sub_delims | ":" | "@" ) ;
    segment = pchar*;
    segment_nz = pchar+;
    segment_nz_nc = (pchar - ":")+;

    action clear_segments
    {
    }

    path_abempty = (("/" segment))? ("/" segment)*;
    path_absolute = ("/" (segment_nz ("/" segment)*)?);
    path_noscheme = segment_nz_nc ("/" segment)*;
    path_rootless = segment_nz ("/" segment)*;
    path_empty = "";
    path = (path_abempty | path_absolute | path_noscheme | path_rootless | path_empty);

    action save_query
    {
        //std::cout << std::string(mark, fpc - mark) << std::endl;
        uri->setQuery(std::string(mark, fpc - mark));
        mark = NULL;
    }
    action save_fragment
    {
        //std::cout << std::string(mark, fpc - mark) << std::endl;
        uri->setFragment(std::string(mark, fpc - mark));
        mark = NULL;
    }

    query = (pchar | "/" | "?")* >marku %save_query;
    fragment = (pchar | "/" | "?")* >marku %save_fragment;

    hier_part = ("//" authority path_abempty > markh %save_path) | path_absolute | path_rootless | path_empty;

    relative_part = ("//" authority path_abempty) | path_absolute | path_noscheme | path_empty;
    relative_ref = relative_part ( "?" query )? ( "#" fragment )?;

    absolute_URI = scheme ":" hier_part ( "?" query )? ;
    # Obsolete, but referenced from HTTP, so we translate
    relative_URI = relative_part ( "?" query )?;

    URI = scheme ":" hier_part ( "?" query )? ( "#" fragment )?;
    URI_reference = URI | relative_ref;
    main := URI_reference;
    write data;
}%%

Uri::ptr Uri::Create(const std::string& uristr) {
    Uri::ptr uri = std::make_shared<Uri>();
    int cs = 0;
    const char* mark = 0;
    %% write init;
    const char *p = uristr.c_str();
    const char *pe = p + uristr.size();
    const char* eof = pe;
    %% write exec;
    if(cs == uri_parser_error) {
        return nullptr;
    } else if(cs >= uri_parser_first_final) {
        return uri;
    }
    return nullptr;
}

Uri::Uri()
    :m_port(0) {
}

bool Uri::isDefaultPort() const {
    if(m_port == 0) {
        return true;
    }
    if(m_scheme == "http"
            || m_scheme == "ws") {
        return m_port == 80;
    } else if(m_scheme == "https"
            || m_scheme == "wss") {
        return m_port == 443;
    }
    return false;
}

const std::string& Uri::getPath() const {
    static std::string s_default_path = "/";
    return m_path.empty() ? s_default_path : m_path;
}

int32_t Uri::getPort() const {
    if(m_port) {
        return m_port;
    }
    if(m_scheme == "http"
        || m_scheme == "ws") {
        return 80;
    } else if(m_scheme == "https"
            || m_scheme == "wss") {
        return 443;
    }
    return m_port;
}

std::ostream& Uri::dump(std::ostream& os) const {
    os << m_scheme << "://"
       << m_userinfo
       << (m_userinfo.empty() ? "" : "@")
       << m_host
       << (isDefaultPort() ? "" : ":" + std::to_string(m_port))
       << getPath()
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment;
    return os;
}

std::string Uri::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

Address::ptr Uri::createAddress() const {
    auto addr = Address::LookupAnyIPAddress(m_host);
    if(addr) {
        addr->setPort(getPort());
    }
    return addr;
}

}
