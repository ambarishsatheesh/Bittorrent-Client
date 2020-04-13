#include <boost/variant/static_visitor.hpp>
#include <string>
#include "ValueTypes.h"

class boostVisitor
	: public boost::static_visitor<>
{
public:

	template <typename T>
	void operator()(T& operand) const
	{
		return operand;
	}

};