#pragma once

class SignManager
{
public:
	SignManager();

	bool TryCheckID(const std::string& id);
	int TrySignIn(const std::string& id, const std::string& pw);
	bool TrySignUp(const std::string& id, const std::string& pw);

private:
	std::string Query_CheckID;
	std::string Query_SignIn;
	std::string Query_SignUp;
};

extern SignManagerPtr GSignManager;