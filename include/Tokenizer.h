#pragma once
#include "ValueTypes.h"
#include <string>
#include <deque>

namespace Bittorrent
{
	class Tokenizer
	{
	public:
		static void tokenize(const std::string& encoded,
			std::deque<std::string>& tokens, bool& isFail);

	private:
		Tokenizer();
	};
}