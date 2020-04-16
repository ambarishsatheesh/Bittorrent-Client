#pragma once
#include <string>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <cerrno>

#include "ValueTypes.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/conversion.hpp"

namespace utility 
{
    inline std::string get_file_contents(const char* filename)
    {
        std::ifstream read(filename, std::ios::in | std::ios::binary);
        if (read)
        {
            std::string contents;
            read.seekg(0, std::ios::end);
            contents.resize(read.tellg());
            read.seekg(0, std::ios::beg);
            read.read(&contents[0], contents.size());
            read.close();
            return(contents);
        }
        throw(errno);
    }

    inline boost::posix_time::ptime unixTime_to_ptime(std::time_t unixTime)
    {
        auto result = boost::posix_time::from_time_t(unixTime);
        std::cout << result << std::endl;
        return result;
    }

    inline std::time_t ptime_to_unixTime(boost::posix_time::ptime ptime)
    {
        auto result = boost::posix_time::to_time_t(ptime);
        std::cout << result << std::endl;
        return result;
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

    inline std::string humanReadableBytes(long bytes)
    {
        std::vector<std::string> units = { "B", "KiB", "MiB", "GiB", "TiB" };
        if (bytes == 0)
        {
            return "0" + units.at(0);
        }
        const int multiple = static_cast<const int>(std::log(bytes) / std::log(1024));
        const float value = static_cast<const float>(bytes / std::pow(1024, multiple));
        const float multiplier = static_cast<const float>(std::pow(10, 2));
        const float res = (std::round(value * multiplier))/ multiplier;
        return boost::lexical_cast<std::string>(res) + units.at(multiple);
    }

}