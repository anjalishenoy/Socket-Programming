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



int main()
{
	//CMD c;

	return 0;
}