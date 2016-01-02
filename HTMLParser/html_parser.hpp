#pragma once

#include <array>
#include <string>

#include <boost/property_tree/ptree.hpp>

#include "html_parser_detail.hpp"

namespace boost
{
	namespace property_tree
	{
		void read_html(std::istream &stream, ptree &pt)
		{
			std::string sLine;
			while (std::getline(stream, sLine))
			{
			}
		}
	}
}
