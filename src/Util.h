#pragma once

namespace regex
{
	inline srell::regex generic{ R"(\((.*?)\))" };                // pos(0,0,100) -> "0,0,100"
	inline srell::regex transform{ R"(\((.*?),(.*?),(.*?)\))" };  // pos(0,0,100) -> 0, 0, 100
	inline srell::regex string{ R"(,\s*(?![^()]*\)))" };          // pos(0, 0, 100), rot(0, 0, 100) -> "pos(0, 0, 100)","rot(0, 0, 100)"
}

namespace util
{
	std::vector<std::string> split_with_regex(const std::string& a_str, const srell::regex& a_regex);

	RE::FormID  GetFormID(const std::string& a_str);
	FormIDOrSet GetSwapFormID(const std::string& a_str);
}
