#pragma once

#include "ValueTypes.h"

//compile-time checked visitation of each bounded type
class encodeVisitor :
	public boost::static_visitor<std::string>
{
public:
	encodeVisitor();
	std::string operator()(const integer& inputObj) const;
	std::string operator()(const std::string& inputObj) const;
	std::string operator()(const valueDictionary& inputObj) const;
	std::string operator()(const valueList& inputObj) const;
};
