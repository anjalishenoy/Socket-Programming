#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <time.h>
#include "md5.h"

typedef enum
{	FileDownload, FileUpload, FileHash,	IndexGet } CMD;

struct sIndexGet
{
	CMD command;
	struct stat fileDetails;
	char fileName[1000];
};

struct sFileDownload
{
	CMD command;
	char fileName[1000];
};

struct sFileUpload
{
	CMD command;
	char fileName[100];
};

struct sFileHash
{
	CMD command;
	char type[1000];
	char fileName[100];
};

struct sFileHash_response
{
	char filename[128];
	MD5_CTX md5Context;
	char time_modified[128];
};

struct sFileHash_response sFileHash_response;
struct sFileHash sFileHash;
int client()
{
	struct sockaddr_in server_addr, client_addr;
}

int main()
{
	//CMD c;

	return 0;
}