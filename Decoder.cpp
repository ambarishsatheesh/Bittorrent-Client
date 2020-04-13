#include "Decoder.h"
#include "Tokenizer.h"
#include "boostVisitor.h"
#include <boost/lexical_cast.hpp>
#include <iostream>

value Decoder::decode(const std::string& encoded)
{
	std::deque<std::string> tokens;
	Tokenizer::tokenize(encoded, tokens);

	return decode(tokens);
}

value Decoder::decode(std::deque<std::string>& tokens)
{
	if (tokens.empty())
		throw std::invalid_argument("Encoded data is too short");

	if (tokens.front() == "i")
		return decodeInt(tokens);
	else if (tokens.front() == "s")
		 return decodeString(tokens);
	else if (tokens.front() == "l")
		return decodeList(tokens);
	else if (tokens.front() == "d")
		return decodeDict(tokens);

	return value(0);
}


value Decoder::decodeInt(std::deque<std::string>& tokens)
{

	if (tokens.size() < 3)
		throw std::invalid_argument("Encoded data is too short");

	if (tokens[2] != "e")
		throw std::invalid_argument("Incorrect encoding: does not end with 'e'");

	if (tokens[1] == "-0")
		throw std::invalid_argument("Negative zero is not allowed");

	try {
		integer valueInt = boost::lexical_cast<integer>(tokens[1]);
		tokens.pop_front(); // eat the "i"
		tokens.pop_front(); // eat the the number
		tokens.pop_front(); // eat the "e"
		return value(valueInt);
	}
	catch (boost::bad_lexical_cast&) {
		throw std::invalid_argument("Incorrect integer: " + tokens[1]);
	}
}

value Decoder::decodeString(std::deque<std::string>& tokens)
{
	tokens.pop_front(); // eat the "s"
	std::string valueString = tokens.front();
	tokens.pop_front(); // eat the value
	return value(valueString);
}


value Decoder::decodeList(std::deque<std::string>& tokens)
{

	valueList vec;
	tokens.pop_front(); // eat the "l"
	while (tokens.front() != "e")
		vec.push_back(decode(tokens));

	tokens.pop_front(); // eat the "e"

	return vec;
}


value Decoder::decodeDict(std::deque<std::string>& tokens)
{

	// Make a list
	valueList vec = boost::get<valueList>(decodeList(tokens));

	valueDictionary dict;
	for (unsigned int i = 0; i < vec.size(); i += 2)
		dict.emplace(boost::get<std::string>(vec[i]), vec[i + 1]);

	//std::cout << dict.at("url-list") << std::endl;

	return dict;
}
