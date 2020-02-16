#include <SFML/Network.hpp>
#include <iostream>
#include <vector>

using namespace std;
using namespace sf;

#define moveLength 2


class Test
{
public:
	string m_name, m_move;
	Vector2f m_pos;
	Test(string name = "none") { m_name = name; m_move = "none"; }

	friend Packet& operator<<(Packet& packet, Test& test)
	{
		return packet << test.m_name << test.m_pos.x << test.m_pos.y << test.m_move;
	}

	friend Packet& operator>>(Packet& packet, Test& test)
	{
		return packet >> test.m_name >> test.m_pos.x >> test.m_pos.y >> test.m_move;
	}
};

Mutex mutex;
vector<TcpSocket*> users;
SocketSelector selector;
bool endAll = false;
int port = 5789;

//nothing to change
void doAtExit()
{
	for (auto& user : users)
	{
		delete user;
	}
	users.clear();
}

void show()
{
	/*while (!endAll)
	{*/
	system("cls");
	mutex.lock();
	cout << "Public Address : " << IpAddress::getPublicAddress() << endl;
	cout << "Local Address : " << IpAddress::getLocalAddress() << endl;
	cout << "Port : " << port << endl;
	cout << users.size() - 1 << endl;
	for (auto& user : users)
	{
		if (user->getRemoteAddress() != TcpSocket().getRemoteAddress())
			cout << user->getRemoteAddress() << endl;
	}
	mutex.unlock();
	/*sleep(seconds(3));
}*/
}

//nothing to change
void listenConnection()
{
	sf::TcpListener listener;
	// lie l'écouteur à un port
	if (listener.listen(port) != sf::Socket::Done)
	{
		cout << "Port indisponible" << endl;
	}

	while (!endAll)
	{
		mutex.lock();
		if(users.empty() || users[users.size() - 1]->getRemoteAddress() != TcpSocket().getRemoteAddress())
			users.push_back(new TcpSocket);
		mutex.unlock();
		// accepte une nouvelle connexion
		if (listener.accept(*users[users.size() - 1]) != sf::Socket::Done)
		{
			// erreur...
			cout << "None" << endl;
		}
		else
		{
			//un seul utilisateur par pc
			/*if (users.size() > 1 && find(users.begin(), users.end() - 1, users[users.size() - 1]) != users.end())
			{
				delete users[users.size() - 1];
				users[users.size() - 1] = new TcpSocket;
			}
			else*/
				selector.add(*users[users.size() - 1]);
				show();
		}

		// utilisez la socket "client" pour communiquer avec le client connecté,
		// et continuez à attendre de nouvelles connexions avec l'écouteur
	}
}

//nothing to change
void checkConnection()
{
	Packet packet;
	while (!endAll)
	{
		mutex.lock();
		//for each user
		for (int i = 0; i < users.size(); i++)
		{
			//if the user has been accepted
			if (users[i]->getRemoteAddress() != TcpSocket().getRemoteAddress())
			{
				//if the user is disconnected (sends an empty packet)
				if (users[i]->send(packet) == Socket::Disconnected)
				{
					//delete the user
					selector.remove(*users[i]);
					delete users[i];
					users.erase(users.begin() + i);
				}
			}
		}
		mutex.unlock();
		sleep(seconds(1));
	}
}

//nothing to change
void send(Packet &packet)
{
	int i = 0;
	for (auto& user : users)
	{
		if (user->getRemoteAddress() != TcpSocket().getRemoteAddress())
		{
			//if the user is disconnected (sends an empty packet)
			if (users[i]->send(packet) == Socket::Disconnected)
			{
				//delete the user
				selector.remove(*users[i]);
				delete users[i];
				users.erase(users.begin() + i);
				i++;
				show();
				continue;
			}
			user->send(packet);
		}
		i++;
	}
}

//some things to change
void receive()
{
	while (!endAll)
	{
		Packet packet;
		if (selector.wait())
		{
			mutex.lock();
			for (auto& user : users)
			{
				if (selector.isReady(*user))
				{
					user->receive(packet);
					if (!packet.endOfPacket())
					{
						//cout << "ok" << endl;
						string name;
						//packet HAS to contain the name of the entity first
						packet >> name;

						//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
						//Insert your conditions from here
						if (name == "Test")
						{
							Test test;
							packet >> test;
							if (test.m_move.find("Down") != string::npos)
							{
								test.m_pos.y += moveLength;
							}
							if (test.m_move.find("Up") != string::npos)
							{
								test.m_pos.y -= moveLength;
							}
							if (test.m_move.find("Right") != string::npos)
							{
								test.m_pos.x += moveLength;
							}
							if (test.m_move.find("Left") != string::npos)
							{
								test.m_pos.x -= moveLength;
							}
							packet.clear();
							packet << name << test;
							//Don't forget to send the packet back
							send(packet);
						}
					}
					else
					{
						//cout << "Fin de paquet" << endl;
					}
				}
			}
			mutex.unlock();
		}
	}
}

int main()
{
	Thread listenerConnection(&listenConnection);
	//Thread showing(&show);
	//Thread check(&checkConnection);
	Thread receiver(&receive);

	atexit(doAtExit);
	listenerConnection.launch();
	//showing.launch();
	//check.launch();
	receiver.launch();
	show();
	return 0;
}