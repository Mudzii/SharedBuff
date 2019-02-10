#include "ComLib.h"

void Produce(int nrOfMessages);
void Consume(int nrOfMessages);
void GenRandom(char *s, const int len);

// ============================== GLOBAL VARIABLES =========================================

std::string role = "";
std::string messageSize = "";

float delay = 0.0;
int memorySize  = 0;
int numMessages = 0;
size_t msgSize  = 0;


// ================================ MAIN ===================================================

int main(int arcg, char** argv) {

	role = argv[1];
	messageSize = argv[5];

	delay = (float)atof(argv[2]);
	memorySize = atoi(argv[3]);
	numMessages = atoi(argv[4]);

	if (messageSize.compare("random") == 0) 
		msgSize = rand() % 100 + 1;
	
	else
		msgSize = atoi(messageSize.c_str());


	if (role.compare("producer") == 0) 
		Produce(numMessages);


	if (role.compare("consumer") == 0) 
		Consume(numMessages);
	

	return 0;
}



// random character generation of "len" bytes.
// the pointer s must point to a big enough buffer to hold "len" bytes.
void GenRandom(char *s, const int len) {

	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	
	for (auto i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	
	s[len-1] = 0;
}



// ============================== PRODUCE ==================================================
void Produce(int nrOfMessages) {

	TYPE type = PRODUCER;
	std::string fielName = "sharedFile";

	char* messageToSend = NULL;
	size_t lastSize = 2048;

	messageToSend = new char[2048];
	
	ComLib comLib(fielName, memorySize, type);	

	int count = 0;
	while (count < nrOfMessages) {

		size_t msgLen = msgSize;

		if (msgLen > lastSize) {
			lastSize = msgLen;
			delete[] messageToSend;
			messageToSend = new char[msgLen];
		}

		GenRandom(messageToSend, msgLen);

		bool sent = comLib.send(messageToSend, msgLen);

		if (sent) {
			std::cout << messageToSend << std::endl;
			count++;

		}

		Sleep(delay);
	}

	delete[] messageToSend;
}

// =============================== CONSUME =================================================
void Consume(int nrOfMessages) {
	

	TYPE type = CONSUMER;
	std::string fielName = "sharedFile";

	char* msgToRecieve = NULL;
	size_t lastSize = 2048;

	std::string message;

	msgToRecieve = new char[2048];

	ComLib comLib(fielName, memorySize, type);

	int count = 0;
	while (count < nrOfMessages) {
		Sleep(delay);

		bool received = false; 

		size_t nextMsgSize = comLib.nextSize();
		if (nextMsgSize > lastSize) {
			lastSize = nextMsgSize;
			delete[] msgToRecieve;
			msgToRecieve = new char[nextMsgSize];
		}
		

		received = comLib.recv(msgToRecieve, nextMsgSize);
		if (received == false) {
			continue;
		}
		
		else if (received) {

			message = std::string(msgToRecieve);
			std::cout << message << std::endl;
			count++;
		}

	}

	delete[] msgToRecieve;
}