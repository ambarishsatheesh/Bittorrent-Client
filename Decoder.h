#pragma once
#include "ValueTypes.h"
#include <string>
#include <deque>

class Decoder
{
public:
	static value decode(const std::string& encoded);

private:
	//use deque instead of vector because we pop elements from the front
	static value decode(std::deque<std::string>& tokens);

	static value decodeInt(std::deque<std::string>& tokens);

	static value decodeString(std::deque<std::string>& tokens);

	static value decodeList(std::deque<std::string>& tokens);

	static value decodeDict(std::deque<std::string>& tokens);

	Decoder();
};



