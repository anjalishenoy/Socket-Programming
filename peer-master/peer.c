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
#define MD5LENGTH 16
#define FALSE 0
#define TRUE !FALSE

typedef enum
{
	FileDownload,
	FileUpload,
	FileHash,
	IndexGet
} CMD;

struct sFileDownload
{
	CMD command;
	char filename[128];
};

struct sFileUpload
{
	CMD command;
	char filename[128];
};

struct sFileHash
{
	CMD command;
	char type[128];
	char filename[128];
};

struct sFileHash_response
{
	char filename[128];
	MD5_CTX md5Context;
	char time_modified[128];
};


// globals
struct sFileHash_response sFileHash_response;
struct sFileHash sFileHash;

// get the file count in a directory
int GetFileCnt()
{

	int num_files = 0;
	struct dirent *result;
	DIR *fd;
	char temp[100];

    // open the directory
	strcpy(temp, "./shared/");
	fd = opendir(temp);
	if(NULL == fd)
	{
		printf("Error opening directory %s\n", temp);
		return 0;
	}

	while((result = readdir(fd)) != NULL)
	{
		if(!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))
			continue;
		printf("%s\n", result->d_name);
		num_files++;
	}
	closedir(fd);

	return num_files;
}

// get next file
char * GetNxtFile(DIR * fd)
{

	struct dirent *result;

	while((result = readdir(fd)) != NULL)
	{
		if(!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))
			continue;
		printf("%s\n", result->d_name);
		return result->d_name;
	}
	return NULL;
}

// get md5 of buffer
void Getmd5(char *readbuf, int size)
{
	MD5Init(&sFileHash_response.md5Context);
	MD5Update(&sFileHash_response.md5Context,(unsigned char *) readbuf, size);
	MD5Final(&sFileHash_response.md5Context);
}


// get file hash information for current sFileHash
void getFileHash()
{

	char temp[100];
	struct stat vstat;
	char *fname = temp;
	int block;
	char *readbuf;

    // copy filename into the response
	strcpy(sFileHash_response.filename, sFileHash.filename);

	strcpy(temp, "./shared/");
	strcat(temp, sFileHash.filename);

    // open file
	FILE *fs = fopen(fname, "r");
	if(fs == NULL)
	{
		printf("Unable to open file.\n");
		return;
	}

    // get time modified of file
	if(stat(temp, &vstat) == -1)
	{
		printf("fstat error\n");
		return;
	}

    // copy the tiem modified into the response
	ctime_r(&vstat.st_mtime, sFileHash_response.time_modified);


    // allocate space for readng the file
	readbuf =(char *) malloc(vstat.st_size * sizeof(char));
	if(readbuf == NULL)
	{
		printf("error, No memory\n");
		exit(0);
	}

    // read the file
	printf("Reading file...\n");
	block = fread(readbuf, sizeof(char), vstat.st_size, fs);
	if(block != vstat.st_size)
	{
		printf("Unable to read file.\n");
		return;
	}

    // get the md5 of the file into the response
	Getmd5(readbuf, vstat.st_size);

    // print the final response
	printf("sFileHash_response of file %s \n", sFileHash_response.filename);
	printf("    - Last Modified @ %s", sFileHash_response.time_modified);
	printf
	("    - MD5 %01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x\n",
		sFileHash_response.md5Context.digest[0],
		sFileHash_response.md5Context.digest[1],
		sFileHash_response.md5Context.digest[2],
		sFileHash_response.md5Context.digest[3],
		sFileHash_response.md5Context.digest[4],
		sFileHash_response.md5Context.digest[5],
		sFileHash_response.md5Context.digest[6],
		sFileHash_response.md5Context.digest[7],
		sFileHash_response.md5Context.digest[8],
		sFileHash_response.md5Context.digest[9],
		sFileHash_response.md5Context.digest[10],
		sFileHash_response.md5Context.digest[11],
		sFileHash_response.md5Context.digest[12],
		sFileHash_response.md5Context.digest[13],
		sFileHash_response.md5Context.digest[14],
		sFileHash_response.md5Context.digest[16]);

    // return
	return;
}


struct sIndexGet
{
	CMD command;
	struct stat vstat;
	char filename[128];
};
int file_select(const struct direct *entry)
{
	if((strcmp(entry->d_name, ".") == 0)
		||(strcmp(entry->d_name, "..") == 0))
		return(FALSE);
	else
		return(TRUE);
}


int fileget(char *buf, int Fclientfd)
{
	int count, i;
	struct direct **files;
    //int file_select();
	struct stat vstat;
	struct sIndexGet fstat;
	count = scandir("./shared", &files, file_select, alphasort);

	if(count <= 0)
	{
		strcat(buf, "No files in this directory\n");
		return 0;
	}
	if(send(Fclientfd, &count, sizeof(int), 0) < 0)
	{
		printf("send error\n");
		return 0;
	}
	char dir[200];
	strcpy(dir, "./shared/");
	strcat(buf, "Number of files = ");
	sprintf(buf + strlen(buf), "%d\n", count);
	for(i = 0; i < count; ++i)
	{
		strcpy(dir, "./shared/");
		strcat(dir, files[i]->d_name);
		if(stat(dir, &(vstat)) == -1)
		{
			printf("fstat error\n");
			return 0;
		}
		fstat.vstat = vstat;
		strcpy(fstat.filename, files[i]->d_name);


		if(write(Fclientfd, &fstat, sizeof(fstat)) == -1)
			printf("Failed to send vstat\n");
		else
			printf("Sent vstat\n");
	//sprintf(buf+strlen(buf),"%s    ",files[i-1]->d_name);
	}
//            strcat(buf,"\n"); 
	return 0;
}

time_t gettime(char *intime)
{
	char *ch = strtok(intime, "_");
	int time1[6];
	int i = 0;
	struct tm tm1={0};
	while(ch != NULL)
	{
		time1[i++] = atoi(ch);
		ch = strtok(NULL, "_");
	}
	while(i < 6)
		time1[i++] = 0;
	if(time1[2] == 0)
		time1[2] = 1;

	tm1.tm_year = time1[0] - 1900;
	tm1.tm_mon = time1[1] - 1;
	tm1.tm_mday = time1[2];
	tm1.tm_hour = time1[3];
	tm1.tm_min = time1[4];
	tm1.tm_sec = time1[5];

	return mktime(&tm1);
}

int client(int portnum, int fd1, char *IP)
{
	int n = 0, listenfd = 0, serverfd = 0;;
	char *srecvBuff, *crecvBuff;
	char ssendBuff[1025], csendBuff[1025];
	struct sockaddr_in serv_addr, c_addr;
	int numrv;
	char line[1000];
	struct sFileDownload cFileDownload;
	struct sFileDownload sFileDownload;
	struct sFileUpload cFileUpload;
	struct sFileUpload sFileUpload;

	memset(&c_addr, '0', sizeof(c_addr));
	memset(csendBuff, '0', sizeof(csendBuff));
	memset(ssendBuff, '0', sizeof(ssendBuff));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portnum);
	serv_addr.sin_addr.s_addr = inet_addr(IP);

	if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\nError creating socket \n");
		return 1;
	}
	else
		printf("created socket\n");
	
	int timeout=0;
	while((connect(serverfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) && timeout < 5000)
	{
		timeout++;
		sleep(1);
	}

	printf("Connect to server on port %d successful\n", portnum);

	while(1)
	{
		scanf("%s", csendBuff);	//scanning for inputs
		printf("Received command : %s\n", csendBuff);

		if(strcmp(csendBuff, "FileUploadDeny") == 0)
		{
			printf("REJECTING!!!!!!!\n");
			write(fd1, "FileUploadDeny",(strlen("FileUploadDeny") + 1));
		}

		if(strcmp(csendBuff, "FileUploadAllow") == 0)
			write(fd1, "FileUploadAllow",(strlen("FileUploadAllow") + 1));

		if(strcmp(csendBuff, "FileDownload") == 0)
		{
			cFileDownload.command = FileDownload;
			scanf("%s", cFileDownload.filename);
			printf("Dowloading file %s ...\n", cFileDownload.filename);
			int command = FileDownload;
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", n, csendBuff);

			if(write(serverfd, &cFileDownload, sizeof(cFileDownload)) == -1)
				printf("Failed to send        cFileDownload\n");
			else
				printf("Sent cFileDownload %s\n", cFileDownload.filename);

			char revbuf[LENGTH];
			printf("Receiveing file from Server ...\n");

			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, cFileDownload.filename);
			FILE *fr = fopen(temp, "w+");
			if(fr == NULL)
			{
				printf("Error creating file %s\n", temp);
				return 0;
			}
			else
				printf("created file %s\n", temp);
			bzero(revbuf, LENGTH);
			int fr_block_sz = 0;
		    // receive the file size
			int size = 0;
			if((fr_block_sz =
				recv(serverfd, &size, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading size of file %s\n", temp);
				return 0;
			}
			else
				printf("Received size of file %d\n", size);

			int recvdsize = 0;
			crecvBuff =(char *) malloc(size * sizeof(char));
		    // save original pointer to calculate MD5
			while((fr_block_sz = recv(serverfd, crecvBuff, LENGTH, 0)) > 0)
			{
				crecvBuff[fr_block_sz] = 0;
				int write_sz =
				fwrite(crecvBuff, sizeof(char), fr_block_sz, fr);
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
			fclose(fr);
			n = 0;
		}

		else if(strcmp(csendBuff, "FileHash") == 0)
		{

			struct sFileHash_response cFileHash_response;
			struct sFileHash cFileHash;
			struct stat vstat;
			int num_responses;
			int command = FileHash;
			int i;

		    // set the FileHash command
			cFileHash.command = FileHash;

		    // get the type
			scanf("%s", cFileHash.type);
			printf("FileHash type %s ...\n", cFileHash.type);

		    // get the filename if Verify
			if(strcmp(cFileHash.type, "Verify") == 0)
			{
				scanf("%s", cFileHash.filename);
				printf("FileHash file %s ...\n", cFileHash.filename);
			}

		    //sending command name
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", n, csendBuff);

		    // send cFileHash with the the information
			if(write(serverfd, &cFileHash, sizeof(cFileHash)) == -1)
				printf("Failed to send    cFileHash\n");
			else
				printf("Sent cFileHash %s %s \n", cFileHash.type,
					cFileHash.filename);

		    // receive number of file hash resposes to expect
			if((n =
				recv(serverfd, &num_responses, sizeof(num_responses),
					0) != sizeof(num_responses)))
			{
				printf("Error reading number of responses of file\n");
				return 0;
			}
			else
				printf("Expecting %d responses\n", num_responses);

		    // print each file hash response
			for(i = 0; i < num_responses; i++)
			{

				if((n =
					recv(serverfd, &cFileHash_response,
						sizeof(cFileHash_response),
						0) != sizeof(cFileHash_response)))
				{
					printf("Error reading cFileHash_response of file %s\n",
						cFileHash.filename);
					return 0;
				}
				else
				{
					printf("Received cFileHash_response of file %s \n",
						cFileHash_response.filename);
					printf("Last Modified @ %s\n",
						cFileHash_response.time_modified);
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

		else if(strcmp(csendBuff, "FileUpload") == 0)
		{

			cFileUpload.command = FileUpload;
			scanf("%s", cFileUpload.filename);
			printf("Uploading file %s ...\n", cFileUpload.filename);

			int command = FileUpload;

		    //sending command name
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", n, csendBuff);

		    //opening the file
			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, cFileUpload.filename);

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
				printf("Failed to send        cFileUpload\n");
			else
				printf("Sent cFileUpload %s\n", cFileUpload.filename);

			int size = vstat.st_size;

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
			if(strcmp(result, "FileUploadDeny") == 0)
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

		else if(strcmp(csendBuff, "IndexGet") == 0)
		{
			char opt[1000];
			char *filelist;
			int command = IndexGet;
			struct stat vstat;
			struct sIndexGet fstat;
			int lenfiles = 0;
			int fr_block_sz;
			char buff[1000];
			char t1[200], t2[200];
			time_t timt1, timt2;
			char regex[100], fname[1000];
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", csendBuff);
			else
				printf("Command sent: %d %s\n", n, csendBuff);
			
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

			if((fr_block_sz =
				recv(serverfd, &lenfiles, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading number of file\n");
				return 0;
			}
			else
				printf("Recieved number of files %d\n", lenfiles);
			int i;

			for(i = 0; i < lenfiles; i++)
			{
				if((n =
					read(serverfd,(void *) &fstat,
						sizeof(fstat))) != sizeof(fstat))
					printf("Error reciving vstat\n");

				if(strcmp(opt, "LongList") == 0)
				{
					printf("Filename: ");
					puts(fstat.filename);
					vstat = fstat.vstat;
					printf("Size: %d\t",(int) vstat.st_size);
					ctime_r(&vstat.st_mtime, buff);
					printf("Time Modified: %s\n", buff);
				}

				else if(strcmp(opt, "ShortList") == 0)
				{
					vstat = fstat.vstat;
					if(difftime(vstat.st_mtime, timt1) >= 0
						&& difftime(timt2, vstat.st_mtime) >= 0)
					{
						printf("Filename: ");
						puts(fstat.filename);
						vstat = fstat.vstat;
						printf("Size: %d\t",(int) vstat.st_size);
						ctime_r(&vstat.st_mtime, buff);
						printf("Time Modified: %s\n", buff);
					}
				}

				else if(strcmp(opt, "RegEx") == 0)
				{

					fwrite(fstat.filename, 1, strlen(fstat.filename), fp);
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

int server(int portnum, int fd0)
{
	int clientfd = 0, n = 0, listenfd = 0;
	char *srecvBuff, *crecvBuff;
	char ssendBuff[1025], csendBuff[1025];
	struct sockaddr_in s_addr, c_addr;
	int numrv;
	char line[1000];
	struct sFileDownload cFileDownload;
	struct sFileDownload sFileDownload;
	struct sFileUpload cFileUpload;
	struct sFileUpload sFileUpload;
	memset(&c_addr, '0', sizeof(c_addr));
	memset(csendBuff, '0', sizeof(csendBuff));
	memset(ssendBuff, '0', sizeof(ssendBuff));
	int cmd;

	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(portnum);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	printf("created socket\n");

	bind(listenfd,(struct sockaddr *) &s_addr, sizeof(s_addr));
	printf("listening on port %d\n", portnum);
	if(listen(listenfd, 10) == -1)
	{
		printf("Failed to listen\n");
		return -1;
	}
    clientfd = accept(listenfd,(struct sockaddr *) NULL, NULL);	// accept awaiting request
    if(clientfd == -1)
    	printf
   ("accept faileindent: Standard input:476: Warning:old style assignment ambiguity in \"=-\".        Assuming \"= -\"d\n");
    else
    	printf("accepted connection \n");

    while(1)
    {
    	if((n = read(clientfd, &cmd, sizeof(int))) > 0)
    	{
    		printf("Recieved %d %d\n", n, cmd);
    		n = 0;
    	}

    	if((CMD) cmd == FileDownload)
    	{
    		printf("Got FileDownload command\n");
    		if((n =read(clientfd,(void *) &sFileDownload,sizeof(sFileDownload))) != sizeof(sFileDownload))
    			printf("Error reading filename\n");
    		printf("Sending file %s\n", sFileDownload.filename);
    		char temp[100];
    		strcpy(temp, "./shared/");
    		strcat(temp, sFileDownload.filename);
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
	    // send file size first
    		int size = vstat.st_size;
    		readbuf =(char *) malloc(size * sizeof(char));
    		if(readbuf == NULL)
    		{
    			printf("error, No memory\n");
    			exit(0);
    		}
    		if(send(clientfd, &size, sizeof(int), 0) < 0)
    		{
    			printf("send error\n");
    			return 0;
    		}
    		else
    			printf("sending file size %d\n", size);

    		while((block = fread(readbuf, sizeof(char), size, fs)) > 0)
    		{
    			if(send(clientfd, readbuf, block, 0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			printf("File Sent\n");
    		}
    		fclose(fs);
    	}

    	else if((CMD) cmd == FileUpload)
    	{
    		char temp[100];
			printf("Got FileUpload command\n");

	   		if((n = read(clientfd,(void *) &sFileUpload,sizeof(sFileUpload))) != sizeof(sFileUpload))
    			printf("Error reading filename\n");
 
     		printf("File name: %s\n", sFileUpload.filename);
    		int size;
    		int fr_block_sz = 0;
    		if((fr_block_sz =
    			recv(clientfd, &size, sizeof(int), 0)) != sizeof(int))
    		{
    			printf("Error reading size of file %s\n", temp);
    			return 0;
    		}
    		else
    			printf("File size: %dB\n", size);

    		char result[100];
    		printf("FileUploadDeny or FileUploadAllow?\n");
    		read(fd0, result, sizeof("FileUploadAllow"));

    		if(strcmp(result, "FileUploadDeny") == 0)
    		{
    			if((n = write(clientfd, &result, sizeof(result))) == -1)
    				printf("Failed to send result %s\n", result);
    			else
    				printf("Result sent: %d %s\n", n, result);
    			continue;
    		}
    		else
    		{
    			printf("Accepted file\n");
    			write(clientfd, &result, sizeof(result));
    		}

    		strcpy(temp, "./shared/");
    		strcat(temp, sFileUpload.filename);
    		FILE *fr = fopen(temp, "w+");
    		if(fr == NULL)
    		{
    			printf("Error creating file %s\n", temp);
    			return 0;
    		}
    		else
    			printf("created file %s\n", temp);

    		srecvBuff =(char *) malloc(size * sizeof(char));
    		int recvdsize = 0;
    		while((fr_block_sz = recv(clientfd, srecvBuff, LENGTH, 0)) > 0)
    		{
    			srecvBuff[fr_block_sz] = 0;
    			int write_sz =
    			fwrite(srecvBuff, sizeof(char), fr_block_sz, fr);
    			srecvBuff += fr_block_sz;
    			if(write_sz == -1)
    			{
    				printf("Error writing to file %s\n", temp);
    				return 0;
    			}
    			else
    				printf("wrote to file %s %d bytes\n", temp, write_sz);
    			recvdsize += fr_block_sz;
    			if(recvdsize >= size)
    				break;

    		}
    		printf("Done Upload!\n");
    		fclose(fr);
    	}

    	if((CMD) cmd == FileHash)
    	{
    		MD5_CTX md5Context;
    		char sign[16];
    		int num_responses;
    		char temp[100];
    		printf("Got FileHash command\n");

    		if((n =read(clientfd,(void *) &sFileHash,sizeof(sFileHash))) != sizeof(sFileHash))
    			printf("Error reading cFileHash\n");
    		else
    			printf("Type:%s\n", sFileHash.type);

    		if(strcmp(sFileHash.type, "Verify") == 0)
    		{

    			printf("Got Verify\n");

    			num_responses = 1;

    			if(send(clientfd, &num_responses, sizeof(int), 0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent number of responses %d\n", num_responses);
    			printf("ALL DONE\n");
    			getFileHash();
    			if(send(clientfd, &sFileHash_response, sizeof(sFileHash_response),0) < 0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent sFileHash_response %s\n",
    					sFileHash_response.filename);
    		}
    		else if(strcmp(sFileHash.type, "CheckAll") == 0)
    		{
    			DIR *fd;
    			int i;
    			printf("Got CheckAll\n");
    			strcpy(temp, "./shared/");
    			fd = opendir(temp);
    			if(NULL == fd)
    			{
    				printf("Error opening directory %s\n", temp);
    				return 0;
    			}

    			num_responses = GetFileCnt();
    			printf("Found %d files in shared folder\n", num_responses);
    			if(send(clientfd, &num_responses, sizeof(num_responses), 0) <    0)
    			{
    				printf("send error\n");
    				return 0;
    			}
    			else
    				printf("sent number of responses %d\n", num_responses);

    			fd = opendir(temp);
    			for(i = 0; i < num_responses; i++)
    			{
    				strcpy(sFileHash.filename, GetNxtFile(fd));
    				getFileHash();

    				if(send(clientfd, &sFileHash_response,sizeof(sFileHash_response), 0) < 0)
    				{
    					printf("send error\n");
    					return 0;
    				}
    				else
    					printf("sent sFileHash_response %s\n",sFileHash_response.filename);
    			}
    			closedir(fd);
    		}
    		else
    		{
    			printf("RReceived invalid FileHash Command %s\n",
    				sFileHash.type);
    			return 0;
    		}

    	}
    	
    	else if((CMD) cmd == IndexGet)
    	{
    		char filelist[1000];
    		strcpy(filelist, "");
    		fileget(filelist, clientfd);
    		printf("File List Sent\n");
    	}
    	
    	if(n < 0)
    		printf("\n Read Error \n");
    }
    close(clientfd);
    return 0;
}

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		printf("Usage ./peer <IP> <Port of Remote Machine> <Port of Your Machine>\n");
		return -1;
	}
	struct stat st = {0};
	if (stat("./shared", &st) == -1) 
	    mkdir("./shared", 0700);
	
	int peer1 = atoi(argv[2]);
	int peer2 = atoi(argv[3]);
	int fd[2];
	int id = -1;

	pipe(fd);
	id = fork();

	if(id > 0)
	{
		close(fd[0]);
		client(peer1, fd[1], argv[1]);
		kill(id, 9);
	}
	else if(id == 0)
	{
		close(fd[1]);
		server(peer2, fd[0]);
		exit(0);
	}
	return 0;
}