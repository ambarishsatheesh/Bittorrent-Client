#pragma once
#include <string>

class trackerObj
{
public:
	std::string trackerAddress;
	trackerObj(std::string address) : trackerAddress{ address } {}
};