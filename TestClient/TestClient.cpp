#include "TestClient.h"

int main()
{
	try
	{
		namespace ssl = asio::ssl;

		static const std::string pem_file{ "../../rootCACert.pem" };

		Brilliant::InitLog<Brilliant::iClientServerLogId>();

		TestClient client;
		client.Connect("127.0.0.1", 60000);
		auto& ssl_ctx = client.GetSslContext();
		ssl_ctx.set_options(ssl::context::default_workarounds | ssl::context::sslv23 | ssl::context::no_sslv2);
		ssl_ctx.load_verify_file(pem_file);
		ssl_ctx.set_verify_mode(ssl::context::verify_peer);

		bool key[3] = { false, false, false };
		bool old_key[3] = { false, false, false };

		bool bQuit = false;
		while (!bQuit)
		{
			if (GetForegroundWindow() == GetConsoleWindow())
			{
				key[0] = GetAsyncKeyState('1') & 0x8000;
				key[1] = GetAsyncKeyState('2') & 0x8000;
				key[2] = GetAsyncKeyState('3') & 0x8000;
			}

			if (key[0] && !old_key[0]) { client.PingServer(); }
			if (key[2] && !old_key[2]) { bQuit = true; }

			for (int i = 0; i < 3; i++)
			{
				old_key[i] = key[i];
			}

			if (client.IsConnected())
			{
				if (!client.GetIncoming().Empty())
				{
					auto msg = client.GetIncoming().PopFront().msg;

					switch (msg.mHeader.tId)
					{
					case TestMessage::ServerPing:
						auto now = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point then;
						msg >> then;
						std::cout << "Ping: " << std::chrono::duration<double>(now - then).count() << '\n';
						break;
					}
				}
			}
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		Brilliant::Log<Brilliant::iClientServerLogId>(Brilliant::LogLevel::Error) << "ERROR: " << e.what();
		return 1;
	}
}
#endif