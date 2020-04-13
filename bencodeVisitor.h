#include "ValueTypes.h"
#include <boost/variant/apply_visitor.hpp>

//compile-time checked visitation of each bounded type
class bencodeVisitor :
	public boost::static_visitor<>
{
public:
	bencodeVisitor(int indentation = 0);

	int getIndentation() const;

	void operator()(integer t) const;

	void operator()(const std::string& t) const;

	void operator()(const valueDictionary& t) const;

	void operator()(const valueList& t) const;

private:
	std::string getSpace() const;

	bool isAscii(int c) const;

	bool isAscii(const std::string& value) const;

	int indentation_;
};

