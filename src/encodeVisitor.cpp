#include "encodeVisitor.h"
#include "Utility.h"

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include <iostream>


namespace Bittorrent
{
	using namespace utility;

	encodeVisitor::encodeVisitor()
	{

	}

	std::string encodeVisitor::operator()(const long long& inputObj) const
	{
		std::stringstream ss;
		ss << "i" << inputObj << "e";

		return ss.str();
	}

	std::string encodeVisitor::operator()(const std::string& inputObj) const
	{
		std::stringstream ss;
		ss << inputObj.size() << ":" << inputObj;

		return ss.str();
	}

	std::string encodeVisitor::operator()(const valueDictionary& inputObj) const
	{
		std::stringstream ss;
		ss << "d";
		for (valueDictionary::const_iterator i = inputObj.begin();
			i != inputObj.end(); ++i)
		{
			ss << (*this)(i->first) <<
				boost::apply_visitor(encodeVisitor(), i->second);
		}
		ss << "e";

		return ss.str();
	}

	std::string encodeVisitor::operator()(const valueList& inputObj) const
	{
		std::stringstream ss;
		ss << "l";
		for (valueList::const_iterator i = inputObj.begin(); i != inputObj.end();
			++i)
		{
			ss << boost::apply_visitor(encodeVisitor(), *i);
		}
        ss << "e";

        //remove "0:" after "l" in encoding
        std::string res = ss.str();
        auto emptyListPos = res.find("l0:");
        if (emptyListPos != std::string::npos)
        {
            res.erase(emptyListPos + 1, 2);
        }

		return ss.str();
	}
}
