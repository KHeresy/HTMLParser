#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>

#include "html_parser_detail.hpp"

namespace boost
{
	namespace property_tree
	{
		void read_html(const std::string &sText, ptree &pt)
		{
			detail::parseWholeString(sText, pt);
		}

		void read_html(std::istream &stream, ptree &pt)
		{
			read_html(std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()), pt);
		}
	}
}
