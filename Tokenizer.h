#pragma once
#include "ValueTypes.h"
#include <string>
#include <deque>

class Tokenizer
{
public:
	static void tokenize(const std::string& encoded,
		std::deque<std::string>& tokens);

private:
	Tokenizer();
};



