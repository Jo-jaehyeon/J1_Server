#pragma once

#include "Types.h"

#include <iostream>

// SQL
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <exception>

// boost
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm//string/split.hpp>
#include <boost/algorithm//string/classification.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/pool/pool_alloc.hpp>

using BufferAllocator = boost::pool_allocator<char>;
using BufferPooledVector = std::vector<char, BufferAllocator>;

// spdlog
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"

//MySQL
#include "mysql/mysql_connection.h"
#include <mysql/cppconn/driver.h>
#include <mysql/cppconn/exception.h>
#include <mysql/cppconn/resultset.h>
#include <mysql/cppconn/prepared_statement.h>
#include <mysql/cppconn/statement.h>