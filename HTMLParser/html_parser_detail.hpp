#pragma once

#pragma region Standard Library
#include <array>
#include <set>
#include <stack>
#pragma endregion

#pragma region Boost Library
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#pragma endregion

namespace boost
{
	namespace property_tree
	{
		namespace detail
		{
			#pragma region pre-defined variables
			// the set of tag that only have open-side tag, no close-side
			const std::set<std::string> setSingleSideTag = { "!DOCTYPE", "base", "basefont", "br", "hr", "input", "link", "meta" };
			#pragma endregion

			bool parseWholeString(const std::string& sContent, ptree& pt, const size_t& uStartPos);
			std::array<size_t, 2> parseFirstTagTree(const std::string& sContent, ptree &pt, const size_t& uStartPos);

			// Add string as text node without tag
			void addTextNode(std::string sContent, ptree &ptNode, bool bTrim = true)
			{
				if(bTrim)
					boost::algorithm::trim(sContent);

				if (sContent.length() > 0)
				{
					ptree nodeThis;
					nodeThis.put_value(sContent);
					ptNode.add_child("<htmltext>", nodeThis);
				}
			}

			std::array<size_t, 2> findToken(const std::string& sContent, const std::string& sBegin, const std::string& sEnd, const size_t& uStartPos)
			{
				size_t uS = sContent.find(sBegin, uStartPos);
				if (uS != std::string::npos)
				{
					size_t uE = sContent.find(sEnd, uS + sBegin.size());
					if (uE != std::string::npos)
						return{ uS, uE + sEnd.size() };
				}
				return{ std::string::npos, std::string::npos };
			}

			// Find a tag, from '<' to '>'
			std::array<size_t, 2> findTag(const std::string& sContent, const size_t& uStartPos)
			{
				// TODO: doesn't consider if there is '>' in tag attribute
				return findToken(sContent, "<", ">", uStartPos);
			}

			std::array<size_t, 2> parseFirstTagTree( const std::string& sContent, ptree& ptNode, const size_t& uStartPos)
			{
				auto aRange = findTag(sContent, uStartPos);
				if (aRange[0] != std::string::npos)
				{
					ptree nodeThis;

					size_t uEndofTagName = sContent.find_first_of(" \t\n\r<>", aRange[0] + 1);
					if (uEndofTagName != std::string::npos)
					{
						// get tag name
						std::string sTagName = sContent.substr(aRange[0] + 1, uEndofTagName - aRange[0] - 1);
						
						// check if is a comment
						if (sTagName.substr(0, 3) == "!--")
						{
							size_t uEndPos = sContent.find("-->", aRange[0]);
							if (uEndPos == std::string::npos)
							{
								// TODO: Can't find the end of comment
								return{ std::string::npos, std::string::npos };
							}

							// TODO: may not work if there is no space between comment and tags
							std::string sComment = sContent.substr(uEndofTagName, uEndPos - uEndofTagName);
							boost::algorithm::trim(sComment);
							nodeThis.put_value(sComment);
							ptNode.add_child("<htmlcomment>", nodeThis);
							return aRange;
						}

						// TODO: process attribute

						// single side tag, without value
						if (sContent[aRange[1] - 2] == '/' || setSingleSideTag.find(sTagName) != setSingleSideTag.end())
						{
							// close directly
							ptNode.add_child(sTagName, nodeThis);
							return aRange;
						}

						// find close tag
						size_t uClosePos = sContent.find("</" + sTagName, aRange[1]);
						if (uClosePos == std::string::npos)
						{
							// TODO: some tag may need a close tag, but can't be found
						}
						else
						{
							// with close tag
							size_t uEndOfClosePos = sContent.find(">", uClosePos);
							if (uEndOfClosePos != std::string::npos)
							{
								std::string sValue = sContent.substr(aRange[1], uClosePos - aRange[1]);

								parseWholeString(sValue,nodeThis, 0);

								ptNode.add_child(sTagName, nodeThis);
								return{ aRange[0] , uEndOfClosePos + 1 };
							}
						}
					}
				}
				return{ std::string::npos, std::string::npos };
			}

			bool parseWholeString(const std::string& sContent, ptree &pt, const size_t& uStartPos)
			{
				std::array<size_t, 2> aLastRange = { uStartPos, uStartPos };
				while (true)
				{
					std::array<size_t, 2> aCurRange = parseFirstTagTree(sContent, pt, aLastRange[1] );
					if (aCurRange[0] == std::string::npos)
					{
						// found no tag, process all text
						std::string sNoTagValue = sContent.substr(aLastRange[1]);
						addTextNode(sNoTagValue, pt);
						break;
					}
					else
					{
						// check the data between tags
						if (aCurRange[0] != aLastRange[1])
						{
							std::string sNoTagValue = sContent.substr(aLastRange[1], aCurRange[0] - aLastRange[1]);
							addTextNode(sNoTagValue, pt);
						}
						aLastRange = aCurRange;
					}
				}
				return false;
			}
		}
	}
}
