#ifndef  COMLIB_H
#define COMLIB_H

#include <string>
#include <iostream>
#include <windows.h>

// ===== TYPE DEFINITIONS =====
enum MSG_TYPE { NORMAL = 0, DUMMY = 1 };
enum TYPE { PRODUCER, CONSUMER };


// ===== HEADER STRUCT =====
struct Header {
	MSG_TYPE type;
	size_t length;
};

// ================= COMLIB CLASS =================
class ComLib {

	// ===== VARIABLES =====
private:
	TYPE comLibType;

	HANDLE hFileMap;
	void* mData;

	//Variables for memory
	char* base;
	char* msgBase; //base for where message starts

	size_t* head;
	size_t* tail;

	//buffers
	size_t totalBuffSize;	  //the total size of the buffer
	size_t messageBufferSize; //the size of the buffer assigned for the message


// ===== FUNCTIONS =====
public:
	//ComLib(); 
	// Constructor
	// secret is the known name for the shared memory
	// buffSize is in MEGABYTES (multiple of 1<<20)
	// type is TYPE::PRODUCER or TYPE::CONSUMER
	ComLib(const std::string& secret, const size_t& buffSize, TYPE type);

	// returns "true" if data was sent successfully.
	// false if for ANY reason the data could not be sent.
	// we will not implement an "error handling" mechanism, so we will assume
	// that false means that there was no space in the buffer to put the message.
	// msg is a void pointer to the data.
	// length is the amount of bytes of the message to send.
	bool send(const void* msg, const size_t length);

	// returns: "true" if a message was received.
	// false if there was nothing to read.
	// "msg" is expected to have enough space for the message.
	// use "nextSize()" to check whether our temporary buffer is big enough
	// to hold the next message.
	// @length returns the size of the message just read.
	// @msg contains the actual message read.
	bool recv(char* &msg, size_t& length);

	// return the length of the next message
	// return 0 if no message is available.
	size_t nextSize();

	//check how much memory is avaliable
	//and return it
	size_t CheckFreeMemory(size_t tempTail, size_t msgLen);

	/* destroy all resources */
	~ComLib();
};

#endif

