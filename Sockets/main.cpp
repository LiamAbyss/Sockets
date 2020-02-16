#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace sf;


class Test
{
public:
	string m_name, m_move;
	Vector2f m_pos;
	Test(string name = "none") { m_name = name; }

	friend Packet& operator<<(Packet& packet, Test &test)
	{
		return packet << test.m_name << test.m_pos.x << test.m_pos.y << test.m_move;
	}

	friend Packet& operator>>(Packet& packet, Test &test)
	{
		return packet >> test.m_name >> test.m_pos.x >> test.m_pos.y >> test.m_move;
	}
};

CircleShape player(50);
map<string, Vector2f> otherPlayers;

Mutex mutex;
Test testPlayer, test;
TcpSocket server;
IpAddress serverAddress;
bool endAll = false;
int port = 5789;


void send()
{
	mutex.lock();
	Packet packet;
	packet << string("Test") << testPlayer;
	mutex.unlock(); 
	if (server.send(packet) == Socket::Disconnected)
	{
		exit(0);
	}
	//sleep(milliseconds(10));
}

void bindKeyboard(RenderWindow& window, CircleShape& player)
{
	mutex.lock();
	testPlayer.m_move = "";
	bool doSend = false;
	if (Keyboard::isKeyPressed(Keyboard::Left))
	{
		testPlayer.m_move += "Left";
		doSend = true;
	}
	if (Keyboard::isKeyPressed(Keyboard::Up))
	{
		testPlayer.m_move += "Up";
		doSend = true;
	}
	if (Keyboard::isKeyPressed(Keyboard::Right))
	{
		testPlayer.m_move += "Right";
		doSend = true;
	}
	if (Keyboard::isKeyPressed(Keyboard::Down))
	{
		testPlayer.m_move += "Down";
		doSend = true;
	}
	if (doSend)
		send();
	mutex.unlock();
}

void drawPlayer(RenderWindow& window, CircleShape& player, Vector2f& pos)
{
	mutex.lock();
	if (pos != Vector2f(INFINITY, INFINITY))
	{
		while (pos.x - player.getRadius() / 2 > window.getSize().x)
		{
			pos = Vector2f(pos.x - window.getSize().x, pos.y);
		}
		if (pos.x + player.getRadius() / 2 < 0)
		{
			pos = Vector2f(pos.x + window.getSize().x, pos.y);
		}
		if (pos.y - player.getRadius() / 2 > window.getSize().y)
		{
			pos = Vector2f(pos.x, pos.y - window.getSize().y);
		}
		if (pos.y + player.getRadius() / 2 < 0)
		{
			pos = Vector2f(pos.x, pos.y + window.getSize().y);
		}
		player.setPosition(pos);
	}
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x + window.getSize().x, player.getPosition().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x, player.getPosition().y + window.getSize().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x - window.getSize().x, player.getPosition().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x - window.getSize().x, player.getPosition().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x, player.getPosition().y - window.getSize().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x, player.getPosition().y - window.getSize().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x + window.getSize().x, player.getPosition().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x + window.getSize().x, player.getPosition().y));
	window.draw(player);
	player.setPosition(Vector2f(player.getPosition().x - window.getSize().x, player.getPosition().y + window.getSize().y));
	mutex.unlock();
}

void connect()
{
	bool connected = false;
	int i = 1, iMax = 5;
	while (!connected)
	{
		cout << "Try : " << i++ << "/" << iMax << endl;
		Socket::Status status = server.connect(serverAddress, port, seconds(5));
		if (status != sf::Socket::Done)
		{
			// erreur...
			if (i > iMax)
			{
				system("pause");
				exit(0);
			}
		}
		else
		{
			connected = true;
			cout << "Connected..." << endl;
		}
	}
}

void receive()
{
	while (!endAll)
	{
		Packet packet;
		if (server.receive(packet) != sf::Socket::Done)
		{
			// erreur...
		}
		else
		{
			if (!packet.endOfPacket())
			{
				string name;
				packet >> name;
				if (name == "Test")
				{
					mutex.lock();
					packet >> test;
					if (test.m_name == testPlayer.m_name)
					{
						testPlayer = test;
					}
					else
					{
						otherPlayers[test.m_name] = test.m_pos;
					}
					mutex.unlock();
				}
			}
			else
			{
				//cout << "Fin de paquet" << endl;
			}
			//continue
		}
	}
}

void checkConnection()
{
	Packet packet;
	while (!endAll)
	{
		mutex.lock();
		//if the server is disconnected (sends an empty packet)
		if (server.send(packet) == Socket::Disconnected)
		{
			exit(0);
		}
		mutex.unlock();
		sleep(seconds(30));
	}
}


void display()
{
	RenderWindow window(VideoMode(1250, 630), "MultiplayerMap", Style::Default, ContextSettings(0U, 0U, 8));

	player.setFillColor(Color(0, 0, 255));
	CircleShape other(50);
	bool active = true;
	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed)
			{
				window.close();
			}
			if (event.type == Event::LostFocus)
			{
				active = false;
			}
			if (event.type == Event::GainedFocus)
			{
				active = true;
			}
		}

		if (active)
		{
			bindKeyboard(window, player);
		}
		window.clear();
		drawPlayer(window, player, testPlayer.m_pos);
		for (auto& players : otherPlayers)
		{
			drawPlayer(window, other, players.second);
		}
		window.display();
		sleep(milliseconds(10));
	}
	exit(0);
}

int main()
{
	cout << "Address : ";
	cin >> serverAddress;
	connect();
	cout << "Name : ";
	cin >> testPlayer.m_name;

	Thread receiver(&receive);
	Thread check(&checkConnection);
	//Thread sender(&send);
	Thread displayer(&display);


	displayer.launch();
	receiver.launch();
	check.launch();
	//sender.launch();

	return 0;
}