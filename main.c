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
#include <time.h>		//for struct tm
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

//function to convert input time to normal time
time_t gettime(char *T)
{
	char *ch = strtok(T, "_"); 		//Tokenise wrt _
	int arr[6];
	int i = 0;
	struct tm tm1={0};
	//store in array
	while(ch != NULL)
	{
		arr[i++] = atoi(ch);
		ch = strtok(NULL, "_");
	}
	while(i < 6)
		arr[i++] = 0;
	if(arr[2] == 0)
		arr[2] = 1;

	tm1.tm_year = arr[0] - 1900;
	tm1.tm_mon = arr[1] - 1;
	tm1.tm_mday = arr[2];
	tm1.tm_hour = arr[3];
	tm1.tm_min = arr[4];
	tm1.tm_sec = arr[5];

	return mktime(&tm1);		//returns time_t
}


int client(int portnum, int fd1, char *IP)
{
	int n = 0, serverfd = 0, command, size, fr_block_sz;
	char *srecvBuff, *crecvBuff;
	char clientInput[1025];		//Buffer for server and client
	struct sockaddr_in serverAddress;
	char line[1000];
	struct Operation cFileDownload;
	struct Operation cFileUpload;

	serverAddress.sin_family = AF_INET;		//IPv4
	serverAddress.sin_port = htons(portnum);		//Ntwork ordering
	serverAddress.sin_addr.s_addr = inet_addr(IP);		//conversion to binary of an IP

	if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nError creating socket \n");
		return 1;
	}
	else
		printf("Ceated socket\n");
	
	int check=0;

	while((connect(serverfd,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) && check < 100)
	{
		check++;
		sleep(1);
	}

	printf("Connection to server on port %d successful\n", portnum);

	while(1)
	{
		scanf("%s", clientInput);	//scanning for inputs
		//printf("Received command : %s\n", clientInput);

		if(strcmp(clientInput, "FileUploadDeny") == 0)
		{
			printf("REJECTING\n");
			//Write to file descriptor
			write(fd1, "FileUploadDeny",(strlen("FileUploadDeny") + 1));
		}

		if(strcmp(clientInput, "FileUploadAllow") == 0)
		{
			printf("ALLOWING\n");
			write(fd1, "FileUploadAllow",(strlen("FileUploadAllow") + 1));
		}

		if(strcmp(clientInput, "FileDownload") == 0)
		{
			cFileDownload.command = FileDownload;
			scanf("%s", cFileDownload.fileName);
			printf("Dowloading file %s ...\n", cFileDownload.fileName);
			command = FileDownload;

			//Send command to server
			n = write(serverfd, &command, sizeof(int));
			if(n == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);

			//Send cFileDownload
			n=write(serverfd, &cFileDownload, sizeof(cFileDownload));
			if(n == -1)
				printf("Failed to send cFileDownload\n");
			else
				printf("Sent cFileDownload %s\n", cFileDownload.fileName);

			printf("Receiving file from Server ...\n");

			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, cFileDownload.fileName);
			FILE *f = fopen(temp, "w+");

			//Creating file to copy downloaded contents
			if(f == NULL)
			{
				printf("Error creating file %s\n", temp);
				return 1;
			}
			else
				printf("created file %s\n", temp);
			fr_block_sz = 0;
		    // receive the file size
			size = 0;
			fr_block_sz =recv(serverfd, &size, sizeof(int), 0);
			if(fr_block_sz!= sizeof(int))
			{
				printf("Error reading size of file %s\n", temp);
				return 0;
			}
			else
				printf("Received size of file %d\n", size);

			int recvdsize = 0;
			//Buffer to receive data from server
			crecvBuff =(char *) malloc(size * sizeof(char));

		    // save original pointer to calculate MD5
			while((fr_block_sz = recv(serverfd, crecvBuff, LENGTH, 0)) > 0)
			{
				crecvBuff[fr_block_sz] = 0;
				int write_sz =fwrite(crecvBuff, sizeof(char), fr_block_sz, f);	//Write to fileptr f
				crecvBuff += fr_block_sz;
				if(write_sz == -1)
				{
					printf("Error writing to file %s\n", temp);
					return 0;
				}

				recvdsize += fr_block_sz;
				printf("%d\n", recvdsize);
				if(recvdsize >= size)
					break;
			}
			printf("done!\n");
			fclose(f);
			n = 0;
		}

		else if(strcmp(clientInput, "FileHash") == 0)
		{

			struct sFileHash_response cFileHash_response;
			struct sFileHash cFileHash;
			struct stat vstat;
			int num_responses;
			command = FileHash;
			int i;

		    // set the FileHash command
			cFileHash.command = FileHash;

		    // get the type
			scanf("%s", cFileHash.type);
			printf("FileHash type %s ...\n", cFileHash.type);

		    // get the fileName if Verify
			if(strcmp(cFileHash.type, "Verify") == 0)
			{
				scanf("%s", cFileHash.fileName);
				printf("FileHash file %s ...\n", cFileHash.fileName);
			}

		    //sending command name
		    n = write(serverfd, &command, sizeof(int));
			if(n == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);

		    // send cFileHash with the the information
			if(write(serverfd, &cFileHash, sizeof(cFileHash)) == -1)
				printf("Failed to send    cFileHash\n");
			else
				printf("Sent cFileHash %s %s \n", cFileHash.type,
					cFileHash.fileName);

		    // receive number of file hash resposes to expect
		    n =recv(serverfd, &num_responses, sizeof(num_responses),0);
		    //n==1 if VERIFY, and N if checkAll (N==number of files in directory)
			if( n!= sizeof(num_responses))
			{
				printf("Error reading number of responses of file\n");
				return 0;
			}
			else
				printf("Expecting %d responses\n", num_responses);

		    // print each file hash response
			for(i = 0; i < num_responses; i++)
			{
				n =recv(serverfd, &cFileHash_response, sizeof(cFileHash_response),0);
				if(n!= sizeof(cFileHash_response))
				{
					printf("Error reading cFileHash_response of file %s\n",
						cFileHash.fileName);
					return 0;
				}
				else
				{
					printf("Received cFileHash_response of file %s \n", cFileHash_response.fileName);
					printf("Last Modified @ %s\n",cFileHash_response.time_modified);
					printf("MD5 %01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x\n\n",
						cFileHash_response.md5Context.digest[0],
						cFileHash_response.md5Context.digest[1],
						cFileHash_response.md5Context.digest[2],
						cFileHash_response.md5Context.digest[3],
						cFileHash_response.md5Context.digest[4],
						cFileHash_response.md5Context.digest[5],
						cFileHash_response.md5Context.digest[6],
						cFileHash_response.md5Context.digest[7],
						cFileHash_response.md5Context.digest[8],
						cFileHash_response.md5Context.digest[9],
						cFileHash_response.md5Context.digest[10],
						cFileHash_response.md5Context.digest[11],
						cFileHash_response.md5Context.digest[12],
						cFileHash_response.md5Context.digest[13],
						cFileHash_response.md5Context.digest[14],
						cFileHash_response.md5Context.digest[16]);
				}
			}
		}

		else if(strcmp(clientInput, "FileUpload") == 0)
		{

			cFileUpload.command = FileUpload;
			scanf("%s", cFileUpload.fileName);
			printf("Uploading file %s ...\n", cFileUpload.fileName);

			command = FileUpload;

		    //sending command name
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);

		    //opening the file
			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, cFileUpload.fileName);

			char *fname = temp;
			FILE *fs = fopen(fname, "r");
			if(fs == NULL)
			{
				printf("Unable to open file.\n");
				return 0;
			}

			int block;
			char *readbuf;
			struct stat vstat;

		    // get size of file
			if(stat(temp, &vstat) == -1)
			{
				printf("vstat error\n");
				return 0;
			}

			if(write(serverfd, &cFileUpload, sizeof(cFileUpload)) == -1)
				printf("Failed to send  cFileUpload\n");
			else
				printf("Sent cFileUpload %s\n", cFileUpload.fileName);

			size = vstat.st_size;

			if(send(serverfd, &size, sizeof(int), 0) < 0)
			{
				printf("send error\n");
				return 0;
			}
			else
				printf("sending file size %d\n", size);

		    //waiting for accept or deny:
			char result[100];
			if((n = read(serverfd, &result, sizeof(result))) <= 0)
				printf("Error reading result\n");
			printf("%s", result);
			if(strcmp(result, "Deny") == 0)
			{
				printf("Upload denied.\n");
				fclose(fs);
				continue;
			}
			else
				printf("Upload accepted\n");
			readbuf =(char *) malloc(size * sizeof(char));
			if(readbuf == NULL)
			{
				printf("error, No memory\n");
				exit(0);
			}

		    //sending the file
			while((block = fread(readbuf, sizeof(char), size, fs)) > 0)
			{
				if(send(serverfd, readbuf, block, 0) < 0)
				{
					printf("send error\n");
					return 0;
				}
			}
			n = 0;
		}

		else if(strcmp(clientInput, "IndexGet") == 0)
		{
			char opt[1000];
			char *filelist;
			command = IndexGet;
			struct stat vstat;
			struct sIndexGet fstat;
			int lenfiles = 0;
			char buff[1000];
			char t1[200], t2[200];
			time_t timt1, timt2;
			char regex[100], fname[1000];
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);
			scanf("%s", opt);
			puts(opt);
			if(strcmp(opt, "ShortList") == 0)
			{
				scanf("%s %s", t1, t2);
				
				timt1 = gettime(t1);
				timt2 = gettime(t2);
			}

			if(strcmp(opt, "RegEx") == 0)
			{
				system("touch res");
				scanf("%s", regex);

			}
			FILE *fp = fopen("./res", "a");

			if((fr_block_sz =recv(serverfd, &lenfiles, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading number of file\n");
				return 0;
			}
			else
				printf("Recieved number of files %d\n", lenfiles);
			int i;

			for(i = 0; i < lenfiles; i++)
			{
				if((n =	read(serverfd,(void *) &fstat, sizeof(fstat))) != sizeof(fstat))
					printf("Error reciving vstat\n");

				if(strcmp(opt, "LongList") == 0)
				{
					printf("Filename: ");
					puts(fstat.fileName);
					vstat = fstat.fileDetails;
					printf("Size: %d\t",(int) vstat.st_size);
					ctime_r(&vstat.st_mtime, buff);
					printf("Time Modified: %s\n", buff);
				}

				else if(strcmp(opt, "ShortList") == 0)
				{
					vstat = fstat.fileDetails;
					if(difftime(vstat.st_mtime, timt1) >= 0
						&& difftime(timt2, vstat.st_mtime) >= 0)
					{
						printf("Filename: ");
						puts(fstat.fileName);
						vstat = fstat.fileDetails;
						printf("Size: %d\t",(int) vstat.st_size);
						ctime_r(&vstat.st_mtime, buff);
						printf("Time Modified: %s\n", buff);
					}
				}

				else if(strcmp(opt, "RegEx") == 0)
				{

					fwrite(fstat.fileName, 1, strlen(fstat.fileName), fp);
					fwrite("\n", 1, 1, fp);
				}
			}
			fclose(fp);
			if(strcmp(opt, "RegEx") == 0)
			{
				char call[10000];
				strcpy(call, "cat res | grep -E ");
				strcat(call, regex);
				strcat(call, " ; rm -rf res");
				system(call);
			}

		}

		n = 0;
		sleep(1);
	}
	return 0;
}
int main()
{
	//CMD c;

	return 0;
}