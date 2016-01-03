#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>

#include "html_parser_detail.hpp"

namespace boost
{
	namespace property_tree
	{
		void read_html(std::istream &stream, ptree &pt)
		{
			std::string sHTML((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
			detail::parseWholeString(sHTML, pt, 0);
		}
	}
}
