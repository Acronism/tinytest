#pragma once

#include <string>

#include "boost/format.hpp"

namespace TestDetail {

inline std::string FormatImpl(boost::format& formatter) {
    return formatter.str();
}

template<typename Head, typename... Tail>
std::string FormatImpl(boost::format& formatter, Head&& h, Tail&& ... args) {
    formatter % std::forward<Head>(h);
    return FormatImpl(formatter, std::forward<Tail>(args)...);
}

/**
* Variadic wrapper for boost.format.
*/
template<typename... Args>
std::string Format(std::string fmt, Args&& ... args) {
	boost::format message(fmt);
	try {
		return FormatImpl(message, std::forward<Args>(args)...);
	} catch (...) {}
	return std::string("[invalid format]: ") + fmt;
}	

} // TestDetail