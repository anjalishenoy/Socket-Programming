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


char * GetNextFile(DIR * fd)
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
	//sprintf(buf+strlen(buf),"%s    ",files[i-1]->d_name);
	}
//            strcat(buf,"\n"); 
	return 0;
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


int client(int portnum, int fd1, char *IP, int type)
{
	int n = 0, serverfd = 0, command, size, fr_block_sz, num_responses,i;
	char *srecvBuff, *receiveBuffer;
	char clientInput[1025];		//Buffer for server and client

	struct sockaddr_in serverAddress;
	char line[1000];
	struct Operation cFileDownload;
	struct Operation cFileUpload;
	struct stat vstat;


	serverAddress.sin_family = AF_INET;		//IPv4
	serverAddress.sin_port = htons(portnum);		//Ntwork ordering
	serverAddress.sin_addr.s_addr = inet_addr(IP);		//conversion to binary of an IP

	serverfd = socket(AF_INET, SOCK_STREAM, 0);			
	//serverfd= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);	for UDP
	if(serverfd < 0)
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

		/*if(strcmp(clientInput, "FileUploadDeny") == 0)
		{
			printf("REJECTING\n");
			//Write to file descriptor
			write(fd1, "FileUploadDeny",(strlen("FileUploadDeny") + 1));
		}

		if(strcmp(clientInput, "FileUploadAllow") == 0)
		{
			printf("ALLOWING\n");
			write(fd1, "FileUploadAllow",(strlen("FileUploadAllow") + 1));
		}*/

		if(strcmp(clientInput, "FileDownload") == 0)
		{
			cFileDownload.command = FileDownload;
			scanf("%s", cFileDownload.fileName);
			printf("Dowloading file %s ...\n", cFileDownload.fileName);
			command = FileDownload;

			//Send command to server
			n = write(serverfd, &command, sizeof(int));
			if(n == -1)
				printf("Failure in sending command %s. Retry!\n", clientInput);

			//Send cFileDownload
			n=write(serverfd, &cFileDownload, sizeof(cFileDownload));
			if(n == -1)
				printf("Failed to send cFileDownload\n");

			printf("....Receiving file from server.... \n");

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
			/*else
				printf("created file %s\n", temp);*/
			
			fr_block_sz = 0; size = 0;

			// receive the file size
			fr_block_sz =recv(serverfd, &size, sizeof(int), 0);
			if(fr_block_sz!= sizeof(int))
			{
				printf("Error reading size of file %s\n", temp);
				return 0;
			}

			int receivedSize = 0;
			//Buffer to receive data from server
			receiveBuffer =(char *) malloc(size * sizeof(char));

		    // save original pointer to calculate MD5
			while((fr_block_sz = recv(serverfd, receiveBuffer, LENGTH, 0)) > 0)
			{
				receiveBuffer[fr_block_sz] = 0;
				int write_sz =fwrite(receiveBuffer, sizeof(char), fr_block_sz, f);	//Write to fileptr f
				receiveBuffer += fr_block_sz;
				if(write_sz == -1)
				{
					printf("Error writing to file %s\n", temp);
					return 0;
				}

				receivedSize += fr_block_sz;
				printf("Amount of size received: %d\n", receivedSize);
				if(receivedSize >= size)
					break;
			}
			printf("Done receiving file!\n");
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
				printf("Failed to send command %s\n", clientInput);

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

			if(write(serverfd, &cFileUpload, sizeof(cFileUpload)) == -1)
				printf("Failed to send  cFileUpload\n");

			size = vstat.st_size;

			if(send(serverfd, &size, sizeof(int), 0) < 0)
			{
				printf("send error\n");
				return 0;
			}

		    //waiting for accept or deny:
			char result[100], *readbuf;
			if((n = read(serverfd, &result, sizeof(result))) <= 0)
				printf("Error reading result\n");
			
			if(strcmp(result, "FileUploadDeny") == 0)
			{
				printf("Upload denied.\n");
				fclose(f);
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
			struct hashFile cFileHash;
			command = FileHash;

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
				printf("Failed to send cFileHash\n");
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
					printf("Error reading cFileHash_response of file %s\n", cFileHash.fileName);
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
			char t1[200], t2[200];
			time_t timt1, timt2;
			char regex[100], fname[1000];
			if((n = write(serverfd, &command, sizeof(int))) == -1)
				printf("Failed to send command %s\n", clientInput);
			else
				printf("Command sent: %d %s\n", n, clientInput);
			scanf("%s", option);
			puts(option);
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



int main()
{
	//CMD c;

	return 0;
}