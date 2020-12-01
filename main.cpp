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
#include <Uploader.h>
#include <SystemUtil.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
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

std::string get_type_extension(const std::vector<std::pair<std::string, std::string>> &xa_priority, const std::string &type)
{
    auto iter = std::find_if(xa_priority.begin(), xa_priority.end(), [&](const std::pair<std::string, std::string> &elem) {
        return elem.first == type;
    });
    if(iter == xa_priority.end())
        return "bin";
    return iter->second;
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

Clipboard::Target choose_best_conversion_target(const std::vector<std::pair<std::string, std::string>> &pref, const std::vector<Clipboard::Target> &available)
{
    //We basically need to loop over the list of possible clipboard conversions, and then pick the one (if any)
    //that's furthest up our priority list.
    auto score = std::numeric_limits<size_t>::max();
    size_t chosen_index = 0;

    for(size_t i = 0; i < available.size(); i++)
    {
        std::cout << "Available conversion: " << available[i].name << std::endl;
        if(auto iter = std::find_if(pref.begin(), pref.end(), [&](const std::pair<std::string, std::string> &elem) {return elem.first == available[i].name;}); iter != pref.end())
        {
            size_t this_score = std::distance(pref.begin(), iter);
            if(this_score < score)
            {
                score = this_score;
                chosen_index = i;
            }
        }
    }
    assert(chosen_index < available.size());

    std::cout << "Requesting as: " << available[chosen_index].name << "(" << chosen_index << ")" << std::endl;
    return available[chosen_index];
}

int main()
{
    //Check if config exists, write a blank one if it doesn't
    if(!SystemUtil::does_file_exist(CONFIG_PATH))
    {
        SystemUtil::write_file(CONFIG_PATH, default_config);
        Notifier::notify("Default Config Created", "Created a default config file. Please fill it in.");
        return 0;
    }

    //Read in config
    json config = json::parse(SystemUtil::read_file(CONFIG_PATH));
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
            auto list = clipboard.list_available_conversions();
            std::cout << "Available conversions: " << std::endl;

            auto best = choose_best_conversion_target(xa_priority, list);
            std::cout << "Requesting..." << std::endl;
            std::string clip_content;
            clipboard.read_clipboard(best, [&clip_content](const std::string &data) -> bool {
                clip_content.append(data);
                return true;
            });

            if(clip_content.starts_with("file://"))
            {
                clip_content.erase(0, 7);
                best.name = get_file_mimetype(xa_priority, clip_content);
                clip_content = SystemUtil::read_file(clip_content);
            }

            Uploader uploader;
            std::string response = uploader.upload(url, {{"api-key", password}, {"file-type", get_type_extension(xa_priority, best.name)}}, clip_content);
            json json_response = json::parse(response);
            std::string download_link = json_response.at("download-link");
            Notifier::notify("Your Link", "<a href=\"" + download_link + "\"> " + download_link + "</a>", std::chrono::seconds(10));
        }
    }
}
#pragma clang diagnostic pop