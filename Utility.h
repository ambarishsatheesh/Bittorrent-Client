#pragma once
#include <string>

#include <fstream>
#include <iostream>
#include <cerrno>
#include <random>
#include <algorithm>

#include <boost/uuid/detail/sha1.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "ValueTypes.h"

namespace Bittorrent
{

    namespace utility
    {

        inline std::string getFileDirectory(const char* filePath)
        {
            if (boost::filesystem::is_directory(filePath))
            {
                return filePath;
            }
            else
            {
                std::string strPath = filePath;
                //remove name and extension
                const auto lastSlashIndex = strPath.find_last_of("/\\");
                if (std::string::npos != lastSlashIndex)
                {
                    return strPath.substr(0, lastSlashIndex + 1);
                }
                else
                {
                    throw std::invalid_argument("Invalid file path! No slashes!");
                }
            }
        }

        inline std::string getFileName(const char* filePath)
        {
            std::string strPath = filePath;
            //remove slashes
            const auto lastSlashIndex = strPath.find_last_of("/\\");
            if (std::string::npos != lastSlashIndex)
            {
                strPath.erase(0, lastSlashIndex + 1);
            }
            else
            {
                throw std::invalid_argument("Invalid file path! No slashes!");
            }
            //remove extension
            const auto periodIndex = strPath.rfind('.');
            if (std::string::npos != periodIndex)
            {
                strPath.erase(periodIndex);
            }
            else
            {
                throw std::invalid_argument("No file extension!");
            }
            return strPath;
        }

        inline std::string getFileNameAndExtension(const char* filePath)
        {
            std::string strPath = filePath;
            //remove slashes
            const auto lastSlashIndex = strPath.find_last_of("/\\");
            if (std::string::npos != lastSlashIndex)
            {
                strPath.erase(0, lastSlashIndex + 1);
            }
            else
            {
                throw std::invalid_argument("Invalid file path! No slashes!");
            }
            //check extension
            const auto periodIndex = strPath.rfind('.');
            if (std::string::npos == periodIndex)
            {
                throw std::invalid_argument("No file extension!");
            }
            return strPath;
        }

        inline std::string loadFromFile(const char* filename)
        {
            std::ifstream read(filename, std::ios::in | std::ios::binary);
            if (read)
            {
                std::string contents;
                read.seekg(0, std::ios::end);
                contents.resize(read.tellg());
                read.seekg(0, std::ios::beg);
                read.read(&contents.at(0), contents.size());
                read.close();
                return(contents);
            }
            throw(errno);
        }


        inline void saveToFile(std::string fullFilePath, std::string encoded)
        {
            std::ofstream file(fullFilePath);
            file << encoded;
            file.close();
        }

        inline bool is10x(int a)
        {
            int bit1 = (a >> 7) & 1;
            int bit2 = (a >> 6) & 1;
            return (bit1 == 1) && (bit2 == 0);
        }

        inline bool isValidUtf8(std::vector<int>& data)
        {
            for (int i = 0; i < data.size(); ++i) {
                // 0xxxxxxx
                int bit1 = (data[i] >> 7) & 1;
                if (bit1 == 0) continue;
                // 110xxxxx 10xxxxxx
                int bit2 = (data[i] >> 6) & 1;
                if (bit2 == 0) return false;
                // 11
                int bit3 = (data[i] >> 5) & 1;
                if (bit3 == 0) {
                    // 110xxxxx 10xxxxxx
                    if ((++i) < data.size()) {
                        if (is10x(data[i])) {
                            continue;
                        }
                        return false;
                    }
                    else {
                        return false;
                    }
                }
                int bit4 = (data[i] >> 4) & 1;
                if (bit4 == 0) {
                    // 1110xxxx 10xxxxxx 10xxxxxx
                    if (i + 2 < data.size()) {
                        if (is10x(data[i + 1]) && is10x(data[i + 2])) {
                            i += 2;
                            continue;
                        }
                        return false;
                    }
                    else {
                        return false;
                    }
                }
                int bit5 = (data[i] >> 3) & 1;
                if (bit5 == 1) return false;
                if (i + 3 < data.size()) {
                    if (is10x(data[i + 1]) && is10x(data[i + 2]) && is10x(data[i + 3])) {
                        i += 3;
                        continue;
                    }
                    return false;
                }
                else {
                    return false;
                }
            }
            return true;
        }

        inline std::string urlEncode(std::string& s)
        {
            static std::vector<char> lookup = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
            std::stringstream e;
            for (size_t i = 0; i < s.size(); i++)
            {
                const char& c = s.at(i);
                if ((48 <= c && c <= 57) ||//0-9
                    (65 <= c && c <= 90) ||//abc...xyz
                    (97 <= c && c <= 122) || //ABC...XYZ
                    (c == '-' || c == '_' || c == '.' || c == '~')
                    )
                {
                    e << c;
                }
                else
                {
                    e << '%';
                    e << lookup.at((c & 0xF0) >> 4);
                    e << lookup.at((c & 0x0F));
                }
            }
            return e.str();
        }

        inline std::string humanReadableBytes(long long bytes)
        {
            std::vector<std::string> units = { "B", "KiB", "MiB", "GiB", "TiB" };
            if (bytes == 0)
            {
                return "0" + units.at(0);
            }
            const int multiple = static_cast<const int>(std::log(bytes) / std::log(1024));
            const float value = static_cast<const float>(bytes / std::pow(1024, multiple));
            const float multiplier = static_cast<const float>(std::pow(10, 2));
            const float res = (std::round(value * multiplier)) / multiplier;
            return boost::lexical_cast<std::string>(res) + units.at(multiple);
        }

        //used http://torrentinvites.org/f29/piece-size-guide-167985/
        inline long long recommendedPieceSize(long long totalSize)
        {
            //up to 50MiB
            if (totalSize <= 52428800)
            {
                return 32768;
            }
            //50 to 150
            if (52428800 < totalSize && totalSize <= 157286400)
            {
                return 65536;
            }
            //150 to 350
            if (157286400 < totalSize && totalSize <= 367001600)
            {
                return 131072;
            }
            //350 to 512
            if (367001600 < totalSize && totalSize <= 536870912)
            {
                return 262144;
            }
            //512MiB to 1GiB
            if (536870912 < totalSize && totalSize <= 1073741824)
            {
                return 524288;
            }
            //1 to 2
            if (1073741824 < totalSize && totalSize <= 2147483648)
            {
                return 1048576;
            }
            //2 to 4
            if (2147483648 < totalSize && totalSize <= 4294967296)
            {
                return 2097152;
            }
            //4 to 8
            if (4294967296 < totalSize && totalSize <= 8589934592)
            {
                return 4194304;
            }
            //8 to 16
            if (8589934592 < totalSize && totalSize <= 17179869184)
            {
                return 8388608;
            }
            //16 to 32
            if (17179869184 < totalSize && totalSize <= 34359738368)
            {
                return 16777216;
            }
        }

    }
}