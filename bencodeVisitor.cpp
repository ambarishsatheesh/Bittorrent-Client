#include "ValueTypes.h"
#include "bencodeVisitor.h"
#include "Utility.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <iostream>

using namespace utility;

bencodeVisitor::bencodeVisitor(int indentation)
	: indentation_(indentation)
{
}

int bencodeVisitor::getIndentation() const
{
	return indentation_;
}

std::string bencodeVisitor::getSpace() const
{
	return std::string(getIndentation() * 2, ' ');
}

bool bencodeVisitor::isUTF8(const std::string& value) const
{
	std::vector<int> data;
	for (unsigned int i = 0; i < value.size(); i++)
	{
		data.push_back(static_cast<int>(value[i]));
	}
	if (!isValidUtf8(data))
	{
		return false;
	}
	return true;
}

void bencodeVisitor::operator()(const integer t) const
{
	std::cout << t << std::endl;
}

void bencodeVisitor::operator()(const std::string& t) const
{
	if (!isUTF8(t))
		std::cout << "BINARY DATA (length: " << t.size() << ")" << std::endl;
	else {
		const int MAX_STRING_LENGTH = 100;
		std::cout << t.substr(0, MAX_STRING_LENGTH) << std::endl;
		//t = t;
	}
}

void bencodeVisitor::operator()(const valueDictionary& t) const
{
	std::cout << std::endl;
	//boost::apply_visitor(bencodeVisitor(), t.at("announce"));
;	for (valueDictionary::const_iterator i = t.begin(); i != t.end(); ++i) {
		std::cout << getSpace() << "{" << i->first << "}: ";
		boost::apply_visitor(bencodeVisitor(getIndentation() + 1), i->second);
	}
}

void bencodeVisitor::operator()(const valueList& t) const
{
	std::cout << std::endl;
	for (unsigned int i = 0; i != t.size(); ++i) {
		boost::apply_visitor(bencodeVisitor(getIndentation() + 1), t[i]);
	}
}