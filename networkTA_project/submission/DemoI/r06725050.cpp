#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>//����socket 
#define __USE_MINGW_ANSI_STDIO 0
#define MAX_BUFFER_SIZE 1024//��ʼ���g 

using namespace std;

class Client//���client
{
private:
	SOCKET _sock;//���SOCKET,����sock 
	SOCKADDR_IN _address;//���SOCKADDR,����address 
	
public: 
	Client(char* ip, uint16_t port);//����client �������Q��contrusted������� 
	string account;// �ַ������ 
	bool logged_in;//���룬�ɹ���ʧ�� 
	bool connecttry();//���У��������� 	
	void sendMessage(char* buffer); 
	void sendMessage(string buffer);//����Ϣ 
	string recvMessage(int buffer_size);
	string recvMessage();	
	void closeConnection();	
};

int main(int argc, char** argv){//argv �ִ����У�argc һ���ׂ�����Ė|�������cmd�\�� 
	char* server_ip;
	uint16_t port;
	if (argc == 1) {//���ֻ�д�n�����Q�]��ݔ��port ip�������A�O									
		server_ip = "140.112.107.194";//�A�O 
		port = 33120;//�A�O 
	}
	else {
		server_ip = argv[1];//����д�port IP������ݔ��� 
		port = atoi(argv[2]);
	}
	
	Client client(server_ip, port);//� ׃�����Q constructing �{�õ� 
	while (!client.connecttry()) {//�����function�ش�һ��false����run����� 
		while (true) {
			cout << "Do you want connect again? (please enter true/false): ";		
			string user_input;
			cin >> user_input;
			if (user_input == "true" || user_input == "True" ||user_input == "TRUE")//����ɂ��\��Ԫ����һ����ɂ�����true���t߉݋OR �\����( || ) �����ز���ֵtrue����t������false 
				break;
			else if (user_input == "false" || user_input == "False"|| user_input == "FALSE")//����ɂ��\��Ԫ����һ����ɂ�����true���t߉݋OR �\����( || ) �����ز���ֵtrue����t������false
				exit(0);
			else
				cout << "Input is wrong" << endl;
		}
		
		cin.ignore();									// ���ʹ���ߵ� input  
	}
	
	client.recvMessage(128);							// ���g128 
	cout << "connection accepted " << endl;//�ɹ����� 
	cout << "User Input Rule" << endl;
	cout << "register(space)<register name> " << endl;
	cout << "login (space) <register Name> (space)<Port> " << endl;
	cout << "list " << endl;
	cout << "exit " << endl;
	while (true) {
		if (client.logged_in)
			cout << client.account << ' ';
		cout << ">>> ";
		string user_input;
		getline(cin, user_input);						// ׌ʹ����ݔ�� 
		
		if (user_input.substr(0, 9) == "register ") {	// һ��9��(�����ո�) ���ͻ�run����functuion 
			if (!client.logged_in) {					// �o���ɹ��r��ֱ��login�r�򣬲�ִ������ 
				string user_name = user_input.substr(9, user_input.length() - 9);
				if (user_name == "") {
					cout << "Input is wrong, please input \"register (space) <Register Name>\" ." << endl;
					continue;
				}
				client.sendMessage("REGISTER#" + user_name + "\n");
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input.substr(0, 6) == "login ") {	// һ��6���֣������ո������login ��run ���棬Try to login
			if (!client.logged_in) {					// �o���ɹ���䛕r���ň������� 
				user_input.erase(0, 6);
				string user_name;
				if (user_input.find(' ') != string::npos) {
					user_name = user_input.substr(0, user_input.find(' '));
					user_input.erase(0, user_input.find(' ') + 1);
				}
				else {
					cout << "Input is wrong, please \"login (space) <Register Name> (space)<Port>\"." << endl;
					continue;
				}
				try {
					int input_port = stoi(user_input);
					if (input_port > 65535 || input_port < 0)//Port ���ܴ��65535��С�0 
						throw "illegal number";
				}
				catch (...) {
					cout << "Input port number is wrong" << endl;
					continue;
				}
				
				client.sendMessage(user_name + "#" + user_input + "\n");
				string result = client.recvMessage();//����login��result 
				if (result.substr(0, 3) == "220") {		// login ʧ�� 
					cout << result << endl;
					continue;
				}
				else {
					client.account = user_name;
					client.logged_in = true;
					cout << "This is Account Balance: " << result << endl;
				}
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input == "list") {				// ĿǰList���Ñ� 
			if (client.logged_in) {						// �ɹ�����ĕr�� 
				client.sendMessage("List\n");
				
				string result = client.recvMessage();
				if (result.substr(0, 3) == "220") {		// loginʧ�� 
					cout << result << endl;
					continue;
				}
				else {
					cout << "This is Account Balance: " << result << endl;
				}
			}
			else {
				cout << "Input is informal command" << endl << endl;
				continue;
			}
		}
		else if (user_input == "exit") {				// �x�_ 
			break;
		}
		else {
			cout << "Input is informal command" << endl << endl;
			continue;
		}
		
		cout << client.recvMessage() << endl;
	}
	
	client.closeConnection();
	
	return 0; 
}

Client::Client(char* ip, uint16_t port) {//�{�� ��� +�������Q  ��constructed 
	WSAData wsaData;//��ʼ�� 
	WORD version = MAKEWORD(2, 2); // �汾 
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);	//����ֵ 0 �� success��1��ʧ�� 
	if (iResult != 0){
		cout << "Initial WinSock failed." << endl;
		exit(1);
	}
	_sock = INVALID_SOCKET;//����socket 
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock == INVALID_SOCKET){
		cout << "Create socket failed." << endl;
		exit(1);
	}
	
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = inet_addr(ip);			// �O�� IP address
	_address.sin_port = htons(port);					// �O�� port number
	
	account = "";//��ʼ����ʹ�������Q�ǿյ�  
	logged_in = false;//��ʼ������false 
}

bool Client::connecttry() {
	int result = connect(_sock, (SOCKADDR*)&_address, sizeof(_address));
	if (result == -1){//�B�Qʧ�� 
		cout << "Can not connect to server." << endl;
		return false;
	}
	
	return true;
}

void Client::sendMessage(char* buffer) {
	send(_sock, buffer, (int)strlen(buffer), 0);
}

void Client::sendMessage(string buffer) {//�����һ������ ���D�и�ʽchar �ں��� 
	char send_buffer[buffer.length()];
	strcpy(send_buffer, buffer.c_str());
	
	sendMessage(send_buffer);
}

string Client::recvMessage(int buffer_size) {//�����һ������
	char recv_buffer[buffer_size];
	ZeroMemory(recv_buffer, buffer_size);				// ��� 
	int status = recv(_sock, recv_buffer, sizeof(recv_buffer), 0);
	if (status == 0) {//ʧ���B�Q�r 
		cout << "Currently,server has closed the connection." << endl;
		exit(0);
	}
	
	return string(recv_buffer);
}

string Client::recvMessage() {
	return recvMessage(MAX_BUFFER_SIZE);
}

void Client::closeConnection() {
	if (logged_in){								
		sendMessage("Exit\n");
		cout << recvMessage(128);
	}
	
	closesocket(_sock);
}

