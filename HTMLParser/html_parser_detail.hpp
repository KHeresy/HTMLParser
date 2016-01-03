#pragma once

#pragma region Standard Library
#include <array>
#include <set>
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
			// the set of tag that only have open-side tag, no close-side. Lower case only
			const std::set<std::string> setSingleSideTag = { "!doctype", "base", "basefont", "br", "hr", "input", "img", "link", "meta" };
			#pragma endregion

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

			// Find a tag, from '<' to '>'
			std::array<size_t, 2> findTag(const std::string& sContent, const size_t& uStartPos)
			{
				// TODO: doesn't consider if there is '>' in tag attribute
				size_t uS = sContent.find('<', uStartPos);
				if (uS != std::string::npos)
				{
					size_t uE = sContent.find('>', uS + 1);
					if (uE != std::string::npos)
						return{ uS, uE + 1 };
				}
				return{ std::string::npos, std::string::npos };
			}

			// Find a string that may have quate
			std::array<size_t, 2> findQuateString(const std::string& sContent, const size_t& uStartPos, const size_t& uEndPos)
			{
				// find a non-empty start
				size_t uFirstPos = sContent.find_first_not_of(" \t\n\r", uStartPos);
				if (uFirstPos == std::string::npos || uFirstPos >= uEndPos)
					return{ std::string::npos, std::string::npos };

				// check if there is ' or "
				char cQuateChar = ' ';
				if (sContent[uFirstPos] == '\'' || sContent[uFirstPos] == '\"')
				{
					cQuateChar = sContent[uFirstPos];

					// find the next quate char
					for (size_t uIdx = uFirstPos + 1; uIdx < uEndPos; ++uIdx)
					{
						if (sContent[uIdx] == cQuateChar && sContent[uIdx - 1] != '\\')
						{
							return{ uFirstPos , uIdx + 1 };
						}
					}
				}
				else
				{
					size_t uSecondPos = sContent.find_first_of(" \t\n\r>", uFirstPos + 1);
					if (uSecondPos != std::string::npos && uSecondPos <= uEndPos)
						return{ uFirstPos, uSecondPos + 1};
				}
				return{ std::string::npos, std::string::npos };
			}

			// parse attributes in tag
			void parseAttribute(const std::string& sContent, const size_t& uStartPos, const size_t& uEndPos, ptree& ptNode )
			{
				size_t uLastPos = uStartPos;
				ptree ptAttributeList;
				while (true)
				{
					ptree ptAttribute;

					// find the begin of attribute name
					size_t uNameStartPos = sContent.find_first_not_of(" \t\n\r<>", uLastPos);
					if (uNameStartPos == std::string::npos || uNameStartPos >= uEndPos)
						break;

					// find the end of attribute name
					size_t uNameEndPos = sContent.find_first_of(" \t\n\r=>", uNameStartPos + 1);
					if (uNameEndPos == std::string::npos || uNameEndPos >= uEndPos)
						break;

					// get attribute name
					std::string sAttrName = sContent.substr(uNameStartPos, uNameEndPos - uNameStartPos);
					if (sAttrName == "/")
					{
						uLastPos = uNameEndPos;
						continue;
					}

					// check if the follow char is '='
					uNameStartPos = sContent.find_first_not_of(" \t\n\r<>", uNameEndPos);
					if (uNameStartPos != std::string::npos && uNameStartPos < uEndPos && sContent[uNameStartPos] == '=')
					{
						// attribute with value
						std::array<size_t, 2> aTextRange = findQuateString(sContent, uNameStartPos + 1, uEndPos);
						if (aTextRange[0] != std::string::npos)
						{
							std::string sValue = sContent.substr(aTextRange[0], aTextRange[1] - aTextRange[0]);
							if (sValue[0] == '\'' || sValue[0] == '\"')
								sValue = sValue.substr(1, sValue.size() - 2);
							ptAttribute.put_value(sValue);
							uLastPos = aTextRange[1];
						}
					}
					else
					{
						uLastPos = uNameStartPos + 1;
					}
					ptAttributeList.add_child(sAttrName, ptAttribute);
				}

				if (ptAttributeList.size() > 0)
					ptNode.add_child("<htmlattr>", ptAttributeList);
			}

			// Find the first tag tree in given string
			// return { tag start position, tag end posinion } if is a vaild tag
			// return { pos, pos } if is a close tag
			// return { std::string::npos, std::string::npos } if invaild
			std::array<size_t, 2> parseFirstTagTree( const std::string& sContent, const size_t& uStartPos, ptree& ptNode)
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
						boost::algorithm::to_lower(sTagName);

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

						// process attribute
						parseAttribute(sContent, uEndofTagName, aOpenTagRange[1], nodeThis);

						// process <script> as a special case since it may have '<' or '>' inside
						if (sTagName == "script")
						{
							// found close-tag directlly
							size_t uClosePos = sContent.find("</script", aOpenTagRange[1]);
							if (uClosePos != std::string::npos)
							{
								std::string sCode = sContent.substr(aOpenTagRange[1], uClosePos - aOpenTagRange[1]);
								addTextNode(sCode, nodeThis);

								uClosePos = sContent.find(">", uClosePos);
								return{ aOpenTagRange[0],uClosePos+1 };
							}
						}

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
							std::array<size_t, 2> aTagRange = parseFirstTagTree(sContent, aChildRange[1], nodeThis);
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

			bool parseWholeString(const std::string& sContent, ptree& ptNode)
			{
				std::array<size_t, 2> aLastRange = { 0, 0 };
				while (true)
				{
					std::array<size_t, 2> aCurRange = parseFirstTagTree(sContent, aLastRange[1], ptNode);
					if (aCurRange[0] == std::string::npos)
					{
						// found no tag, process all text
						std::string sNoTagValue = sContent.substr(aLastRange[1]);
						addTextNode(sNoTagValue, ptNode);
						break;
					}
					else if (aCurRange[0] == aCurRange[1])
					{
						// found a close tag, should not happen?
					}
					else
					{
						// check the data between tags
						if (aCurRange[0] != aLastRange[1])
						{
							std::string sNoTagValue = sContent.substr(aLastRange[1], aCurRange[0] - aLastRange[1]);
							addTextNode(sNoTagValue, ptNode);
						}
						aLastRange = aCurRange;
					}
				}
				return false;
			}
		}
	}
}
