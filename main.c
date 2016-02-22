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

#define LENGTH 1000

// FileDownload=0, FileUpload=1, FileHash=2, IndexGet=3
typedef enum
{	FileDownload, FileUpload, FileHash,	IndexGet } CMD;

//To hold details of files being shown
struct sIndexGet
{
	CMD command;
	struct stat fileDetails;		//from stat()
	char fileName[1000];
};

//To send/receive to/from server/client for upload/download

 /*********************************************************** Do we need 2 different ones? ***********************************************/
struct Operation
{
	CMD command;
	char fileName[1000];
};


struct sFileHash
{
	CMD command;
	char type[1000];
	char fileName[100];
};

struct sFileHash_response
{
	char fileName[128];
	MD5_CTX md5Context;
	char time_modified[128];
};

struct sFileHash_response sFileHash_response;
struct sFileHash sFileHash;

int server(int portNo, int fdUpload); // Prototype

int server ( int portNo, int fdUpload )
{
	struct sockaddr_in s_addr, c_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(portnum);

	int fdListen = 0;
	fdListen = socket(AF_INET, SOCK_STREAM, 0);
	cout<<"Created Socket\n";
	bind( fdListen ,(struct sockaddr *) &s_addr, sizeof(s_addr));
	
	if(portNo == -1)
		cout<<"Error listening to port!\n"	;
		return -1;

	int fdClient = 0;
	if( fdClient = accept(listenfd,(struct sockaddr *) NULL, NULL) == -1)	// accept awaiting request
	    cout<<"Couldn't accept client request!\n";

	int c = 0, cmd, d; // file decriptors for command, download file

	struct Operation downloadFile;

	while(1)
	{
		c = read(fdClient, &cmd, sizeof(int) ); // Take input for command to be performed
		if( c>0 )
		{
			cout<<"received!\n";
			c = 0;
		}

		switch( (CMD) cmd)
		{
			case FileDownload:
			/*  begin */
				printf("Download File!\n");
				if((d =read(fdClient,(void *) &downloadFile,sizeof(downloadFile))) != sizeof(downloadFile))
    				printf("Error reading filename\n");
    			printf("Got FileDownload command\n");
    		
	    		char dest[100];
	    		strcpy(dest, "./shared/");
	    		strcat(dest, downloadFile.filename);

	    		char *fname = dest;
	    		if( (FILE *fs = fopen(dest, "r") ) == NULL )
	    		{
	    			printf("Unable to open file.\n");
	    			return 0;
	    		}

	    		struct stat filestat;
		    	// get size of file
	    		if(stat(dest, &filestat) == -1)
	    		{
	    			printf("vstat error\n");
	    			return 0;
	    		}
		    // send file size 
	    		int block;
	    		char *readBuffer;
	    		int size = filestat.st_size;
	    		if( (readBuffer = (char *) malloc(size * sizeof(char) ) ) == NULL)
	    		{
	    			printf("error, No memory\n");
	    			exit(0);
	    		}


	    		if(send(fdClient, &size, sizeof(int), 0) < 0)
	    		{
	    			printf("send error\n");
	    			return 0;
	    		}

	    		else
	    			printf("sending file of size %d\n", size);

	    	// send file block by block
	    		while((block = fread(readBuffer, sizeof(char), size, fs)) > 0)
	    		{
	    			if(send(fdClient, readBuffer, block, 0) < 0)
	    			{
	    				printf("send error\n");
	    				return 0;
	    			}
	    			printf("File Sent\n");
	    		}
	    		fclose(fs);
			/*end case */
			break;

			case IndexGet:
			/* begin */
				char list[1000];
				fileget(list, fdClient);
			/* end case */
				break;

			case FileUpload:
			

		}

	}
}

int main()
{
	//CMD c;

	return 0;
}