#pragma once

using boost::asio::ip::tcp;

class LoginServer : public std::enable_shared_from_this<LoginServer>
{
public:
	LoginServer(asio::io_context& io_context, int port);

	void StartAccept();

protected:
	void OnAccept(SessionPtr session, boost::system::error_code ec);


private:
	tcp::acceptor _acceptor;
	asio::io_context& _io_context;
};