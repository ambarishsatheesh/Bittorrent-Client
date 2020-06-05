#pragma once

#include "ValueTypes.h"
#include "Tokenizer.h"
#include "loguru.h"

#include <string>
#include <deque>
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Bittorrent
{
	namespace Decoder
	{
		static value decode(const std::string& encoded);
		static value decodeInt(std::deque<std::string>& tokens, bool& isFail);
		static value decodeString(std::deque<std::string>& tokens);
		static value decodeNext(std::deque<std::string>& tokens, bool& isFail);
		static value decodeList(std::deque<std::string>& tokens, bool& isFail);
		static value decodeDict(std::deque<std::string>& tokens, bool& isFail);

		value decode(const std::string& encoded)
		{
			if (encoded.empty())
			{
				
			}

			std::deque<std::string> tokens;
			bool isFail = false;
			Tokenizer::tokenize(encoded, tokens, isFail);

			if (isFail)
			{
				return value(0);
			}

			return decodeNext(tokens, isFail);
		}

		value decodeNext(std::deque<std::string>& tokens, bool& isFail)
		{
			if (tokens.empty())
			{
				LOG_F(ERROR, "Encoding error: Encoded data is too short");
				isFail = true;
				return value(0);
			}

			if (tokens.front() == "i")
			{
				return decodeInt(tokens, isFail);
			}
			else if (tokens.front() == "s")
			{
				return decodeString(tokens);
			}
			else if (tokens.front() == "l")
			{
				return decodeList(tokens, isFail);
			}
			else if (tokens.front() == "d")
			{
				return decodeDict(tokens, isFail);
			}

			return value(0);
		}

		value decodeInt(std::deque<std::string>& tokens, bool& isFail)
		{

			if (tokens.size() < 3)
			{
				LOG_F(ERROR, "Encoding error: Encoded data is too short!");
				isFail = true;
				return value(0);
			}


			if (tokens.at(2) != "e")
			{
				LOG_F(ERROR, "Encoding error: Encoded integer does not end with 'e'!");
				isFail = true;
				return value(0);
			}
				

			if (tokens.at(1) == "-0")
			{
				LOG_F(ERROR, "Encoding error: Negative zero is not allowed!");
				isFail = true;
				return value(0);
			}

			try {
				long long valueInt = boost::lexical_cast<long long>(tokens.at(1));
				tokens.pop_front(); // eat the "i"
				tokens.pop_front(); // eat the the number
				tokens.pop_front(); // eat the "e"
				return value(valueInt);
			}
			catch (boost::bad_lexical_cast&) 
			{
				LOG_F(ERROR, "Encoding error: Incorrect integer: %s", 
					tokens.at(1).c_str());
				isFail = true;
				return value(0);
			}
		}

		value decodeString(std::deque<std::string>& tokens)
		{
			tokens.pop_front(); // eat the "s"
			std::string valueString = tokens.front();
			tokens.pop_front(); // eat the value
			return value(valueString);
		}


		value decodeList(std::deque<std::string>& tokens, bool& isFail)
		{
			valueList vec;
			tokens.pop_front(); // eat the "l"
			while (tokens.front() != "e")
			{
				vec.push_back(decodeNext(tokens, isFail));
				if (isFail)
				{
					return vec;
				}
			}

			tokens.pop_front(); // eat the "e"

			return vec;
		}


		value decodeDict(std::deque<std::string>& tokens, bool& isFail)
		{
			// Make a list
			valueList vec = boost::get<valueList>(decodeList(tokens, isFail));

			if (isFail)
			{
				return value(0);
			}

			valueDictionary dict;

			for (size_t i = 0; i < vec.size(); i += 2)
			{
				dict.emplace(boost::get<std::string>(vec.at(i)), vec.at(i + 1));
			}

			return dict;
		}

	}
}
