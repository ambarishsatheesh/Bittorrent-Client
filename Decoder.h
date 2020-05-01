#include "ValueTypes.h"
#include <string>
#include <deque>
#include "Tokenizer.h"
#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Decoder
{
	static value decode(const std::string& encoded);
	static value decodeInt(std::deque<std::string>& tokens);
	static value decodeString(std::deque<std::string>& tokens);
	static value decodeNext(std::deque<std::string>& tokens);
	static value decodeList(std::deque<std::string>& tokens);
	static value decodeDict(std::deque<std::string>& tokens);

	value decode(const std::string& encoded)
	{
		std::deque<std::string> tokens;
		Tokenizer::tokenize(encoded, tokens);

		return decodeNext(tokens);
	}

	value decodeNext(std::deque<std::string>& tokens)
	{
		if (tokens.empty())
		{
			throw std::invalid_argument("Encoded data is too short");
		}

		if (tokens.front() == "i")
		{
			return decodeInt(tokens);
		}
		else if (tokens.front() == "s")
		{
			return decodeString(tokens);
		}
		else if (tokens.front() == "l")
		{
			return decodeList(tokens);
		}
		else if (tokens.front() == "d")
		{
			return decodeDict(tokens);
		}

		return value(0);
	}

	value decodeInt(std::deque<std::string>& tokens)
	{

		if (tokens.size() < 3)
			throw std::invalid_argument("Encoded data is too short");

		if (tokens.at(2) != "e")
			throw std::invalid_argument("Incorrect encoding: does not end with 'e'");

		if (tokens.at(1) == "-0")
			throw std::invalid_argument("Negative zero is not allowed");

		try {
			long long valueInt = boost::lexical_cast<long long>(tokens.at(1));
			tokens.pop_front(); // eat the "i"
			tokens.pop_front(); // eat the the number
			tokens.pop_front(); // eat the "e"
			return value(valueInt);
		}
		catch (boost::bad_lexical_cast&) {
			throw std::invalid_argument("Incorrect integer: " + tokens.at(1));
		}
	}

	value decodeString(std::deque<std::string>& tokens)
	{
		tokens.pop_front(); // eat the "s"
		std::string valueString = tokens.front();
		tokens.pop_front(); // eat the value
		return value(valueString);
	}


	value decodeList(std::deque<std::string>& tokens)
	{
		valueList vec;
		tokens.pop_front(); // eat the "l"
		while (tokens.front() != "e")
		{
			vec.push_back(decodeNext(tokens));
		}

		tokens.pop_front(); // eat the "e"

		return vec;
	}


	value decodeDict(std::deque<std::string>& tokens)
	{
		// Make a list
		valueList vec = boost::get<valueList>(decodeList(tokens));

		valueDictionary dict;
		for (size_t i = 0; i < vec.size(); i += 2)
		{
			dict.emplace(boost::get<std::string>(vec.at(i)), vec.at(i + 1));
		}

		return dict;
	}

}