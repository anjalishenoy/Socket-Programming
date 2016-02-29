#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	//printf(" %d\n ",num_files);
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
    printf("%d\n", atoi(path));
  }

  /* close */
  pclose(fp);

  return 0;
}