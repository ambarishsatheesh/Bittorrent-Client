/**
 * @file ValueTypes.h
 *
 * @brief Definition of types used in the Bittorrent client
 */

#pragma once
#define BOOST_VARIANT_NO_FULL_RECURSIVE_VARIANT_SUPPORT
#include <boost/variant.hpp>
#include <string>
#include <vector>
#include <map>

using byte = uint8_t;

//define alias for recursive variant (e.g. to allow for variants within 
//vectors which then contain further vector of variants, or maps with 
//recursive variant values
using value = boost::make_recursive_variant<
	long long,
	std::string, 
	std::vector<boost::recursive_variant_>,
	std::map<std::string, boost::recursive_variant_>
>::type;

using valueDictionary = std::map<std::string, value>;
using valueList = std::vector<value>;

