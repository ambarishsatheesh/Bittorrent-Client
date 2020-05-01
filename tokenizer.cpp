#include "Tokenizer.h"
#include <assert.h>
#include <boost/regex.hpp>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <string>

void Tokenizer::tokenize(const std::string& encoded,
	std::deque<std::string>& tokens)
{
	boost::regex expr("([idel])|(\\d+):|(-?\\d+)");
	//container to store regex matches on string objects
	boost::smatch regStore;
	//Instructs the matching engine to retain all available capture 
	//information; information about repeated capturing groups is available
	boost::match_flag_type matchFlag = boost::match_extra;

	size_t encodedPos = 0;
	auto encodedLen = encoded.size();
	while (encodedPos < encodedLen)
	{
		auto currentStr = encoded.substr(encodedPos, encodedLen - encodedPos);
		if (boost::regex_search(currentStr, regStore, expr, matchFlag))
		{
			//if string
			if (!regStore[2].str().empty())
			{
				//+1 to skip colon
				size_t strStart = regStore[2].str().size() + 1;
				//get length of string (e.g. get "5" from "l5:string") 
				size_t strSize = boost::lexical_cast<int>(regStore[2].str());
 				tokens.push_back("s");
				if (strStart + strSize > currentStr.size())
				{
					tokens.clear();
					throw std::invalid_argument("String is incorrectly sized!");
				}

				auto curToken = currentStr.substr(strStart, strSize);

				if (curToken == "-0")
				{
					throw std::invalid_argument("Decoding error: negative zero is not allowed in encoded data!");
				}
				tokens.push_back(curToken);
				encodedPos += strStart + strSize;
			}
			//if not string
			else 
			{
				//convert to long long if number, else push string
	/*			long long res;
				if (boost::conversion::try_lexical_convert<long long>(regStore[0].str(), res))
				{
					tokens.push_back(res);
				}
				else
				{
					tokens.push_back(regStore[0].str());
				}*/
				tokens.push_back(regStore[0].str());
				encodedPos += regStore[0].str().size();
			}
		}
		else
		{
			tokens.clear();
			throw std::invalid_argument("Invalid input format!");
		}
	}
}
