#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "errno.h"
#include <dirent.h>
#include <string.h>


/* make sure to use syserror() when a system call fails. see common.h */

void usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

void copy_file(char* src, char* dest){

  //open the source file
  int fd;
  fd = open(src,O_RDONLY);

  if(fd < 0) syserror(open,src);
  
  else{

    //open, and if the dest file doesnt exist create one with user permissions enabled
    int fs= open(dest, O_WRONLY| O_CREAT, S_IRWXU);
    
    if(fs < 0) syserror(creat,dest); 
    
    else{
      // int fs = open(dest,O_WRONLY,S_IRWXG);
      char buf[4096];
      int bytes;

      //copy all bytes from src to dest until we reach EOF
      while((bytes = read(fd,buf,4096)) != 0){
	int noel = write(fs,buf,bytes);
	//printf("%d buff wrote\n",noel);
	if(noel < 0){
	  syserror(write,dest);
	  break;
	}
       
	if(bytes < 0){
	  syserror(read,src);
          break;
	}
      }
      //close the files to ensure no memory leaks or undefined behaviour
      close(fd);
      close(fs);  
    }
  }
}
                 
void copy_dir(char* source, char* dest){

  DIR* src_stream = opendir(source);
  struct dirent* cool;
                                                                                                                                                                                                                       
  if(src_stream == NULL){
    //printf("ERROR OMG!!\n");
    syserror(opendir,source);
  }
  else{
    //make new dir, this will be dest dir
    int new_dir = mkdir(dest,S_IRWXU | S_IRWXO);

    if(new_dir == -1){
      closedir(src_stream);
      syserror(mkdir,dest);
    }
    
    else{
      int stats;
      //need to read the file name(256), the path name(256), "/" and the null character: assume this is 514 char
      char src_name[514];
      char dest_name[514];
      struct stat* statbuf = (struct stat*)malloc(sizeof(struct stat));
      while((cool = readdir(src_stream)) != NULL){
	
	int change_mod;
	//clear the prev file or dir name for both src and dest
	memset(src_name,'\0',514);
	memset(dest_name,'\0',514);

	//create new path names
	strcpy(src_name,source);
	strcat(src_name, "/");
	strcat(src_name, cool->d_name);
	//strcat(src_name,'\0');
	strcpy(dest_name,dest);
        strcat(dest_name, "/");
        strcat(dest_name, cool->d_name);
      	//strcat(dest_name,'\0');

	//dont want to copy the current directory again nor the previous directory of the src directory
	if(strcmp(cool->d_name,".") != 0 && strcmp(cool->d_name, "..") != 0){
	  stats = stat(src_name,statbuf);
	  if(stats == -1) syserror(stat,src_name);
	  
	  //	  int change_mod;
	  //if file is a reg file goes into copy_file
	  if(S_ISREG(statbuf->st_mode)){
	    copy_file(src_name,dest_name);
	    //if((change_mod = chmod(dest_name, statbuf->st_mode)) == -1) syserror(chmod,dest_name);
	  }
	  //if file is a dir we enter that dir to copy its files
	  else if(S_ISDIR(statbuf->st_mode)){
	    copy_dir(src_name,dest_name);
	    //if((change_mod = chmod(dest_name, statbuf->st_mode)) == -1) syserror(chmod,dest_name);
	  }
	  //change permissions once this is done
	  if((change_mod = chmod(dest_name, statbuf->st_mode)) == -1) syserror(chmod,dest_name);
	  
	}
	//if((change_mod = chmod(dest_name, statbuf->st_mode)) == -1) syserror(chmod,dest_name);
      }
      free(statbuf);
    }
  }

  closedir(src_stream);
}

  
int main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
	}
	
	else copy_dir(argv[1],argv[2]);

	return 0;
}
