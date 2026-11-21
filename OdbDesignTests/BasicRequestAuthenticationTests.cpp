#include <gtest/gtest.h>

#include <App/BasicRequestAuthentication.h>
#include <ExitCode.h>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>

// F004 process-level fail-closed test: spawns the REAL OdbDesignServer binary
// and asserts it exits with ExitCode::PreServerRunFailed and opens no listener
// when authentication is enabled but credentials are not configured.
#ifndef _WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#endif

using namespace Odb::Lib::App;

namespace Odb::Test
{
	namespace
	{
		// RFC 4648 standard base64 alphabet, for building Basic credentials
		std::string base64Encode(const std::string& input)
		{
			constexpr char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			std::string output;
			output.reserve((input.size() + 2) / 3 * 4);

			std::string::size_type i = 0;
			while (i + 3 <= input.size())
			{
				const auto b0 = static_cast<unsigned char>(input[i]);
				const auto b1 = static_cast<unsigned char>(input[i + 1]);
				const auto b2 = static_cast<unsigned char>(input[i + 2]);
				const auto triple = (b0 << 16) | (b1 << 8) | b2;

				output += ALPHABET[(triple >> 18) & 0x3F];
				output += ALPHABET[(triple >> 12) & 0x3F];
				output += ALPHABET[(triple >> 6) & 0x3F];
				output += ALPHABET[triple & 0x3F];
				i += 3;
			}

			const auto remaining = input.size() - i;
			if (remaining == 1)
			{
				const auto b0 = static_cast<unsigned char>(input[i]);
				output += ALPHABET[(b0 >> 2) & 0x3F];
				output += ALPHABET[(b0 << 4) & 0x3F];
				output += '=';
				output += '=';
			}
			else if (remaining == 2)
			{
				const auto b0 = static_cast<unsigned char>(input[i]);
				const auto b1 = static_cast<unsigned char>(input[i + 1]);
				output += ALPHABET[(b0 >> 2) & 0x3F];
				output += ALPHABET[((b0 << 4) | (b1 >> 4)) & 0x3F];
				output += ALPHABET[(b1 << 2) & 0x3F];
				output += '=';
			}

			return output;
		}

		std::string basicAuthHeader(const std::string& username, const std::string& password)
		{
			return "Basic " + base64Encode(username + ":" + password);
		}

		// RAII environment-variable control. Setting to nullptr removes the
		// variable; the previous value (including "absent") is restored on
		// destruction so tests cannot leak environment state into each other.
		class ScopedEnvVar
		{
		public:
			ScopedEnvVar(const char* name, const char* value)
				: m_name(name)
			{
				const auto previous = std::getenv(name);
				m_previouslySet = previous != nullptr;
				if (m_previouslySet) m_previousValue = previous;

				apply(value);
			}

			~ScopedEnvVar()
			{
				apply(m_previouslySet ? m_previousValue.c_str() : nullptr);
			}

			ScopedEnvVar(const ScopedEnvVar&) = delete;
			ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

		private:
			void apply(const char* value)
			{
#if defined(_WIN32)
				if (value == nullptr)
				{
					// deleting on Windows: empty assignment makes getenv return ""
					// (an empty value is treated as unconfigured by the product)
					std::string deletion = std::string(m_name) + "=";
					(void)_putenv(deletion.c_str());
				}
				else
				{
					std::string assignment = std::string(m_name) + "=" + value;
					(void)_putenv(assignment.c_str());
				}
#else
				if (value == nullptr)
				{
					::unsetenv(m_name);
				}
				else
				{
					::setenv(m_name, value, 1);
				}
#endif
			}

			const char* m_name;
			bool m_previouslySet = false;
			std::string m_previousValue;
		};

		crow::request makeRequest(const std::string& authorizationHeader)
		{
			crow::request req;
			if (!authorizationHeader.empty())
			{
				req.add_header("Authorization", authorizationHeader);
			}
			return req;
		}
	}

	// Controls the credential environment for every test: no credentials are
	// configured unless a test explicitly configures them, and Environment is
	// never "Local" so the debug/local authentication bypass cannot mask a
	// regression.
	class BasicRequestAuthenticationTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			m_environment.emplace("Environment", "Testing");
			clearCredentials();
		}

		void setCredentials(const std::string& username, const std::string& password)
		{
			m_username.emplace(BasicRequestAuthentication::USERNAME_ENV_NAME, username.c_str());
			m_password.emplace(BasicRequestAuthentication::PASSWORD_ENV_NAME, password.c_str());
		}

		void clearCredentials()
		{
			m_username.emplace(BasicRequestAuthentication::USERNAME_ENV_NAME, nullptr);
			m_password.emplace(BasicRequestAuthentication::PASSWORD_ENV_NAME, nullptr);
		}

		BasicRequestAuthentication makeAuth(bool disableAuthentication = false) const
		{
			return BasicRequestAuthentication(disableAuthentication);
		}

	private:
		std::optional<ScopedEnvVar> m_environment;
		std::optional<ScopedEnvVar> m_username;
		std::optional<ScopedEnvVar> m_password;
	};

	// -----------------------------------------------------------------------
	// VerifyCredentials: the fail-closed decision (F004)
	// -----------------------------------------------------------------------

	// F004 regression: the previously built-in default credential pair
	// ("odb"/"plusplus") must NOT authorize a request when no credentials are
	// configured. Old behavior returned 200 here.
	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_NoCredentialsConfigured_DefaultCredentials_FailsClosed)
	{
		auto auth = makeAuth();
		auto resp = auth.VerifyCredentials("odb", "plusplus");
		EXPECT_EQ(resp.code, 500);
		EXPECT_NE(resp.code, crow::status::OK);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_NoCredentialsConfigured_AnyCredentials_FailsClosed)
	{
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("someuser", "somepassword").code, 500);
		EXPECT_EQ(auth.VerifyCredentials("", "").code, 500);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_UsernameOnlyConfigured_MissingPassword_FailsClosed)
	{
		ScopedEnvVar username(BasicRequestAuthentication::USERNAME_ENV_NAME, "TestUser");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("TestUser", "whatever").code, 500);
		// the known default must not combine with a partially configured server either
		EXPECT_EQ(auth.VerifyCredentials("odb", "plusplus").code, 500);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_PasswordOnlyConfigured_MissingUsername_FailsClosed)
	{
		ScopedEnvVar password(BasicRequestAuthentication::PASSWORD_ENV_NAME, "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("TestUser", "S3cretPass").code, 500);
		EXPECT_EQ(auth.VerifyCredentials("odb", "plusplus").code, 500);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_EmptyEnvValues_FailsClosed)
	{
		setCredentials("", "");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("TestUser", "S3cretPass").code, 500);
		EXPECT_EQ(auth.VerifyCredentials("", "").code, 500);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_ConfiguredCredentials_CorrectPair_Authorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("TestUser", "S3cretPass").code, 200);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_ConfiguredCredentials_WrongPassword_Forbidden)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("TestUser", "wrongpassword").code, 403);
	}

	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_ConfiguredCredentials_WrongUsername_Forbidden)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("wronguser", "S3cretPass").code, 403);
	}

	// the old default must also be rejected when the server has DIFFERENT
	// credentials configured
	TEST_F(BasicRequestAuthenticationTest, VerifyCredentials_ConfiguredCredentials_DefaultCredentials_Forbidden)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.VerifyCredentials("odb", "plusplus").code, 403);
	}

	// -----------------------------------------------------------------------
	// AreCredentialsConfigured: the startup-validation predicate
	// -----------------------------------------------------------------------

	TEST_F(BasicRequestAuthenticationTest, AreCredentialsConfigured_NoEnvVars_False)
	{
		EXPECT_FALSE(BasicRequestAuthentication::AreCredentialsConfigured());
	}

	TEST_F(BasicRequestAuthenticationTest, AreCredentialsConfigured_EmptyEnvVars_False)
	{
		setCredentials("", "");
		EXPECT_FALSE(BasicRequestAuthentication::AreCredentialsConfigured());
	}

	TEST_F(BasicRequestAuthenticationTest, AreCredentialsConfigured_OnlyUsername_False)
	{
		ScopedEnvVar username(BasicRequestAuthentication::USERNAME_ENV_NAME, "TestUser");
		EXPECT_FALSE(BasicRequestAuthentication::AreCredentialsConfigured());
	}

	TEST_F(BasicRequestAuthenticationTest, AreCredentialsConfigured_BothEnvVars_True)
	{
		setCredentials("TestUser", "S3cretPass");
		EXPECT_TRUE(BasicRequestAuthentication::AreCredentialsConfigured());
	}

	// -----------------------------------------------------------------------
	// AuthenticateRequest: full request-path behavior (auth ENABLED)
	// -----------------------------------------------------------------------

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_NoAuthorizationHeader_Unauthorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		auto resp = auth.AuthenticateRequest(makeRequest(""));
		EXPECT_EQ(resp.code, 401);
	}

	// F004 regression through the request path: default credentials in a
	// well-formed Basic header must not authorize when nothing is configured.
	// Old behavior returned 200 here.
	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_NoCredentialsConfigured_DefaultCredentials_FailsClosed)
	{
		auto auth = makeAuth();
		auto resp = auth.AuthenticateRequest(makeRequest(basicAuthHeader("odb", "plusplus")));
		EXPECT_EQ(resp.code, 500);
		EXPECT_NE(resp.code, crow::status::OK);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_ConfiguredCredentials_CorrectPair_Authorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		auto resp = auth.AuthenticateRequest(makeRequest(basicAuthHeader("TestUser", "S3cretPass")));
		EXPECT_EQ(resp.code, 200);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_ConfiguredCredentials_WrongPassword_Forbidden)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		auto resp = auth.AuthenticateRequest(makeRequest(basicAuthHeader("TestUser", "wrongpassword")));
		EXPECT_EQ(resp.code, 403);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_ConfiguredCredentials_WrongUsername_Forbidden)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		auto resp = auth.AuthenticateRequest(makeRequest(basicAuthHeader("wronguser", "S3cretPass")));
		EXPECT_EQ(resp.code, 403);
	}

	// malformed/truncated headers must fail closed as Unauthorized — a bare
	// "Basic" token previously threw std::out_of_range from substr(6)
	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_MalformedHeader_BareSchemeToken_Unauthorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("Basic")).code, 401);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_MalformedHeader_SchemeWithEmptyPayload_Unauthorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("Basic ")).code, 401);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_WrongScheme_Unauthorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("Bearer " + base64Encode("TestUser:S3cretPass"))).code, 401);
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest(base64Encode("TestUser:S3cretPass"))).code, 401);
	}

	// a base64 payload with no "user:password" separator is not credentials
	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_PayloadWithoutColon_Unauthorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("Basic " + base64Encode("userpassword"))).code, 401);
	}

	// RFC 7235: the auth-scheme token is case-insensitive
	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_LowercaseScheme_CorrectCredentials_Authorized)
	{
		setCredentials("TestUser", "S3cretPass");
		auto auth = makeAuth();
		auto header = "basic " + base64Encode("TestUser:S3cretPass");
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest(header)).code, 200);
	}

	// -----------------------------------------------------------------------
	// AuthenticateRequest: auth explicitly DISABLED — behavior unchanged
	// -----------------------------------------------------------------------

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_DisabledAuthentication_NoHeader_Authorized)
	{
		clearCredentials();	// disabled mode must not require any configuration
		auto auth = makeAuth(true);
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("")).code, 200);
	}

	TEST_F(BasicRequestAuthenticationTest, AuthenticateRequest_DisabledAuthentication_BogusHeader_Authorized)
	{
		clearCredentials();
		auto auth = makeAuth(true);
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest("Basic notbase64!!!")).code, 200);
		EXPECT_EQ(auth.AuthenticateRequest(makeRequest(basicAuthHeader("odb", "plusplus"))).code, 200);
	}

#ifndef _WIN32
	// -----------------------------------------------------------------------
	// Process level: the REAL server binary refuses to start (and opens no
	// listener) when authentication is enabled but credentials are missing.
	// This is the fail-closed-before-bind guarantee (F004 / SEC-*).
	// -----------------------------------------------------------------------

	namespace
	{
		bool tryConnect(const std::string& address, unsigned short port)
		{
			int sock = ::socket(AF_INET, SOCK_STREAM, 0);
			if (sock < 0) return false;	// cannot probe; treat as no listener observed

			sockaddr_in target {};
			target.sin_family = AF_INET;
			target.sin_port = htons(port);
			if (::inet_pton(AF_INET, address.c_str(), &target.sin_addr) != 1)
			{
				::close(sock);
				return false;
			}

			bool connected = ::connect(sock, reinterpret_cast<sockaddr*>(&target), sizeof(target)) == 0;
			::close(sock);
			return connected;
		}

		// waits up to timeoutSeconds for the child to exit; returns -1 if it had
		// to be killed (a server that keeps running is a test failure, not a hang)
		int waitOrKill(pid_t child, int timeoutSeconds, bool& exitedNormally, int& exitStatus)
		{
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);
			while (std::chrono::steady_clock::now() < deadline)
			{
				int status = 0;
				const auto pid = ::waitpid(child, &status, WNOHANG);
				if (pid == child)
				{
					exitedNormally = WIFEXITED(status);
					exitStatus = exitedNormally ? WEXITSTATUS(status) : -1;
					return 0;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}

			::kill(child, SIGKILL);
			int status = 0;
			(void)::waitpid(child, &status, 0);
			return -1;
		}
	}

	TEST_F(BasicRequestAuthenticationTest, ServerStartup_AuthEnabledMissingCredentials_ExitsNonZeroWithoutListener)
	{
		// isolation: this test process may have real credentials or Environment
		// set for other tests; the child must see an UNCONFIGURED environment
		auto environment = ScopedEnvVar("Environment", "Testing");
		auto username = ScopedEnvVar(BasicRequestAuthentication::USERNAME_ENV_NAME, nullptr);
		auto password = ScopedEnvVar(BasicRequestAuthentication::PASSWORD_ENV_NAME, nullptr);
		ASSERT_FALSE(BasicRequestAuthentication::AreCredentialsConfigured());

		// unique-ish ports so concurrent test runs do not collide
		const auto base = static_cast<unsigned short>(20000 + (::getpid() % 20000));
		const auto restPort = base;
		const auto grpcPort = static_cast<unsigned short>(base + 1);
		const auto restPortStr = std::to_string(restPort);
		const auto grpcPortStr = std::to_string(grpcPort);

		const char* childArgv[] = {
			ODBDESIGN_SERVER_EXECUTABLE_PATH,
			"--bind", "127.0.0.1",
			"--port", restPortStr.c_str(),
			"--grpc-port", grpcPortStr.c_str(),
			nullptr
		};

		const auto child = ::fork();
		ASSERT_NE(child, -1) << "fork failed: " << strerror(errno);
		if (child == 0)
		{
			execv(ODBDESIGN_SERVER_EXECUTABLE_PATH, const_cast<char* const*>(childArgv));
			// exec failed — exit distinctly so the parent can tell
			_exit(127);
		}

		bool exitedNormally = false;
		int exitStatus = -1;
		const auto waited = waitOrKill(child, 30, exitedNormally, exitStatus);
		ASSERT_EQ(waited, 0) << "server did not exit within 30s (it must fail closed, not run unauthenticated)";

		// 127 from exec failure would mean the binary path is wrong, not a product outcome
		ASSERT_NE(exitStatus, 127) << "could not execute " << ODBDESIGN_SERVER_EXECUTABLE_PATH;

		// REAL outcome 1: the process exited on its own with PreServerRunFailed (5)
		ASSERT_TRUE(exitedNormally) << "server terminated by signal instead of exiting fail-closed";
		EXPECT_EQ(exitStatus, static_cast<int>(::Utils::ExitCode::PreServerRunFailed));
		EXPECT_NE(exitStatus, 0);

		// REAL outcome 2: no REST or gRPC listener exists after the failed start
		EXPECT_FALSE(tryConnect("127.0.0.1", restPort)) << "REST port is listening after fail-closed startup";
		EXPECT_FALSE(tryConnect("127.0.0.1", grpcPort)) << "gRPC port is listening after fail-closed startup";
	}
#endif
}
