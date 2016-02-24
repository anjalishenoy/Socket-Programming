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
#define BUF_SIZE 256
#define MD5LENGTH 16
#define LOCAL_PORT 0
#define FALSE 0
#define TRUE 1
// FileDownload=0, FileUpload=1, FileHash=2, IndexGet=3
typedef enum
{	FileDownload, FileUpload, FileHash,	IndexGet } CMD;

//To hold details of files being shown
struct sIndexGet
{
	struct stat fileDetails;		//from stat()
	char fileName[1000];
};

//To send/receive to/from server/client for upload/download
struct Operation
{
	CMD command;
	char fileName[1000];
};




struct HashFile
{
	CMD command;
	char type[1000];
	char fileName[100];
};

struct FileHash_response
{
	char fileName[128];
	MD5_CTX md5Context;
	char time_modified[128];
};

struct FileHash_response FileHash_response;
struct HashFile sFileHash;
int server(int portNo, int fdUpload); // Prototype



int GetNoOfFiles()
{


		//int num_files = 0;

	/*	struct dirent *result;
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
	*/
	  int num_files = 0;
	  FILE *fp;
	  char path[1035];

	  /* Open the command for reading. */
	  fp = popen("ls -l ./shared/ | grep  ^- | wc -l", "r");
	  if (fp == NULL) {
	    printf("Failed to run command\n" );
	    exit(1);
	  }

	  /* Read the output a line at a time - output it. */
	  while (fgets(path, sizeof(path)-1, fp) != NULL) {
	    num_files = atoi(path);
	  }

	  /* close */
	  pclose(fp);

		return num_files;
}


// get md5 of buffer
void Getmd5(char *readbuf, int size)
{
	MD5Init(&FileHash_response.md5Context);
	MD5Update(&FileHash_response.md5Context,(unsigned char *) readbuf, size);
	MD5Final(&FileHash_response.md5Context);
}


char *GetNextFile(DIR * fd)
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

int file_select(const struct direct *entry)
{
	if((strcmp(entry->d_name, ".") == 0)||(strcmp(entry->d_name, "..") == 0))
		return(FALSE);
	else
		return(TRUE);
}

int fileget(char *buf, int Fclientfd)
{
	int count, i;
	struct direct **files;
    //int file_select();
	struct stat fileDetails;
	struct sIndexGet fstat;
	count = scandir("./shared", &files, file_select, alphasort); // scans the directory for matching entries

	if(count <= 0)
	{
		strcat(buf, "No files in this directory\n");
		return 0;
	}
	if(send(Fclientfd, &count, sizeof(int), 0) < 0)
	{
		printf("Couldn't send!\n");
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
		if(stat(dir, &(fileDetails)) == -1)
		{
			printf("fstat error\n");
			return 0;
		}
		fstat.fileDetails = fileDetails;
		strcpy(fstat.fileName, files[i]->d_name);


		if(write(Fclientfd, &fstat, sizeof(fstat)) == -1)
			printf("Failed to send fileDetails\n");
		else
			printf("Sent fileDetails\n");
			//sprintf(buf+strlen(buf),"%s",files[i-1]->d_name);
	}
	//strcat(buf,"\n"); 
	return 0;
}

// get file hash information for current FileHash
void getFileHash()
{

	char temp[100];
	struct stat vstat;
	char *fname = temp;
	int block;
	char *readbuf;

    // copy filename into the response
	strcpy(FileHash_response.fileName, sFileHash.fileName);

	strcpy(temp, "./shared/");
	strcat(temp, sFileHash.fileName);

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
	ctime_r(&vstat.st_mtime, FileHash_response.time_modified);


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
	printf("FileHash_response of file %s \n", FileHash_response.fileName);
	printf("    - Last Modified @ %s", FileHash_response.time_modified);
	printf
	("    - MD5 %01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x%01x\n",
		FileHash_response.md5Context.digest[0],
		FileHash_response.md5Context.digest[1],
		FileHash_response.md5Context.digest[2],
		FileHash_response.md5Context.digest[3],
		FileHash_response.md5Context.digest[4],
		FileHash_response.md5Context.digest[5],
		FileHash_response.md5Context.digest[6],
		FileHash_response.md5Context.digest[7],
		FileHash_response.md5Context.digest[8],
		FileHash_response.md5Context.digest[9],
		FileHash_response.md5Context.digest[10],
		FileHash_response.md5Context.digest[11],
		FileHash_response.md5Context.digest[12],
		FileHash_response.md5Context.digest[13],
		FileHash_response.md5Context.digest[14],
		FileHash_response.md5Context.digest[16]);

    // return
	return;
}
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
	int n = 0, serverfd = 0, command, size, fr_block_sz, num_responses,i, type;
	char *srecvBuff, *receiveBuffer;
	char clientInput[1025];		//Buffer for server and client
	char t1[200], t2[200];
	time_t timt1, timt2;
	struct hostent *hp;
	socklen_t len;

	struct sockaddr_in serverAddress, clientAddress;
	char line[1000];
	struct Operation cFileDownload;
	struct Operation cFileUpload;
	struct stat vstat;


	//FOR TCP
	bzero((char *) &serverAddress, sizeof(serverAddress));


	serverAddress.sin_family = AF_INET;		//IPv4
	serverAddress.sin_port = htons(portnum);		//Ntwork ordering
	serverAddress.sin_addr.s_addr = inet_addr(IP);		//conversion to binary of an IP

	//FOR UDP
	bzero((char *) &clientAddress, sizeof(clientAddress));
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddress.sin_port = htons(LOCAL_PORT);

	serverfd = socket(AF_INET, SOCK_STREAM, 0);	


	//serverfd= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);	for UDP
	if(serverfd < 0)
	{
		printf("CLIENT: Error creating socket \n");
		return 1;
	}
	else
		printf("CLIENT: Created socket in CLIENT\n");

	/*if(type==UDP)
	{
		if (bind(sockfd, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0)//check UDP socket is bind correctly
   		{
        perror("Cannot bind ");
        close(sockfd);
        return 1;
 	   }

	}*/
	int check=0;

	while((connect(serverfd,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) && check < 50000)
	{
			check++;
			sleep(1);
	}
	

	printf("CLIENT: Connection to server on port %d successful\n", portnum);

	while(1)
	{
		//printf("CLIENT: Enter command: ");
		scanf("%s", clientInput);	//scanning for inputs
		printf("CLIENT: Received command : %s\n", clientInput);

		if(strcmp(clientInput, "Deny") == 0)
		{
			printf("REJECTING\n");
			//Write to file descriptor
			write(fd1, "Deny",(strlen("Deny") + 1));
		}

		if(strcmp(clientInput, "FileUploadAllow") == 0)
		{
			printf("ALLOWING\n");
			write(fd1, "Allow",(strlen("Allow") + 1));
		}

		if(strcmp(clientInput, "FileDownload") == 0)
		{
			printf("Input type of connection:\n");
			scanf("%d", &type);
			if(type==UDP)
				serverfd = socket(AF_INET, SOCK_DGRAM, 0);
			int sockfd=serverfd;		//socfd used if UDP
			if(serverfd < 0)
			{
				printf("CLIENT(UDP): Error creating socket \n");
				return 1;
			}
			else
				printf("CLIENT(UDP): Created socket in CLIENT\n");
			if(type==UDP)
			{
				if (bind(sockfd, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0)//check UDP socket is bind correctly
   				{
        			perror("Cannot bind ");
        			close(sockfd);
        			return 1;
 	   			}
 	   		}
			//--------------------------------------------------------------------
			cFileDownload.command = FileDownload;
			//printf("CLIENT: Enter Filename for download:");
			scanf("%s", cFileDownload.fileName);
			printf("Downloading file %s ...\n", cFileDownload.fileName);

			command = FileDownload;

			//Send command to server
			if(type==TCP)
				n = write(serverfd, &command, sizeof(int));
			else
			{
				n=0;
				sendto(sockfd, &command, sizeof(int), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
			}
			if(n == -1)
				printf("CLIENT: Failure in sending command %s. Retry!\n", clientInput);
			else
				printf("CLIENT: Command %s sent to SERVER \n", clientInput);

			//Send cFileDownload
			if(type==TCP)
				n=write(serverfd, &cFileDownload, sizeof(cFileDownload));
			else
			{
				n=0;
				sendto(sockfd, &cFileDownload, sizeof(cFileDownload), 0,(struct sockaddr *)&serverAddress, sizeof(serverAddress));
			}
			if(n == -1)
				printf("CLIENT: Failed to send cFileDownload Object\n");
			else
				printf("CLIENT: Sent cFileDownload Object\n");

			printf("....Receiving file from server.... \n");

			char temp[100];
			strcpy(temp, "./shared/");
			strcat(temp, cFileDownload.fileName);
			FILE *f = fopen(temp, "w+");

			//Creating file to copy downloaded contents
			if(f == NULL)
			{
				printf("CLIENT: Error Creating File %s\n", temp);
				return 1;
			}
			else
				printf("CLIENT: Created file %s\n", temp);
			
			fr_block_sz = 0; size = 0;

			// receive the file size
			if(type==TCP)
				fr_block_sz =recv(serverfd, &size, sizeof(int), 0);
			else
				fr_block_sz =recvfrom(serverfd, &size, sizeof(int), 0, (struct sockaddr *)&clientAddress, &len);

			if(fr_block_sz!= sizeof(int))
			{
				printf("CLIENT: Error reading size of file %s\n", temp);
				return 0;
			}
			else
				printf("CLIENT: Size of file:%d\n",size);

			int receivedSize = 0;
			//Buffer to receive data from server
			receiveBuffer =(char *) malloc(size * sizeof(char));

		    // save original pointer to calculate MD5
		    if(type==TCP)
				fr_block_sz = recv(serverfd, receiveBuffer, LENGTH, 0);
			else
				fr_block_sz =recvfrom(serverfd, receiveBuffer, LENGTH, 0, (struct sockaddr *)&clientAddress, &len);

			while(fr_block_sz  > 0)
			{
				receiveBuffer[fr_block_sz] = 0;
				int write_sz =fwrite(receiveBuffer, sizeof(char), fr_block_sz, f);	//Write to fileptr f
				receiveBuffer += fr_block_sz;
				if(write_sz == -1)
				{
					printf("CLIENT: Error writing to file %s\n", temp);
					return 0;
				}

				receivedSize += fr_block_sz;
				printf("CLIENT: Size of file received ==> %d\n", receivedSize);
				if(receivedSize >= size)
					break;
				if(type==TCP)
					fr_block_sz = recv(serverfd, receiveBuffer, LENGTH, 0);
				else
					fr_block_sz =recvfrom(serverfd, receiveBuffer, LENGTH, 0, (struct sockaddr *)&clientAddress, &len);
			}
			printf("CLIENT: Done Receiving File!\n");
			//close the file
			fclose(f);
			n = 0;
		}

		else if(strcmp(clientInput, "FileUpload") == 0)
		{

			cFileUpload.command = FileUpload;
			scanf("%s", cFileUpload.fileName);
			printf(".....Uploading file %s ....\n", cFileUpload.fileName);

			command = FileUpload;

		    //sending command name
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("CLIENT: Failed to send command %s\n", clientInput);

		    //opening the file
			char fileName[100];
			strcpy(fileName, "./shared/");
			strcat(fileName, cFileUpload.fileName);

			char *fname = fileName;
			FILE *f = fopen(fname, "r");
			if(f == NULL)
			{
				printf("Unable to open file.\n");
				return 0;
			}

		    // get size of file
			if(stat(fileName, &vstat) == -1)
			{
				printf("vstat error\n");
				return 0;
			}

			// send details of file to be uploaded to server
			if(write(serverfd, &cFileUpload, sizeof(cFileUpload)) == -1)
				printf("Failed to send FileUpload Object\n");
			else
				printf("CLIENT: sent FileUpload Object\n");

			size = vstat.st_size;

			//send size of file to be uploaded
			if(send(serverfd, &size, sizeof(int), 0) < 0)
			{
				printf("send error\n");
				return 0;
			}

		    //waiting for accept or deny:
			char result[100], *readbuf;
			if((n = read(serverfd, &result, sizeof(result))) <= 0)
				printf("Error reading result\n");

			if(strcmp(result, "Deny") == 0)
			{
				printf("Upload denied.\n");
				fclose(f);
				continue;
			}
			else
				printf("Upload accepted\n");

			// commence sending process
			readbuf =(char *) malloc(size * sizeof(char));
			if(readbuf == NULL)
			{
				printf("error, No memory\n");
				exit(0);
			}

		    //sending the file
		    int block;
			while((block = fread(readbuf, sizeof(char), size, f)) > 0)
			{
				if(send(serverfd, readbuf, block, 0) < 0)
				{
					printf("send error\n");
					return 0;
				}
			}
			n = 0;
		}


		else if(strcmp(clientInput, "FileHash") == 0)
		{

			struct FileHash_response cFileHash_response;
			struct HashFile cFileHash;
			command = FileHash;

		    // set the FileHash command
			cFileHash.command = FileHash;

		    // get the type
			scanf("%s", cFileHash.type);
			printf("FileHash Type: %s\n", cFileHash.type);

		    // get the fileName only for Verify option
			if(strcmp(cFileHash.type, "Verify") == 0)
			{
				printf("Enter filename : \n");
				scanf("%s", cFileHash.fileName);
				printf("Hashing file %s ...\n", cFileHash.fileName);
			}
		    //sending command name
		    n = write(serverfd, &command, sizeof(int));
			if(n == -1)
				printf("CLIENT: Failed to send command %s\n", clientInput);
			else
				printf("CLIENT: Command sent: %s\n", clientInput);

		    // send cFileHash with the the information
			if(write(serverfd, &cFileHash, sizeof(cFileHash)) == -1)
				printf("CLIENT: Failed to send cFileHash\n");
			else
				printf("CLIENT: Sent cFileHash %s %s \n", cFileHash.type,cFileHash.fileName);
		    
		    // receive number of file hash resposes to expect
			int num_responses = 0;
		    //n=recv(serverfd, &num_responses, sizeof(num_responses),0);

		    //n==1 if VERIFY, and N if checkAll (N==number of files in directory)
		    if((n = recv(serverfd, &num_responses, sizeof(num_responses),0) != sizeof(num_responses)))
			{
				printf("CLIENT: Error reading number of responses of file\n");
				return 0;
			}
			else
				printf("CLIENT: Expecting %d responses\n", num_responses);
			/*
			if( n!= sizeof(num_responses))
			{
				printf("Error reading number of responses of file\n");
				return 0;
			}
			else
				printf("Expecting %d responses\n", num_responses);
			*/

		    // print each file hash response
		    MD5_CTX hash;
			for(i = 0; i < num_responses; i++)
			{
				n=recv(serverfd, &hash, sizeof(hash),0);
				if(n!=sizeof(hash))
				{
					printf("Error reading hash sent from server\n");
					return 0;
				}

				n =recv(serverfd, &cFileHash_response, sizeof(cFileHash_response),0);
				if(n!= sizeof(cFileHash_response))
				{
					printf("Error reading File Hash Response of file %s\n", cFileHash.fileName);
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
		else if(strcmp(clientInput, "IndexGet") == 0)
		{
			char option[1000];
			char *filelist;
			command = IndexGet;
			struct sIndexGet fstat;
			int lenfiles = 0;
			char buff[1000];

			char regex[100], fname[1000];
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);
			scanf("%s", option);

			if(strcmp(option, "ShortList") == 0)
			{
				scanf("%s %s", t1, t2);
				
				timt1 = gettime(t1);
				timt2 = gettime(t2);
			}

			if(strcmp(option, "RegEx") == 0)
			{
				system("touch res");
				scanf("%s", regex);		//inputting the regex

			}
			FILE *fp = fopen("./res", "a");

			if((fr_block_sz =recv(serverfd, &lenfiles, sizeof(int), 0)) != sizeof(int))
			{
				printf("Error reading number of file\n");
				return 0;
			}
			else
				printf("Recieved number of files %d\n", lenfiles);

			for(i = 0; i < lenfiles; i++)
			{
				if((n =	read(serverfd,(void *) &fstat, sizeof(fstat))) != sizeof(fstat))
					printf("Error reciving vstat\n");

				if(strcmp(option, "LongList") == 0)
				{
					printf("Filename: ");
					puts(fstat.fileName);
					vstat = fstat.fileDetails;
					printf("Size: %d\t",(int) vstat.st_size);
					ctime_r(&vstat.st_mtime, buff);
					printf("Time Modified: %s\n", buff);
				}

				else if(strcmp(option, "ShortList") == 0)
				{
					vstat = fstat.fileDetails;
					if(difftime(vstat.st_mtime, timt1) >= 0	&& difftime(timt2, vstat.st_mtime) >= 0)
					{
						printf("Filename: ");
						puts(fstat.fileName);
						vstat = fstat.fileDetails;
						printf("Size: %d\t",(int) vstat.st_size);
						ctime_r(&vstat.st_mtime, buff);
						printf("Time Modified: %s\n", buff);
					}
				}

				else if(strcmp(option, "RegEx") == 0)
				{

					fwrite(fstat.fileName, 1, strlen(fstat.fileName), fp);
					fwrite("\n", 1, 1, fp);
				}
			}
			fclose(fp);
			if(strcmp(option, "RegEx") == 0)
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


int server ( int portNo, int fdUpload)
{
	//initialise a TCP socketstructure
	struct sockaddr_in s_addr, c_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_port = htons(portNo);

	// created socket
	int fdListen = 0;
	fdListen = socket(AF_INET, SOCK_STREAM, 0);
	printf("SERVER: Created Socket for SERVER\n");

	//bind
	bind( fdListen ,(struct sockaddr *) &s_addr, sizeof(s_addr));

	//listen
	if(listen(fdListen, 10) == -1)
	{
		printf("Failed to listen\n");
		return -1;
	}
	else
		printf("SERVER: Listening on port %d\n", portNo);

/*
	if(portNo == -1)
	{
		printf("Error listening to port!\n");
		return -1;
	}
*/
	int fdClient=0;	
	if( (fdClient = accept(fdListen,(struct sockaddr *) NULL, NULL) ) == -1)	// accept awaiting request
		printf("Couldn't accept client request!\n");
	else
		printf("SERVER: Accepted CLIENT request\n");	//not happening

	int c = 0, cmd, d; // file decriptors for command, download file
	struct Operation downloadFile, uploadFile;
	char list[1000];
	while(1)
	{
		 // Take input for command to be performed
		if((c = read(fdClient, &cmd, sizeof(int))) > 0)
		{
			printf("SERVER: Received command %s\n", cmd);
			c = 0;
		}
		
		if( (CMD) cmd == IndexGet)
			{
				fileget(list, fdClient);
			}

		else if( (CMD) cmd ==  FileDownload)
			/*  begin */
			{
				printf("SERVER: Download File!\n");

				// read filename of command to be downloaded
				if((d =read(fdClient,(void *) &downloadFile,sizeof(downloadFile))) != sizeof(downloadFile))
    				printf("Error reading filename\n");
    			else
    				printf("SERVER: Read file name for download as: %s\n", downloadFile.fileName);
    		
	    		char dest[100];
	    		strcpy(dest, "./shared/");
	    		strcat(dest, downloadFile.fileName);

	    		//opening the file to copy the contents of file to be downloaded
	    		FILE *fs;
	    		if( (fs = fopen(dest, "r") ) == NULL )
	    		{
	    			printf("Unable to open file.\n");
	    			return 0;
	    		}
	    		else
	    			printf("SERVER: Opened file %s for download\n", downloadFile.fileName);

	    		struct stat filestat;
		    	
		    	// get size of file
	    		if(stat(dest, &filestat) == -1)
	    		{
	    			printf("vstat error\n");
	    			return 0;
	    		}
		    	
		    	// send file size 
	    		int block;
	    		int size;
	    		char *readBuffer = (char *) malloc(size * sizeof(char) );
	    		size = filestat.st_size;
	    		if( readBuffer == NULL)
	    		{
	    			printf("No memory\n");
	    			exit(0);
	    		}


	    		if(send(fdClient, &size, sizeof(int), 0) < 0) // initiate transmission of a message from the specified socket to its peer
	    		{
	    			printf("send error\n");
	    			return 0;
	    		}

	    		else
	    			printf("SERVER: Sending file of size %d\n", size);

	    	// send file block by block
	    		while((block = fread(readBuffer, sizeof(char), size, fs)) > 0)
	    		{
	    			if(send(fdClient, readBuffer, block, 0) < 0)
	    			{
	    				printf("send error\n");
	    				return 0;
	    			}
	    			printf("SERVER: File Sent\n");
	    		}
	    		fclose(fs);
			}
			/*end case */
			
			else if( (CMD) cmd == FileUpload)
			/* begin */
			{
				printf("Upload a file!\n");
				
				//read filename
				int readName  = 0;
				if( (readName = read(fdClient,(void *) &uploadFile,sizeof(uploadFile))) != sizeof(uploadFile))
    				printf("Couldnt read filename!\n");

    			//read filesize
    			int recSize = 0;
    			int size = 0;
    			if( (recSize = recv(fdClient, &size, sizeof(int), 0)) != sizeof(int)  )
    			{
    				printf("Couldnt read filesize!\n"); 
    				return 0;
    			}

	    		char result[100];
	    		int inputfd = 0,w = 0;
	    		printf("File Upload requestedt, Allow or Deny? \n");
	    		read(inputfd, result, sizeof("Allow"));

	    		if(strcmp(result, "Deny") == 0)
	    		{
	    			if((w = write(fdClient, &result, sizeof(result))) == -1)
	    				printf("Failed to send result %s\n", result);
	    			else
	    				printf("Rejection to upload sent: %d %s\n", w, result);
	    			continue;
	    		}
	    		else
	    		{
	    			printf("Accepted file\n");
	    			write(fdClient, &result, sizeof(result));
	    		}

	    		char temp[100];
	    		strcpy(temp, "./shared/");
	    		strcat(temp, uploadFile.fileName);
	    		
	    		// Open the file to be uploaded
	    		FILE *filePointer;
	    		if( ( filePointer = fopen(temp, "w+") ) == NULL)
	    		{
	    			printf("Error creating file %s\n", temp);
	    			return 0;
	    		}
	 
	    		int uploadBlock = 0, sizeOfBlockRecvd;
	    		char *recvBuffPtr =(char *) malloc(size * sizeof(char));
	    		while(1)
	    		{
	    			if ( (uploadBlock = recv(fdClient, recvBuffPtr, LENGTH, 0)) <= 0 )
	    				break;

	    			recvBuffPtr[uploadBlock] = 0;
	    			int writeBlock = fwrite(recvBuffPtr, sizeof(char), uploadBlock, filePointer);
	    			recvBuffPtr += uploadBlock;
	    			if(writeBlock != -1)
	    			{
	    				printf("Amount Received ==> %d\n", sizeOfBlockRecvd);
	    			}
	    			else
	    			{
	    				printf("File write error!\n");
	    				return 0;
	    			}

	    			sizeOfBlockRecvd += uploadBlock;
	    			if(sizeOfBlockRecvd >= size)		// size - total size of recieved file to be uploaded that was saved earlier
	    				break;

	    		}
	    		printf("SERVER: Done Upload!\n");
	    		fclose(filePointer);
	    	
	    	}
	    		/* end case */

	    	else if( (CMD) cmd ==  FileHash)
	    	/* begin */
	    	{
	    		MD5_CTX md5Context;
	    		int num_responses;
	    		char temp[1000];
	    		printf("SERVER: File Hash!\n");
	    		
	    		int readFileHashPtr = 0;
	    		readFileHashPtr = read(fdClient,(void *) &sFileHash,sizeof(sFileHash));
	    		
	    		if(readFileHashPtr != sizeof(sFileHash))
	    			printf("SERVER: Error reading file hash!\n");
	    		else
	    			printf("SERVER: Commencing File Hash %s, sFileHash.type\n");

	    		if(strcmp( sFileHash.type, "Verify" ) == 0)
	    		{
	    			num_responses = 1;

	    			if(send(fdClient, &num_responses, sizeof(int), 0) < 0)
	    			{
	    				printf("SERVER: Couldn't send number of responses!\n");
	    				return 0;
	    			}
	    			else
	    				printf("SERVER: The number of responses: %d\n", num_responses);

	    			getFileHash();

	    			if(send(fdClient, &FileHash_response, sizeof(FileHash_response),0) < 0)
	    			{
	    				printf("SERVER: Couldn't send number of file hash responses!\n");
	    				return 0;
	    			}
	    			else
	    				printf("SERVER: Sent FileHash_response %s\n",FileHash_response.fileName);
	    		}
	    		else if(strcmp(sFileHash.type, "CheckAll") == 0)
	    		{
	    			DIR *fd;
	    			int i;
	    			printf("Check all \n");
	    			strcpy(temp, "./shared/");
	    			fd = opendir(temp);
	    			if(NULL == fd)
	    			{
	    				printf("Error opening directory %s\n", temp);
	    				return 0;
	    			}

	    			num_responses = GetNoOfFiles();
	    			printf("Found %d files in shared folder\n", num_responses);
	    			if(send(fdClient, &num_responses, sizeof(num_responses), 0) <  0)
	    			{
	    				printf("Couldn't send! \n");
	    				return 0;
	    			}
	    			else
	    				printf("sent number of responses %d\n", num_responses);

	    			fd = opendir(temp);
	    			for(i = 0; i < num_responses; i++)
	    			{
	    				strcpy(sFileHash.fileName, GetNextFile(fd));
	    				getFileHash();

	    				if(send(fdClient, &FileHash_response,sizeof(FileHash_response), 0) < 0)
	    				{
	    					printf("Couldn't send!\n");
	    					return 0;
	    				}
	    				else
	    					printf("sent FileHash_response %s\n",FileHash_response.fileName);
	    			}
	    			closedir(fd);
	    		}
	    		else
	    		{
	    			//printf("Received invalid File Hash Command %s\n",FileHash.type);
	    			return 0;
	    		}
	    		/* end case */
	    	}
	    	else
	    	{
	    		printf("Wrong Command Entered!\n");
	    		continue;
	    	}

	    		
		}
}


/*int main(int argc, char *argv[])
{
<<<<<<< HEAD
	int count=0, type;
	char inputBuff[4][100];
	if(argc<5)
		printf("\nInput <IP> <Port of Remote Machine> <Port on your Machine> <TCP/UDP> \n");

	if(strcmp(argv[4], "TCP")==0)
		type=TCP;
	else if(strcmp(argv[4], "UDP")==0)
		type=UDP;
=======
	if(argc < 4)
	{
		printf("Usage ./peer <IP> <Port of Remote Machine> <Port of Your Machine>\n");
		return -1;
	}
>>>>>>> 103a04f71d06df831b09a0d043465359cd4e80e7
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
<<<<<<< HEAD
		client(peer1, fd[1], argv[0], type);
=======
		client(peer1, fd[1], argv[1]);
>>>>>>> 103a04f71d06df831b09a0d043465359cd4e80e7
		kill(id, 9);
	}
	else if(id == 0)
	{
		close(fd[1]);
		server(peer2, fd[0]);
		exit(0);
	}
	return 0;
}*/

int main(int argc, char *argv[])
{
	int type;
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
