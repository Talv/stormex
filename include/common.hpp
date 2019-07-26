#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <iostream>
#include "plog/Log.h"
#include "mplog/ColorConsoleAppenderStderr.h"
// #include "plog/Formatters/FuncMessageFormatter.h"

static const std::string stormexVersion = "2.0.1";

static plog::ColorConsoleAppenderStdErr<plog::TxtFormatter> consoleAppender;
// static plog::ColorConsoleAppender<plog::FuncMessageFormatter> consoleAppender;

#endif // __COMMON_HPP__
