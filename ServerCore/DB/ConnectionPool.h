#pragma once
/* Copyright 2013 Active911 Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http: *www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


 /* ConnectionPool manages a connection pool of some kind.  Worker threads can ask for a connection, and must return it when done.
  * Each connection is guaranteed to be healthy, happy, and free of disease.
  *
  * Connection and ConnectionFactory are virtual classes that should be overridden to their actual type.
  *
  * NOTE: To avoid using templates AND inheritance at the same time in the ConnectionFactory, ConnectionFactory::create must create a derved type
  * but return the base class.
  */


  // Define your custom logging function by overriding this #define

#include "pch.h"
#include <deque>
#include "ConnectionFactory.h"

using namespace std;

namespace active911
{
	class Connection;
	class MySQLConnection;

	struct ConnectionUnavailable : std::exception
	{
		char const* what() const throw()
		{
			return "Unable to allocate connection";
		};
	};

	struct ConnectionPoolStats
	{
		size_t pool_size;
		size_t borrowed_size;
	};

	template<class T>
	class ConnectionPool
	{
	public:
		static void Init(size_t pool_size, std::shared_ptr<ConnectionFactory> factory);

		ConnectionPoolStats get_stats()
		{
			// Lock
			std::unique_lock<std::mutex> lock(this->io_mutex);

			// Get stats
			ConnectionPoolStats stats;
			stats.pool_size = this->pool.size();
			stats.borrowed_size = this->borrowed.size();

			return stats;
		};

		ConnectionPool(size_t pool_size, std::shared_ptr<ConnectionFactory> factory)
		{
			// Setup
			this->pool_size = pool_size;
			this->factory = factory;

			// Fill the pool
			while (this->pool.size() < this->pool_size)
			{
				this->pool.push_back(this->factory->create());
			}
		};

		~ConnectionPool()
		{
		};

		/**
		 * Borrow
		 *
		 * Borrow a connection for temporary use
		 *
		 * When done, either (a) call unborrow() to return it, or (b) (if it's bad) just let it go out of scope.  This will cause it to automatically be replaced.
		 * @retval a shared_ptr to the connection object
		 */
		std::shared_ptr<T> borrow()
		{

			// TODO : LockGuard
			// Lock
			std::unique_lock<std::mutex> lock(this->io_mutex);

			// Check for a free connection
			if (this->pool.size() == 0)
			{
				// Are there any crashed connections listed as "borrowed"?
				for (std::set<std::shared_ptr<Connection> >::iterator it = this->borrowed.begin(); it != this->borrowed.end(); ++it)
				{

					if (it->use_count() == 1)
					{

						// This connection has been abandoned! Destroy it and create a new connection
						try
						{
							// If we are able to create a new connection, return it
							spdlog::debug("Creating new connection to replace discarded connection");
							std::shared_ptr<Connection> conn = this->factory->create();
							this->borrowed.erase(it);
							this->borrowed.insert(conn);
							return static_pointer_cast<T>(conn);
						}
						catch (std::exception& e)
						{
							// Error creating a replacement connection
							throw ConnectionUnavailable();
						}
					}
				}
				// Nothing available
				throw ConnectionUnavailable();
				//return nullptr;
			}

			// Take one off the front
			std::shared_ptr<Connection>conn = this->pool.front();
			this->pool.pop_front();

			// Add it to the borrowed list
			this->borrowed.insert(conn);

			return static_pointer_cast<T>(conn);
		};

		/**
		 * Unborrow a connection
		 *
		 * Only call this if you are returning a working connection.  If the connection was bad, just let it go out of scope (so the connection manager can replace it).
		 * @param the connection
		 */
		void unborrow(std::shared_ptr<T> conn)
		{
			// TODO : LockGuard
			// Lock
			std::unique_lock<std::mutex> lock(this->io_mutex);

			// borrowed set에 해당 요소가 있는지 먼저 확인
			auto it = this->borrowed.find(conn);
			if (it != this->borrowed.end())
			{
				// 존재할 경우에만 pool에 추가하고 borrowed에서 제거
				this->pool.push_back(static_pointer_cast<Connection>(conn));
				this->borrowed.erase(it);
			}
		};

	protected:
		std::shared_ptr<ConnectionFactory> factory;
		size_t pool_size;
		deque<std::shared_ptr<Connection> > pool;
		set<std::shared_ptr<Connection>> borrowed;
		std::mutex io_mutex;
	};
}

extern active911::ConnectionPool<active911::MySQLConnection>* GConnectionPool;

template<class T>
void active911::ConnectionPool<T>::Init(size_t pool_size, std::shared_ptr<ConnectionFactory> factory)
{
	GConnectionPool = new ConnectionPool<T>(pool_size, factory);
	
	// Fill the pool
	while (GConnectionPool->pool.size() < GConnectionPool->pool_size) 
	{
		GConnectionPool->pool.push_back(GConnectionPool->factory->create());
	}
}