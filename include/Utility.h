#pragma once
#include <string>

#include <fstream>
#include <iostream>
#include <cerrno>
#include <random>
#include <algorithm>
#include <iomanip>
#include <unordered_map>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "ValueTypes.h"
#include "loguru.h"

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
                    LOG_F(INFO, "Invalid file path: %s!", filePath);
                    return "FAIL!";
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
                LOG_F(INFO, "Invalid file path: %s!", filePath);
                return "FAIL!";
            }
            //remove extension
            const auto periodIndex = strPath.rfind('.');
            if (std::string::npos != periodIndex)
            {
                strPath.erase(periodIndex);
            }
            else
            {
                LOG_F(INFO, "No file extension!: %s!", filePath);
                return "FAIL!";
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
                LOG_F(INFO, "Invalid file path: %s!", filePath);
                return "FAIL!";
            }
            //check extension
            const auto periodIndex = strPath.rfind('.');
            if (std::string::npos == periodIndex)
            {
                LOG_F(INFO, "No file extension!: %s!", filePath);
                return "FAIL!";
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

                //handle empty file
                if (read.tellg() == 0)
                {
                    LOG_F(ERROR, "Torrent file is empty!");
                    return contents;
                }

                contents.resize(read.tellg());
                read.seekg(0, std::ios::beg);
                read.read(&contents.at(0), contents.size());
                read.close();
                return(contents);
            }

            LOG_F(INFO, "Error loading from file \"%s\"!", filename);
            return "FAIL!";
        }


        inline void saveToFile(std::string fullFilePath, std::string encoded)
        {
            std::ofstream file(fullFilePath, std::ios::binary);
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
            int multiple = static_cast<int>(std::log(bytes) / std::log(1024));
            float value = static_cast<float>(bytes / std::pow(1024, multiple));
            float multiplier = static_cast<float>(std::pow(10, 2));
            const float res = (std::round(value * multiplier)) / multiplier;

            std::ostringstream streamObj;
            streamObj << std::fixed << std::setprecision(2) << res;

            return streamObj.str() + " " + units.at(multiple);
        }

        //used http://torrentinvites.org/f29/piece-size-guide-167985/
        inline long long recommendedPieceSize(long long totalSize)
        {
            if (totalSize <= 367001600)
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

        //get hex representation of data
        inline std::string toHex(std::vector<byte> data)
        {
            std::string hex_res;
            for (auto i : data) {
                std::ostringstream oss;
                oss << std::hex << std::setw(2) << std::setfill('0') << (unsigned)i;
                hex_res += oss.str();
            }
            return hex_res;
        }

        inline void saveLogOrderByThread(const char* inputLog, 
            const char* outputLog)
        {
            std::ifstream file(inputLog);
            std::string logLine;
            std::string threadName;
            std::unordered_multimap<std::string, std::string> logByThreadMap;

            while (std::getline(file, logLine)) {
                auto brack1 = logLine.find_first_of("[");
                brack1++;
                auto brack2 = logLine.find_last_of("]");
                if (brack1 != std::string::npos && brack2 != std::string::npos
                    && logLine.find("File:Line") == std::string::npos)
                {
                    threadName = logLine.substr(brack1, brack2 - (brack1));
                    logByThreadMap.emplace(threadName, logLine);
                }
            }

            std::ofstream outFile(outputLog, std::ofstream::out);
            for (auto thread : logByThreadMap)
            {
                outFile << thread.second << "\n";
            }
        }


    }
}
