//
// Created by fred on 01/12/2020.
//

#include <frnetlib/TcpSocket.h>
#include <frnetlib/SSLSocket.h>
#include <frnetlib/URL.h>
#include <frnetlib/HttpRequest.h>
#include <frnetlib/HttpResponse.h>
#include <cassert>
#include "Uploader.h"

#define SSL_PORT "443"
#define CONNECTION_TIMEOUT_SECS 5

Uploader::Uploader()
{
    ssl_context = std::make_shared<fr::SSLContext>();
    if(!load_system_ca(ssl_context))
        throw std::runtime_error("Failed to load system SSL cert.");
}

std::string Uploader::upload(const std::string &url, const std::unordered_map<std::string, std::string> &headers, const std::string &data)
{
    //Establish a connection with the upload server
    fr::URL parsed_url(url);
    std::shared_ptr<fr::Socket> socket = create_socket(parsed_url.get_port() == SSL_PORT);

    fr::Socket::Status status = socket->connect(parsed_url.get_host(), parsed_url.get_port(), {std::chrono::seconds(CONNECTION_TIMEOUT_SECS)});
    if(status != fr::Socket::Status::Success)
    {
        throw std::runtime_error("Failed to connect: " + fr::Socket::status_to_string(status));
    }

    fr::HttpRequest request;
    request.set_uri(parsed_url.get_uri());
    for(auto &iter : headers)
    {
        request.header(iter.first) = iter.second;
    }
    request.set_body(data);

    status = socket->send(request);
    if(status != fr::Socket::Status::Success)
    {
        throw std::runtime_error("Failed to send request: " + fr::Socket::status_to_string(status));
    }

    fr::HttpResponse response;
    status = socket->receive(response);
    if(status != fr::Socket::Status::Success)
    {
        throw std::runtime_error("Failed to receive response: " + fr::Socket::status_to_string(status));
    }

    if(response.get_status() != fr::Http::RequestStatus::Ok)
    {
        throw std::runtime_error("Upload failed: " + std::to_string((int)response.get_status()) + " response code!");
    }

    return response.get_body();
}

bool Uploader::load_system_ca(const std::shared_ptr<fr::SSLContext>& context)
{
    static const std::vector<std::string> possible_locations = {"/etc/ssl/certs/ca-certificates.crt",                 // Debian/Ubuntu/Gentoo etc.
                                                                "/etc/pki/tls/certs/ca-bundle.crt",                   // Fedora/RHEL 6
                                                                "/etc/ssl/ca-bundle.pem",                             // OpenSUSE
                                                                "/etc/pki/tls/cacert.pem",                            // OpenELEC
                                                                "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"}; // CentOS/RHEL 7
    assert(context);
    for(auto &loc : possible_locations)
    {
        if(context->load_ca_certs_from_file(loc))
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<fr::Socket> Uploader::create_socket(bool is_ssl)
{
    if(is_ssl)
        return std::make_shared<fr::SSLSocket>(ssl_context);
    return std::make_shared<fr::TcpSocket>();
}


