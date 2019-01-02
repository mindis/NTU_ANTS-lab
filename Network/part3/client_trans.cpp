#include <stdio.h>
#include <cstring>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#pragma comment (lib, "ws2_32.lib")

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#undef APPMACROS_ONLY 
#include <openssl/applink.c>

using namespace std;

void error(string printout) {
    cout << printout << endl;
    exit(0);
}
void output(string printout) {
    cout << printout;
}

u_short cliport;
string myname;
string clientname;
string payamount;
bool trans = false;
//record workers status
SOCKET counter[3];
bool idle_worker[3];
//Load the certificate 
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    //set the local certificate from CertFile
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //set the private key from KeyFile
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    //verify private key 
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}
//Init server instance and context
SSL_CTX* InitServerCTX(void)
{   
    const SSL_METHOD *method;
    SSL_CTX *ctx;
 
    OpenSSL_add_all_algorithms();  //load & register all cryptos, etc. 
    SSL_load_error_strings();   // load all error messages */
    method = SSLv3_server_method();  // create new server-method instance 
    ctx = SSL_CTX_new(method);   // create new context from method
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
SSL_CTX* InitCTX(void)
{   const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = SSLv23_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}
//find idle worker to serve client
int find_idle_worker()
{
	for (int i = 0; i < 3; i++)
	{
		if (idle_worker[i])
		{
			return i;
		}
	}
	return -1;
}
// �Pclient�������s 
LPVOID service(LPVOID ID)
{
	string request;
	string response;
	int worker_ID = *((int *)ID);
	SOCKET new_client = counter[worker_ID];
	
	SSL_CTX *ctx;
	SSL *ssl;
	SSL_library_init();
    ctx = InitServerCTX();
    char cert[100]= "mycert.pem";
    char key[100]= "mykey.pem";
	LoadCertificates(ctx, cert, key);
   	ssl = SSL_new(ctx);              // get new SSL state with context 
    SSL_set_fd(ssl, new_client);      // set connection socket to SSL state
    if (SSL_accept(ssl) == -1)  
    {  
		cout << "error in accepting to SSL." << endl;
    	perror("accept");  
    }
    else
    {
	    char recvbuf[10000] = {0};
		//wait for request
		SSL_read(ssl, recvbuf, sizeof(recvbuf));
		request.assign(recvbuf);
		int last = request.find_last_of('#');
		int first = request.find('#');
		
		if (request.substr(last + 1, (request.length() - last)) == myname)
		{
			clientname = request.substr(0,first);
			payamount = request.substr(first + 1, last);
			trans = true;
		}	
	}
    
	idle_worker[worker_ID] = true;	
	closesocket(new_client); 
	SSL_free(ssl);         // release SSL state   
}
// client ������Lclients���s�u�n�D 
LPVOID start_listen()
{
	int r;
    WSAData wsaData;
    WORD DLLVSERION;
    DLLVSERION = MAKEWORD(2,1);//Winsocket-DLL ����
 
    //�� WSAStartup �}�l Winsocket-DLL
    r = WSAStartup(DLLVSERION, &wsaData);
 
    //�ŧi socket ��}��T(���P���q�T,�����P����}��T,�ҥH�|�����P����Ƶ��c�s��o�Ǧ�}��T)
    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);
 
    //�إ� socket
    SOCKET sListen; //listening for an incoming connection
    //�]�w��}��T�����
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cliport);
    //�]�w Listen
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);//SOMAXCONN: listening without any limit

	bool online = true;
	bool busy = false;

	for (int i = 0; i < 3; i++)
		idle_worker[i] = true;
	
	DWORD threadId[3];
	HANDLE hthread[3];
	
	//���ݳs�u
  	SOCKADDR_IN clinetAddr;
    while (online)
	{
		//accept connection
		SOCKET new_client;
		new_client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		new_client = accept(sListen, (SOCKADDR*)&clinetAddr, &addrlen);
		
		int worker_ID = find_idle_worker();
    
    	if (worker_ID != -1)
		{
			idle_worker[worker_ID] = false;
			counter[worker_ID] = new_client;
			int *worker_pointer = &worker_ID;
			hthread[worker_ID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)service, worker_pointer, 0, &threadId[worker_ID]);
			//close socket and thread
			CloseHandle(hthread[worker_ID]);
		
		}
		//busy - refuse connection
		else
		{
			char sendbuf[] = "200 Busy. Please try again later.";
			send(new_client, sendbuf, (int)strlen(sendbuf), 0);
			closesocket(new_client);
		}
      
    }
 	
 	closesocket(sListen);

}

int main()
{
	SSL_library_init();
    SSL_load_error_strings();
    SSL_CTX *ctx;
    SSL *ssl;
    ctx = InitCTX();
    ssl = SSL_new(ctx);
	// ��l��windows socket DLL 
	WSAData wsaData;
	WORD version = MAKEWORD(2, 2); // ����
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // ���\�^�� 0
	if (iResult != 0) 
	{
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

//	string str;//��JIP 
//	cout << "Please enter the IP address: ";
//	cin >> str;
//	const char *cp = str.c_str();//�x�sIP 
	
//	u_short hostshort = 10000;//��Jport number 
//	cout << "Please enter the portNumber: ";
//	cin >> hostshort; 
	
//	addr.sin_port = htons(hostshort); // �]�w port
//	addr.sin_addr.s_addr = inet_addr(cp); // �]�w IP

	addr.sin_port = htons(1234); // �]�w port
	addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // �]�w IP	
	// �s�u��socket server
	int r = connect(sock, (struct sockaddr *)&addr, sizeof (addr));
	if (r == SOCKET_ERROR)
		error("ERROR in connecting");
	
	SSL_set_fd(ssl, sock);
	if (SSL_connect(ssl) == -1)  
    	ERR_print_errors_fp(stderr);
    else
    {
    	//recieve server���T�� 
		char recv1[1024];
		if(SSL_read(ssl, recv1, sizeof(recv1)) < 0)
			cout << "Error of receive message." << endl;
		cout << recv1 << endl;
		
		//�}�lrequest server 
		cout << "Enter 1 for Register, 2 for Login: ";	
		int input;
		bool login = 0;//�O�_���\�n�J 
		bool end = 0;//�O�_���� 
	
		while(!end)
		{
			//client �� client ��������G�Ǧ^server
			if(trans)
			{
				string response;
				response += ("REPORT#" + payamount + "#" + clientname);
				const char *sendbuf = response.c_str();
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				trans = false;
			}
	
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
					SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
					SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
					cout << "-----------------------" << endl;
					cout << recvbuf << endl;
					cout << "Enter 1 for Register, 2 for Login: ";
				}
				else/*�w�g���\�n�J*/ 
				{
					char sendbuf[] = "List";
					SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
					SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
					cout << "-----------------------" << endl;
					cout << recvbuf << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
			}
			else if(input == 2)
			{
				cout << "Enter the name u want to Login: ";
				cin >> name;
				string temp(name);
				char port[10];
				cout << "Enter the port number: ";
				cin >> port;
				// �Nname#portnum�ǵ�server
				myname = temp;
			   	stringstream ss;
		       	ss.str(port);
		       	ss >> cliport;
				char sendbuf[]= "#";
				strcat(sendbuf, port);
				strcat(name, sendbuf);
				strcpy(sendbuf, name);
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
		        cout << "-----------------------" << endl;
				cout << recvbuf << endl;
				
				if(recvbuf[0] != '2')/*�n�J���\*/ 
		        {
		        	login = 1;
		           	cout << "Log in successfully" << endl;
		           
				
		           	//thread for listen to clients
		           	{
		           		DWORD listen_trans;
		           		HANDLE thread_trans;
		           		thread_trans = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_listen, NULL, 0, &listen_trans);
						//close socket and thread
						if (end)
							CloseHandle(thread_trans);
		           	}
				
					
					cout << "Enter the number of actions u want to take." << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
				else
					cout << "Enter 1 for Register, 2 for Login: ";
			}
			else if(input == 8)
			{
				char sendbuf[] = "Exit";	
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
				cout << recvbuf << endl;
				end = 1;
			}
			
			else if(input == 9)
			{
				cout << "Please enter the client name: ";
				string name;
				cin >> name;
				string temp;
				temp = ("TRANSACTION#" + name);
				
				const char *sendbuf = temp.c_str();
				SSL_write(ssl, sendbuf, (int)strlen(sendbuf));
				SSL_read(ssl, recvbuf, sizeof(recvbuf)); 
				if(recvbuf[1] == 2)
				{
					cout << recvbuf << endl;
					cout << "Enter the number of actions u want to take." << endl;
					cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
				}
				else
				{
					string receive(recvbuf);
					int first = receive.find('#');
					string str = receive.substr(0, first);
					string port = receive.substr(first + 1, (receive.length() - first));
				
					SSL_library_init();
					SSL_CTX *ctx1;
    				ctx1 = InitCTX(); 
					SSL *ssltoClient;
					ssltoClient = SSL_new(ctx1);
					
					WSAData wsaData;
					WORD version = MAKEWORD(2, 2); // ����
					int iResult = WSAStartup(MAKEWORD(2,2), &wsaData); // ���\�^�� 0
					if (iResult != 0) {
					    // ��l��Winsock ����
					}
					// �إ�socket
	
					SOCKET socktocli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if (sock < 0) 
						error("ERROR in setting socket");
	
					// �]�wIP �� port
					SOCKADDR_IN addr;
					memset (&addr, 0, sizeof (addr)) ; // �M��,�N��Ƴ]�� 0
					addr.sin_family = AF_INET;
					const char *cp = str.c_str();//�x�sIP 
					stringstream ss;
			        ss.str(port);
			        u_short cliport;
			        ss >> cliport; 
					addr.sin_port = htons(cliport); // �]�w port
					addr.sin_addr.s_addr = inet_addr(cp); // �]�w IP
					
					// �s�u��socket server
					int r = connect(socktocli, (struct sockaddr *)&addr, sizeof (addr));
					if (r == SOCKET_ERROR)
						error("ERROR in connecting");
						
					
					SSL_set_fd(ssltoClient, socktocli);
					if (SSL_connect(ssltoClient) == -1)  
				    	ERR_print_errors_fp(stderr);
				    else
					{
						string amount;
						cout << "Please enter payamount: ";
						cin >> amount;
						string cliname;
						cliname = name;
						string response;
						response += (myname + "#" + amount + "#" + cliname);
						const char *sendbuf = response.c_str();
						SSL_write(ssltoClient, sendbuf, (int)strlen(sendbuf));
						closesocket(socktocli);
						SSL_free(ssltoClient);
						cout << "Enter the number of actions u want to take." << endl;
						cout << "1 to ask for the latest list, 8 to Exit, 9 for transaction: ";
					}	
					
				}
			}
			else
				cout << "ERROR, please input again\n";
	
		}
	}
	
	closesocket(sock); // ����socket 
	SSL_CTX_free(ctx); // release context
}                                     
