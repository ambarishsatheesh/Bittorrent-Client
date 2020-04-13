#pragma once
#include <string>
#include <boost/lexical_cast.hpp>
#include "ValueTypes.h"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/posix_time/conversion.hpp"
#include <fstream>
#include <cerrno>

std::string get_file_contents(const char* filename)
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

boost::posix_time::ptime unixTime_to_ptime(std::time_t unixTime)
{
	auto result = boost::posix_time::from_time_t(unixTime);
	std::cout << result << std::endl;
	return result;
}

std::time_t ptime_to_unixTime(boost::posix_time::ptime ptime)
{
	auto result = boost::posix_time::to_time_t(ptime);
	std::cout << result << std::endl;
	return result;
}