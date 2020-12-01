#include <Clipboard.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <nlohmann/json.hpp>
#include <frnetlib/TcpSocket.h>
#include <frnetlib/HttpRequest.h>
#include <frnetlib/HttpResponse.h>
#include <frnetlib/SSLSocket.h>
#include <frnetlib/URL.h>
#include <Notifier.h>
#include <Keyboard.h>

#define CONFIG_PATH "config.json"
using json = nlohmann::json;

static const char *default_config = "{\n"
                              "    \"url\": \"\",\n"
                              "    \"password\": \"\",\n"
                              "    \"priority\": [{\"type\": \"image/png\", \"extension\": \"png\"},\n"
                              "                 {\"type\": \"image/jpeg\", \"extension\": \"jpg\"},\n"
                              "                 {\"type\": \"image/bmp\", \"extension\": \"bmp\"},\n"
                              "                 {\"type\": \"image/tiff\", \"extension\": \"tiff\"},\n"
                              "                 {\"type\": \"video/webm\", \"extension\": \"webm\"},\n"
                              "                 {\"type\": \"video/html\", \"extension\": \"html\"},\n"
                              "                 {\"type\": \"text/plain\", \"extension\": \"txt\"},\n"
                              "                 {\"type\": \"UTF8_STRING\", \"extension\": \"txt\"}]\n"
                              "}";

bool does_file_exist(const std::string &filepath)
{
    struct stat st = {};
    return stat(filepath.c_str(), &st) == 0;
}

std::string read_file(const std::string &path)
{
    //Open the file
    std::string data;
    std::ifstream stream(path);
    if(!stream.is_open())
        throw std::runtime_error("Failed to open path '" + path + "': " + strerror(errno));

    //Figure out its size and resize buffers appropriately
    stream.seekg(0, std::ios::end);
    data.resize(stream.tellg());
    stream.seekg(0, std::ios::beg);

    //Read in data
    stream.read(data.data(), data.size());

    if(!stream.good())
        throw std::runtime_error("Bad read from '" + path + "': " + strerror(errno));

    return data;
}

void write_file(const std::string &path, const std::string &data)
{
    std::ofstream stream(path.c_str());
    if(!stream.is_open())
        throw std::runtime_error("Failed to open file '" + path + "' for writing: " + strerror(errno));
    stream.write(data.c_str(), data.size());
    if(!stream.good())
        throw std::runtime_error("Failed to write to '" + path + "': " + strerror(errno));
}

std::string get_type_extension(const std::vector<std::pair<std::string, std::string>> &xa_priority, const std::string &type)
{
    auto iter = std::find_if(xa_priority.begin(), xa_priority.end(), [&](const std::pair<std::string, std::string> &elem) {
        return elem.first == type;
    });
    if(iter == xa_priority.end())
        return "bin";
    return iter->second;
}

bool load_system_ca_store(const std::shared_ptr<fr::SSLContext>& context)
{
    static const std::vector<std::string> possible_locations = {"/etc/ssl/certs/ca-certificates.crt",                 // Debian/Ubuntu/Gentoo etc.
                                                                "/etc/pki/tls/certs/ca-bundle.crt",                   // Fedora/RHEL 6
                                                                "/etc/ssl/ca-bundle.pem",                             // OpenSUSE
                                                                "/etc/pki/tls/cacert.pem",                            // OpenELEC
                                                                "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem"}; // CentOS/RHEL 7
    for(auto &loc : possible_locations)
    {
        if(context->load_ca_certs_from_file(loc))
        {
            return true;
        }
    }
    return false;
}

std::string get_file_mimetype(const std::vector<std::pair<std::string, std::string>> &xa_priority, const std::string &filepath)
{
    std::cout << "Filepath: " << filepath << std::endl;
    auto pos = filepath.find_last_of('.');
    std::string extension = pos == std::string::npos ? "txt" : filepath.substr(pos + 1);
    auto iter = std::find_if(xa_priority.begin(), xa_priority.end(), [&](const std::pair<std::string, std::string> &elem) {
        return elem.second == extension;
    });
    if(iter == xa_priority.end())
        return "application/octet-stream";
    return iter->first;
}

void do_upload_clipboard(Clipboard &clipboard, const std::string &url, const std::string &password, const std::vector<std::pair<std::string, std::string>> &xa_priority)
{
    //Read clipboard
    Clipboard::ClipboardRead clip;
    try
    {
        clip = clipboard.read_clipboard();
        if(clip.data.empty())
            return;
    }
    catch(const std::exception &e)
    {
        Notifier::notify("Upload Failed", e.what());
        return;
    }


    //If it's a filepath, read in the image. Raw image data wont begin with this.
    std::cout << "Data: " << clip.data << std::endl;
    if(clip.data.starts_with("file://"))
    {
        clip.data.erase(0, 7);
    }

    if(clip.data.starts_with('/'))
    {
        clip.type = get_file_mimetype(xa_priority, clip.data);
        clip.data = read_file(clip.data);
    }

    //Establish a connection with the upload server
    fr::URL parsed_url(url);
    std::shared_ptr<fr::Socket> socket;
    if(parsed_url.get_port() == "443")
    {
        auto ssl_context = std::make_shared<fr::SSLContext>();
        if(!load_system_ca_store(ssl_context))
            throw std::runtime_error("Failed to load system CA store!");
        socket = std::make_shared<fr::SSLSocket>(ssl_context);
    }
    else
    {
        socket = std::make_shared<fr::TcpSocket>();
    }


    fr::Socket::Status status = socket->connect(parsed_url.get_host(), parsed_url.get_port(), {std::chrono::seconds(5)});
    if(status != fr::Socket::Status::Success)
    {
        Notifier::notify("Upload Failed","Failed to connect: " + fr::Socket::status_to_string(status));
        return;
    }

    fr::HttpRequest request;
    request.set_uri(parsed_url.get_uri());
    request.header("api-key") = password;
    request.header("file-type") = get_type_extension(xa_priority, clip.type);
    request.set_body(clip.data);

    status = socket->send(request);
    if(status != fr::Socket::Status::Success)
    {
        Notifier::notify("Upload Failed","Failed to send request: " + fr::Socket::status_to_string(status));
        return;
    }

    fr::HttpResponse response;
    status = socket->receive(response);
    if(status != fr::Socket::Status::Success)
    {
        Notifier::notify("Upload Failed","Failed to receive request: " + fr::Socket::status_to_string(status));
        return;
    }

    if(response.get_status() != fr::Http::RequestStatus::Ok)
    {
        Notifier::notify("Upload Failed", std::to_string((int)response.get_status()) + " response code!");
        return;
    }

    json json_response = json::parse(response.get_body());
    std::string download_link = json_response.at("download-link");
    Notifier::notify("Your Link", "<a href=\"" + download_link + "\"> " + download_link + "</a>", std::chrono::seconds(10));
}

int main()
{
    //Check if config exists, write a blank one if it doesn't
    if(!does_file_exist(CONFIG_PATH))
    {
        write_file(CONFIG_PATH, default_config);
        Notifier::notify("Default Config Created", "Created a default config file. Please fill it in.");
        return 0;
    }

    //Read in config
    json config = json::parse(read_file(CONFIG_PATH));
    std::string url = config.at("url");
    std::string password = config.at("password");
    std::vector<std::pair<std::string, std::string>> xa_priority;
    const auto &priority = config.at("priority");
    for(auto &elem : priority)
        xa_priority.emplace_back(elem.at("type"), elem.at("extension"));

    //Listen for the shortcut, ctrl + shift + a
    Clipboard clipboard(xa_priority);
    Keyboard keyboard;
    keyboard.bind_key("A", Keyboard::KeyModifier{1, 1, 0, 0});

    //Keep going in an infinite loop, waiting for the shortcut
    while(true)
    {
        std::string key = {};
        Keyboard::KeyModifier modifier = {};
        keyboard.wait_for_keys(key, modifier);
        if(modifier.key_pressed)
        {
           do_upload_clipboard(clipboard, url, password, xa_priority);
        }
    }
}