#pragma once
#include <string>
#include <boost/lexical_cast.hpp>
#include "ValueTypes.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/conversion.hpp"
#include <fstream>
#include <cerrno>

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
}
