#include "ComLib.h"
#include <iostream>

// ============================= CONSTRUCTOR ===============================================
ComLib::ComLib(const std::string& secret, const size_t& buffSize, TYPE type) {

	// set type of message and total buffer size
	comLibType = type;
	totalBuffSize = (1 << 20) * buffSize;


	//creates or opens a file mapping object for a 
	//specified file 
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		totalBuffSize,
		(LPCSTR)secret.c_str());

	if (hFileMap == NULL) {
		std::cout << "ERROR! Could not create mapping object!" << std::endl;
		getchar();
		exit(-1);
	}

	bool first = true;
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		//std::cout << "ERROR! shared memory already exists" << std::endl << std::endl;
		first = false;
	}


	//maps a view of a file mapping into 
	//the address space
	this->mData = MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (mData == NULL) {
		std::cout << "Could not map view of file!" << std::endl;
		getchar();
		exit(-1);
	}

	//set base to the fist   
	base = (char*)mData;
	head = (size_t*)base;
	tail = head + 1;

	// if producer, initialize content of head and tail to 0,
	// if consumer, do nothing.
	if (first) {
		*head = 0;
		*tail = 0;
	}

	//new base where message starts
	msgBase = (char*)(tail + 1);

	//get the size of the buffer where messages can be placed
	messageBufferSize = totalBuffSize - (2 * sizeof(size_t));

}

// =========================================================================================

size_t ComLib::CheckFreeMemory(size_t tempTail, size_t msgLen) {

	size_t freeMemory = 0;
	//bool full = false;

	//if the head is behind the tail, the free memory 
	//is the tail - head
	if (*head < tempTail) {
		freeMemory = tempTail - *head;
	}

	//if head is in front of or equal to tail, the free momory is 
	// the messagebuffer - head
	if (*head >= tempTail) {
		freeMemory = messageBufferSize - *head;
	}

	return freeMemory;
}

size_t ComLib::nextSize() {

	size_t msgLenght = 0;

	//check if the head != tail. If they are, 
	//there is no message to be read
	if (*tail != *head) {

		Header hdr = { DUMMY, 0 };
		memcpy(&hdr, msgBase + (*tail), sizeof(Header));
		/*msgLenght = hdr.length;*/

		//if the message type is normal, there is a message
		if (hdr.type == NORMAL) {
			//msgLenght = hdr.length;
			msgLenght = ((Header*)(msgBase + (*tail)))->length;
		}

		//if the type is dummy, just treat it as if there
		//is no message
		else if (hdr.type == DUMMY) {
			msgLenght = 0;
		}

	}

	return msgLenght;
}

// ================================ SEND ===================================================

bool ComLib::send(const void* msg, const size_t length) {

	bool messageSent = false;

	Header hdr;
	size_t tempTail = *tail;

	size_t freeMemory = CheckFreeMemory(tempTail, length);
	size_t minFreeMemory = length + (sizeof(Header) * 2);

	//if there is memory for the message, write it
	if (freeMemory > minFreeMemory) {

		if ((size_t)(msgBase + *head + (sizeof(Header) + length)) == tempTail) {
			messageSent = false;
		}


		hdr.type = NORMAL;
		hdr.length = length;

		//copy header info (with *(Header*)(base + (*head)) = hdr or memcpy) 
		//and message
		memcpy(msgBase + (*head), &hdr, sizeof(Header));
		memcpy(msgBase + (*head) + sizeof(Header), msg, length);

		//increment header after copy
		*head = *head + sizeof(Header) + length;

		messageSent = true;


	}

	//if there is not enough free space, 
	//fill with dummy message
	else {

		//check to make sure that the tail is not 0
		if (tempTail == 0) {
			messageSent = false;
		}


		//if it isn't, a dummy message can be sent
		if (tempTail != 0) {

			hdr.type = DUMMY;
			hdr.length = 0;
			memcpy(msgBase + (*head), &hdr, sizeof(Header));

			//reset for the new message
			*head = 0;

			//send the original message again
			if (send(msg, length))
				messageSent = true;

			else
				messageSent = false;

		}
	}


	return messageSent;
}

// ================================ RECV ===================================================

bool ComLib::recv(char* &msg, size_t& length) {

	bool messageReceived = false;
	bool headIsInfrontOfTail = true;

	Header hdr;
	size_t tempHead = *head;

	memcpy(&hdr, msgBase + (*tail), sizeof(Header));


	if (hdr.type == NORMAL) {

		if (*tail != tempHead) {

			//std::cout << "RECIEVE" << std::endl;

			//copy it into the new variable
			memcpy(msg, (msgBase + (*tail) + sizeof(Header)), hdr.length);

			//increment tail
			*tail = *tail + sizeof(Header) + hdr.length;

			messageReceived = true;

		}
	}

	else if (hdr.type == DUMMY) {
		if (*tail != tempHead) {
			*tail = 0;
			messageReceived = false;
		}
	}


	return messageReceived;
}

// ============================== DESTRUCTOR ===============================================

ComLib::~ComLib() {

	//if CONSUMER, unmap view of file and close the handle
	UnmapViewOfFile((LPCVOID)mData);
	CloseHandle(hFileMap);

}

// =========================================================================================