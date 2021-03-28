#include "TestClient.h"

int main()
{
	TestClient client;
	client.Connect("127.0.0.1", 60000);

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
		if (key[2] && !old_key[2]) { bQuit=true; }

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