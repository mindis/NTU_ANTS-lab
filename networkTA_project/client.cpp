#include <stdio.h>
#include <cstring>
#include <string>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")

#include <iostream>
// jijij
using namespace std;
//ksdjhflskdjf
void error(string printout) {
    cout << printout << endl;
    exit(0);
}
void output(string printout) {
    cout << printout;
}

int main()
{
	// ��l��windows socket DLL 
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // ����
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // ���\�^�� 0
	if (iResult != 0) {
	    // ��l��Winsock ����
	}
	// �إ�socket

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) 
		error("ERROR in setting socket");

	// �]�wIP �� port
	SOCKADDR_IN addr;
	memset (&addr, 0, sizeof (addr)) ; // �M��,�N��Ƴ]�� 0
	addr.sin_family = AF_INET;

	string str;//��JIP 
	cout << "Please enter the IP address: ";
	cin >> str;
	const char *cp = str.c_str();//�x�sIP 
	u_short hostshort = 10000;//��Jport number 
	cout << "Please enter the portNumber: ";
	cin >> hostshort; 
	addr.sin_port = htons(hostshort); // �]�w port
	addr.sin_addr.s_addr = inet_addr(cp); // �]�w IP

	
	// �s�u��socket server
	int r = connect(sock, (struct sockaddr *)&addr, sizeof (addr));
	if (r == SOCKET_ERROR)
		error("ERROR in connecting");
	
	//recieve server���T�� 
	char recv1[1024];
	if(recv(sock, recv1, sizeof(recv1), 0) < 0)
		cout << "Error of receive message." << endl;
	cout << recv1 << endl;
	
	//�}�lrequest server 

	cout << "Enter 1 for Register, 2 for Login: ";	
	int input;
	bool login = 0;//�O�_���\�n�J 
	bool end = 0;//�O�_���� 
	while(!end)
	{
		cin >> input;
		char name[100] = {0};
		char recvbuf[10000] = {0};
		if (input == 1)
		{
			if(!login)/*�|���n�J���\*/ 
			{
				cout << "Enter the name u want to Register: ";
				cin >> name;
				char sendbuf[]= "REGISTER#";/*�NREGISTER#name�ǵ�server*/
				strcat(sendbuf, name);
				send(sock, sendbuf, (int)strlen(sendbuf), 0);
				recv(sock, recvbuf, sizeof(recvbuf), 0); 
				cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				cout << "Enter 1 for Register, 2 for Login: ";
		
			}
			else/*�w�g���\�n�J*/ 
			{
				char sendbuf[] = "List";
				send(sock, sendbuf, (int)strlen(sendbuf), 0);
				recv(sock, recvbuf, sizeof(recvbuf), 0);
				cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				cout << "1 to ask for the latest list, 8 to Exit: ";
			}
		}
		else if(input == 2)
		{
			cout << "Enter the name u want to Login: ";
			cin >> name;
			char port[10];
			cout << "Enter the port number: ";
			cin >> port;
			// �Nname#portnum�ǵ�server
			char sendbuf[]= "#";
			strcat(sendbuf, port);
			strcat(name, sendbuf);
			strcpy(sendbuf, name);
			send(sock, sendbuf, (int)strlen(sendbuf), 0);
			recv(sock, recvbuf, sizeof(recvbuf), 0);
            cout << "-----------------------" << endl;
			cout << recvbuf << endl;
			
			if(recvbuf[0] != '2')/*�n�J���\*/ 
            {
            	cout << "Log in successfully" << endl;
				cout << "Enter the number of actions u want to take." << endl;
				cout << "1 to ask for the latest list, 8 to Exit: ";
				login = 1;
			}
			else
			{
				cout << "Enter 1 for Register, 2 for Login: ";
			}
		}
		else if(input == 8)
		{
			char sendbuf[] = "Exit";
			send(sock, sendbuf, (int)strlen(sendbuf), 0);
			recv(sock, recvbuf, sizeof(recvbuf), 0);
			cout << recvbuf << endl;
			end = 1;
		}
		else
			cout << "ERROR, please input again\n";
	}
	
	closesocket(sock); // ����socket 
	
}                                     
