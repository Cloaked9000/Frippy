//
// Created by fred on 01/12/2020.
//

#ifndef CLIPUPLOAD_UPLOADER_H
#define CLIPUPLOAD_UPLOADER_H

#include <memory>
#include <vector>
#include <frnetlib/Socket.h>
#include <frnetlib/SSLContext.h>

class Uploader
{
public:
    Uploader();
    std::string upload(const std::string &url, const std::unordered_map<std::string, std::string> &headers, const std::string &data);

private:
    bool load_system_ca(const std::shared_ptr<fr::SSLContext>& ssl_context);
    std::shared_ptr<fr::Socket> create_socket(bool is_ssl);

    std::shared_ptr<fr::SSLContext> ssl_context;
};


#endif //CLIPUPLOAD_UPLOADER_H
