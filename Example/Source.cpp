#include <fstream>
#include <iostream>
#include <map>

#include "html_parser.hpp"

std::ostream& operator<<(std::ostream& os, const std::pair<int, const boost::property_tree::ptree&>& rNode)
{
	int iNext = rNode.first + 2;
	const boost::property_tree::ptree& rPT = rNode.second;

	// output node value
	if(rPT.data().size() > 0 )
		os << " Value: [" << rPT.data() << "]";
	os << "\n";

	// output sub nodes
	for (auto it = rPT.begin(); it != rPT.end(); ++it)
	{
		os.width(iNext);
		os << "";
		os << "Name: [" << it->first << "]";
		os << std::pair<int, const boost::property_tree::ptree&>(iNext, it->second);
	}
	return os;
}

int main(int argc, char** argv)
{
	// read HTML
	std::ifstream fileHTML("test.html");
	std::string sHTML((std::istreambuf_iterator<char>(fileHTML)), std::istreambuf_iterator<char>());
	fileHTML.close();

	// Parse html
	boost::property_tree::ptree ptreeHTML;
	boost::property_tree::detail::parseWholeString(sHTML, ptreeHTML, 0);

	// Output
	std::cout << std::pair<int, const boost::property_tree::ptree&>(0, ptreeHTML) << std::endl;
}
