#pragma once

namespace active911 
{
	class Connection;

	class ConnectionFactory 
	{
	public:
		virtual std::shared_ptr<Connection> create() = 0;
	};
}