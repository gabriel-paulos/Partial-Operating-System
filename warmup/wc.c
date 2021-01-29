#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

struct hash{

  int occurrence;
  int key;
  char* word;
};

struct wc {

  int size;
  struct hash** items; 
};

//void copy(char* word, int start, int end);
struct hash* create_hash(char* word, int index);
unsigned long hash_key(char* str,int size);
void insert_word(struct wc* wc,char* hash);

struct wc* wc_init(char *word_array, long size)
{
	struct wc *wc;
	char str[130];

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

	int words = 0;
	//get the word count, so we can make a reasonably sized hash table to fit all words
        for(int i = 0; i < size; i++){
	  if((isspace(word_array[i]) != 0 && isspace(word_array[i+1]) == 0)||(i == (size-1))){ // || (isspace(word_array[i]) != 0 && word_array[i+1] == '\0')){
	   words++;
	  }
	 }
	//		printf("words %d\n",words);
	//int word_small= 1005;
	wc->items = (struct hash**)malloc(words*sizeof(struct hash*));
	wc->size = words;
	//int start_letter = 0;
	//int end_letter=0;
	
	int letter_in=0;
	//get the letters before any space characters
	for(int i =0; i < size; i++){
	  // printf("%d\n",i);
	  if(isspace(word_array[i]) == 0){
	    str[letter_in] = word_array[i];
	    letter_in++;
	    // printf("%c\n",word_array[i]);
	  }
	  
	  else {
	    // printf("%s\n",str);
	    if(strlen(str) > 0) insert_word(wc,str);
	    memset(str,'\0',130);
	    letter_in = 0;
	    
	  }
	}
	
	return wc;
}

/*
void copy(char* dest, char* src, int size){
  int count = 0;
  for(int i < 0; i < size; i++){
    dest[count]=src[i];
    count++;
  }
}
*/

struct hash* create_hash(char* word, int index){
  
  struct hash* item = (struct hash*)malloc(sizeof(struct hash));
  item->word = (char*)malloc((strlen(word)+1)*sizeof(char*));

  //don't need linked list since we only need to store its occurrence
  //linked list would take more space and longer to execute
  strcpy(item->word,word);
  item->word[strlen(word)] = '\0';
  item->occurrence = 1;
  
  return item;
}

//got it from stackoverflow 
unsigned long hash_key(char* str, int size){
  unsigned long hash = 5381;
  int c = 0;
    
  while(c < strlen(str)){
    hash = ((hash << 5) + hash) + str[c]; /* hash * 33 + c */
    c++;
  }  
  return (hash % size);
}

void insert_word(struct wc* wc,char* hash){
  int index = hash_key(hash,wc->size);

  //if word is brand new goes into new element of table
  if(wc->items[index] == NULL){
    /* 
    wc->items[index]->word= word;
    wc->items[index]->occurrence+=1;
    wc->items[index]->key= index;
    */
    struct hash* new_word =  create_hash(hash, index);
    wc->items[index] = new_word;
  }

  //if word exists already just increase count of word
  else if(strcmp(wc->items[index]->word,hash) == 0){
    wc->items[index]->occurrence+= 1;
  }

  //if space is taken by diff word find another place
  else if(strcmp(wc->items[index]->word,hash) != 0){
    while(index < wc->size){
      if(wc->items[index] == NULL){
	/*
	wc->items[index]->word = word;
	wc->items[index]->occurrence+=1;
	wc->items[index]->key = index;
	*/
	struct hash* new_word =  create_hash(hash, index);
	wc->items[index] = new_word;
	break;
      }

      else if(strcmp(wc->items[index]->word,hash) == 0){
	wc->items[index]->occurrence+= 1;
	break;
      }
      index++;
    }
  } 
}

void wc_output(struct wc *wc)
{
  for(int i = 0; i < wc->size; i++){
    if(wc->items[i] != NULL) printf("%s:%d\n",wc->items[i]->word,wc->items[i]->occurrence);

  }
}

void wc_destroy(struct wc *wc)
{
  //make sure every element is deleted in table, then delete the table
  for(int i = 0; i < wc->size; i++){
    if(wc->items[i] != NULL){
      free(wc->items[i]->word);
      free(wc->items[i]);
    }
  }
	free(wc);
}

