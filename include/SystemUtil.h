//
// Created by fred on 01/12/2020.
//

#ifndef CLIPUPLOAD_SYSTEMUTIL_H
#define CLIPUPLOAD_SYSTEMUTIL_H


class SystemUtil
{
public:
    static bool does_file_exist(const std::string &filepath)
    {
        struct stat st = {};
        return stat(filepath.c_str(), &st) == 0;
    }

    static std::string read_file(const std::string &path)
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

    static void write_file(const std::string &path, const std::string &data)
    {
        std::ofstream stream(path.c_str());
        if(!stream.is_open())
            throw std::runtime_error("Failed to open file '" + path + "' for writing: " + strerror(errno));
        stream.write(data.c_str(), data.size());
        if(!stream.good())
            throw std::runtime_error("Failed to write to '" + path + "': " + strerror(errno));
    }

};


#endif //CLIPUPLOAD_SYSTEMUTIL_H
