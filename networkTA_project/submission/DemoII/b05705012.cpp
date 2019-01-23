//Server
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream> 
#include<string>
#include<algorithm>
#include<vector>
#pragma warning(disable:4996) 

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

vector<pair<string, int>> user;
vector<vector<string>> Online; //ӛ��ھ�ʹ���ߵ����֡�IP��ע����port
vector<SOCKET> Pool;
vector<SOCKET> Queue; //������Queue
vector<int> Empty; //ӛ䛿���ľ��̵�indexֵ���ٴ�����
int maxQueue = 1; //��꠵�����˔������^������client���B����serverʧ��
int maxConnection = 2; //���ͬ�r�ھ����˔�

//���˂���Thread����ֵ��index,IP,PORT���½�һ��def
typedef struct data
{
	int index;
	char ip[20];
	int port;
}Data;

vector<Data> Queue_Data; //�����錦�����Y��

//Client Thread
void ClientHandlerThread(LPVOID pM)
{
	Data *pt = (Data *)pM;
	string IP = pt->ip;
	int index = pt->index;
	int PORT = pt->port; //ע�⣺С����port��client����rע����׌�e��client͸�^��port�����ӍϢ����port����PORT��client�B���е�port
	char text[1000]; 
	bool login = FALSE;
	string name;
	while (true)
	{
		memset(text, 0, sizeof(text));
		recv(Pool[index], text, sizeof(text), NULL); //get message from client
		//���������@ʾ����ӍϢ��������Q
		if (login == FALSE)
		{
			cout << "receive message from client: " << text << endl;
		}
		else
		{
			cout << "receive message from " << name << ": " << text << endl;
		}
		
		string mean = string(text);
		int jcount = 0; //"#"�Ă���
		for (int j = 0; j < mean.size(); j++) //Ӌ��ClientՈ����"#"�Ĕ����Ա�eՈ������
		{
			if (mean[j] == '#')
			{
				jcount++;
			}
		}
		if (mean.find("REGISTER#") == 0 & login == FALSE) //�]��(������o���]��)
		{
			name = mean.substr(9);
			//ȥ����β��\r��\n
			name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
			name.erase(std::remove(name.begin(), name.end(), '\r'), name.end());
			cout << "REGISTER: Name: " << name << endl;
			bool find = FALSE;
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					find = TRUE;
					break;
				}
			}
			if (find == TRUE) //�]��ʧ��
			{
				cout << "REGISTER: Name: " << name << " FAILED." << endl;
				mean = "210 FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else //�]�Գɹ�
			{
				cout << "REGISTER: Name: " << name << " SUCESSED." << endl;
				mean = "100 OK\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				//����client����Ľ��~
				recv(Pool[index], text, sizeof(text), NULL);
				cout << "receive message from client: " << text << endl;
				mean = string(text);
				mean.erase(std::remove(mean.begin(), mean.end(), '\n'), mean.end());
				mean.erase(std::remove(mean.begin(), mean.end(), '\r'), mean.end());
				int start = stoi(mean);
				user.push_back(make_pair(name, start));	
				//�؂��ɹ�
				mean = "Register Success\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
		}
		else if (jcount == 1 & login == FALSE) //����
		{
			int k = mean.find("#");
			name = mean.substr(0, k);
			string port = mean.substr(k + 1);
			//ȥ����β��\r��\n
			port.erase(std::remove(port.begin(), port.end(), '\n'), port.end());
			port.erase(std::remove(port.begin(), port.end(), '\r'), port.end());
			cout << "LOG IN: Name: " << name << ", Port: "<< port << endl;
			//���]�Բ��ܵ���
			bool find = FALSE;
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					find = TRUE;
					break;
				}
			}
			//��ֹ���}����
			bool isLogIn = FALSE;
			for (int i = 0; i < Online.size(); i++)
			{
				if (Online[i][0] == name) //����һ�ӟo�����}����
				{
					isLogIn = TRUE;
					break;
				}
			}
			if (find == TRUE & isLogIn == FALSE) //����ɹ�
			{
				login = TRUE;
				cout <<"Client: " << name << " has logged in." << endl;
				vector<string> newOnline;
				newOnline.push_back(name);
				newOnline.push_back(IP);
				newOnline.push_back(port);
				Online.push_back(newOnline); //����Online��list��
				//����ɹ���ݔ�������N�~
				for (int i = 0; i < user.size(); i++)
				{
					if (user[i].first == name)
					{
						mean = to_string(user[i].second) + "\n";
						break;
					}
				}
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				//ݔ���Ͼ����
				mean = "number of accounts online: " + to_string(Online.size()) + "\n";
				for (int i = 0; i < Online.size(); i++)
				{
					mean += Online[i][0] + "#" + Online[i][1] + "#" + Online[i][2] + "\n";
				}
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else if(isLogIn == FALSE) //����ʧ����δ�]�ԣ�
			{
				cout << "Client: " << name << " failed to log in.(not registered)" << endl;
				mean = "220 AUTH_FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
			else //����ʧ����ͬ���Ď����ѵ��룩
			{
				cout << "Client: " << name << " failed to log in.(already logged in)" << endl;
				mean = "220 AUTH_FAIL\n";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
			}
		}
		else if (mean == "List\n" & login == TRUE) //�N�~���Ͼ���̖���
		{
			//����ɹ���ݔ�������N�~
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					mean = to_string(user[i].second) + "\n";
					break;
				}
			}
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
			//ݔ���Ͼ����
			mean = "number of accounts online: " + to_string(Online.size()) + "\n";
			for (int i = 0; i < Online.size(); i++)
			{
				mean += Online[i][0] + "#" + Online[i][1] + "#" + Online[i][2] + "\n";
			}
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else if (jcount == 2 & login == TRUE) //�R��
		{
			//�@ȡ���׌������ֺͽ��~
			mean = mean.substr(mean.find("#")+1);
			int pay = stoi(mean.substr(0,mean.find("#")));
			mean = mean.substr(mean.find("#") + 1);
			//ȥ����β��\r��\n
			mean.erase(remove(mean.begin(), mean.end(), '\n'), mean.end());
			mean.erase(remove(mean.begin(), mean.end(), '\r'), mean.end());
			string other = mean; //other: ���׌��������
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == name)
				{
					user[i].second -= pay;
					break;
				}
			}
			for (int i = 0; i < user.size(); i++)
			{
				if (user[i].first == other)
				{
					user[i].second += pay;
					break;
				}
			}
			mean = name + "->" + to_string(pay) + "->" + other + "\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else if (mean == "Exit\n" & login == TRUE) //�x�_
		{
			//�Ȱl��Bye�oClient
			mean = "Bye\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
			//�Y��thread�r�˳����룬���ھ�������Ƴ�
			for (int i = 0; i < Online.size(); i++)
			{
				if (Online[i][0] == name)
				{
					Online.erase(Online.begin() + i);
					break;
				}
			}
			cout << "Client: " << name << ", IP: " << IP << ", Port: " << PORT << " is Disconnect." << endl;
			closesocket(Pool[index]);
			if (Queue.empty() == FALSE) //���������Queue���, �Ͱ��@���ճ����thread�o����һ�����^�mޒȦ
			{
				Pool[index] = Queue[0];
				Queue.erase(Queue.begin());
				//���½Ӽ{��Client�YӍ�o�ó���
				Data data = Queue_Data[0];
				Queue_Data.erase(Queue_Data.begin());
				IP = string(data.ip);
				PORT = data.port;
				cout << "Connection from client IP: " << IP << ", Port: " << PORT << endl;
				string mean = "Hello, how can I serve you?";
				memset(text, 0, sizeof(text));
				strcpy(text, mean.c_str());
				send(Pool[index], text, sizeof(text), NULL);
				login = FALSE;
			}
			else //����]������ꠣ��Ͱ��@���յ�thread��index�ŵ�Empty�Y�棬�Y��ޒȦ
			{
				Empty.push_back(index);
				break;
			}	
		}
		else if (mean == "")
		{
			//Client�g��ͨӍ��ʲ�NҲ����
			mean = "\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
		else
		{
			mean = "please type the right option number!\n";
			memset(text, 0, sizeof(text));
			strcpy(text, mean.c_str());
			send(Pool[index], text, sizeof(text), NULL);
		}
	}
	//����ޒȦ��tread�Y��
}


int main() {

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 1), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 0;
	}

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // �O�� IP	   // addr.sin_addr.s_addr = INADDR_ANY; // ���O�� INADDR_ANY ��ʾ�κ� IP
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345); // �O�� port


	//create socket descriptor
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, addrlen);
	listen(sListen, SOMAXCONN);

	cout << "IP: " << inet_ntoa(addr.sin_addr) << endl;
	cout << "Port: " << (int)ntohs(addr.sin_port) << endl;

	SOCKET newConnection;
	while (TRUE)
	{
		newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);
		if (Pool.size() == maxConnection) //�ѽ��½����̔��_�����ֵ
		{
			if (Empty.empty() == FALSE) //�п��N�ľ��̿���ʹ��
			{
				int index = Empty[0]; //ȡ��һ����thread�ľ�̖
				Empty.erase(Empty.begin());	
				cout << "Connection from client IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << endl;
				if (newConnection == 0)
				{
					cout << "failed in creating new connection!!" << endl;
				}
				else
				{
					//cout << "connect to client successfully!!" << endl;
					char text[1000] = "Hello, how can I serve you?";
					send(newConnection, text, sizeof(text), NULL);
					Pool[index] = newConnection;
					//dataӛ䛴�Client��ip, index, port
					Data data;
					data.index = index;
					memset(data.ip, 0, sizeof(data.ip));
					strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
					data.port = (int)ntohs(addr.sin_port);
					CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(&data), NULL, NULL); //Create Thread to handle this client. The index in the socket array for this thread is the value (i).		
				}
			}
			else if(Queue.size() < maxQueue) //�]�п��N��thread, Queue߀�п��g��׌��ȥQueue���
			{	
				Data data;
				data.index = 0; //�䌍�Ƕ��N��ֵ�����S���O�����������ǰһ��ʹ���ߵ�index��
				memset(data.ip, 0, sizeof(data.ip));
				strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
				data.port = (int)ntohs(addr.sin_port);
				Queue.push_back(newConnection);
				Queue_Data.push_back(data);
				cout << "Client Request IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << " has been added to Queue." << endl;
			}
			else //Queue�M�ˣ�drop��
			{
				cout << "Client Request has dropped!!" << endl;
			}
		}
		else //߀�]����󾀳̔��ĕr��
		{
			cout << "Connection from client IP: " << inet_ntoa(addr.sin_addr) << ", Port: " << (int)ntohs(addr.sin_port) << endl;
			if (newConnection == 0)
			{
				cout << "failed in creating new connection!!" << endl;
			}
			else
			{
				//cout << "connect to client successfully!!" << endl;
				char text[1000] = "Hello, how can I serve you?";
				send(newConnection, text, sizeof(text), NULL);
				Pool.push_back(newConnection);
				//dataӛ䛴�Client��ip, index, port
				Data data;
				data.index = Pool.size() - 1;
				memset(data.ip, 0, sizeof(data.ip));
				strcpy(data.ip, string(inet_ntoa(addr.sin_addr)).c_str());
				data.port = (int)ntohs(addr.sin_port);
				CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(&data), NULL, NULL); //Create Thread to handle this client. The index in the socket array for this thread is the value (i).		
			}
		}
		
	}
	system("pause");





	return 0;
}