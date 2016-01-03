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
			const std::set<std::string> setSingleSideTag = { "!DOCTYPE", "base", "basefont", "br", "hr", "input", "img", "link", "meta" };
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

			// Find the first tag tree in given string
			// return { tag start position, tag end posinion } if is a vaild tag
			// return { pos, pos } if is a close tag
			// return { std::string::npos, std::string::npos } if invaild
			std::array<size_t, 2> parseFirstTagTree( const std::string& sContent, ptree& ptNode, const size_t& uStartPos)
			{
				std::array<size_t, 2> aOpenTagRange = findTag(sContent, uStartPos);
				if (aOpenTagRange[0] != std::string::npos)
				{
					ptree nodeThis;

					size_t uEndofTagName = sContent.find_first_of(" \t\n\r<>", aOpenTagRange[0] + 1);
					if (uEndofTagName != std::string::npos)
					{
						// get tag name
						std::string sTagName = sContent.substr(aOpenTagRange[0] + 1, uEndofTagName - aOpenTagRange[0] - 1);
						
						// check if is a comment
						if (sTagName.substr(0, 3) == "!--")
						{
							size_t uEndPos = sContent.find("-->", aOpenTagRange[0]);
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
							return aOpenTagRange;
						}

						// check if is a close-tag
						if (sTagName[0] == '/')
						{
							return{ aOpenTagRange[0],aOpenTagRange[0] };
						}

						// TODO: process attribute

						// single side tag, without value
						if ((sContent.size() > 2 && sContent[aOpenTagRange[1] - 2] == '/' ) || setSingleSideTag.find(sTagName) != setSingleSideTag.end())
						{
							// close directly
							ptNode.add_child(sTagName, nodeThis);
							return aOpenTagRange;
						}

						// process string between start and close tags
						std::array<size_t, 2> aChildRange = { aOpenTagRange[1], aOpenTagRange[1] };
						while (aChildRange[0] != std::string::npos)
						{
							std::array<size_t, 2> aTagRange = parseFirstTagTree(sContent, nodeThis, aChildRange[1]);
							if (aTagRange[0] != std::string::npos)
							{
								// process data between tags
								if (aChildRange[1] != aTagRange[0])
								{
									std::string sValue = sContent.substr(aChildRange[1], aTagRange[0] - aChildRange[1]);
									addTextNode(sValue, nodeThis);
								}

								if (aTagRange[0] == aTagRange[1])
								{
									// is a close tag
									std::array<size_t, 2> aCloseTagRange = findTag(sContent, aTagRange[0]);
									std::string sCloseTagName = sContent.substr(aCloseTagRange[0] + 2, aCloseTagRange[1] - aCloseTagRange[0] - 3);

									// close current tag
									ptNode.add_child(sTagName,nodeThis);

									if (sTagName == sCloseTagName)
										return{ aOpenTagRange[0],aCloseTagRange[1] };
									else
										return{ aCloseTagRange[0],aCloseTagRange[0] };
								}
							}
							aChildRange = aTagRange;
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
					else if (aCurRange[0] == aCurRange[1])
					{
						// found a close tag
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
