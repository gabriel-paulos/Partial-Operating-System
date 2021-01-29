#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <time.h>

struct thread{

  pthread_t id;
  // int status;             
  //  struct thread* next;   
};

//using hash table I made from lab 1

struct file_element{
  
  struct file_data* data;    
  //  struct file_element* next;
  char* key;                 
  int hash_num;              
  int using;                                                                                                                                        
};                                                                                                                                  
            
struct cache_element{                                 
  //struct file_element* file;
  int f_key;                           
  struct cache_element* next;
};

struct hash_table{
  
  int current_cache;                                                    
  int size;                                                             
  struct file_element** elements;                                       
  pthread_mutex_t lo;                                                   
  struct cache_element* recent_list; //or could make it into its own global variable and class
};                            

struct server {
        int nr_threads;
        int max_requests;
        int max_cache_size;
        int exiting;
        /* add any other parameters you need */
  pthread_t* threads;
  int* bufferq;
  pthread_mutex_t l;
  int in, out;
  pthread_cond_t full;
  pthread_cond_t empty;

  struct hash_table* table;                                      
};

void* server_response(struct server* sv);

struct file_element* cache_lookup(struct server* sv, struct file_data* data);
struct file_element* cache_insert(struct server* sv, struct file_data* data);
struct file_element* table_insert(struct server* sv, struct file_data* data);
void cache_evict(struct server* sv, int file_size);
unsigned long hash_key(struct server* sv, char* str);
void cache_fix(struct server* sv, int loc); 
void enqueue(struct server* sv, int loc);
void dequeue(struct server* sv, int loc);

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
        struct file_data *data;

        data = Malloc(sizeof(struct file_data));
        data->file_name = NULL;
        data->file_buf = NULL;
        data->file_size = 0;
        return data;
}


/* free all file data */
static void file_data_free(struct file_data *data)
{
        free(data->file_name);
        free(data->file_buf);
        free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
        int ret;
        struct request *rq;
        struct file_data *data;

        data = file_data_init();

        /* fill data->file_name with name of the file being requested */
        rq = request_init(connfd, data);
        if (!rq) {
                file_data_free(data);
                return;
        }
        /* read file,                                                                                                                                                                                      
         * fills data->file_buf with the file contents,                                                                                                                                                    
         * data->file_size with file size. */

        if(sv->max_cache_size > 0) {
                pthread_mutex_lock(&sv->table->lo);

		struct file_element* file = cache_lookup(sv,data);          

		//check if the file exists or not
		if(file != NULL) { 

		  request_set_data(rq, file->data);

		  //communicate to the server thread not to delete this element
		  file->using++;

		  //exists so we update the LRU
		  cache_fix(sv, file->hash_num);

		  pthread_mutex_unlock(&sv->table->lo);

		  request_sendfile(rq); 
			
		  pthread_mutex_lock(&sv->table->lo);

		  //we can delete it now
		  file->using--;

		  pthread_mutex_unlock(&sv->table->lo);

                        goto out;                                                                                                                                                        
                }
		else if(file == NULL) {
		  
		  pthread_mutex_unlock(&sv->table->lo);

		  ret = request_readfile(rq);

		  if (ret == 0){
		    goto out; 
		  }
			
		  pthread_mutex_lock(&sv->table->lo);

		  //insert the file because it doesn't exist
		  file = cache_insert(sv,data);	
		  request_set_data(rq, data);

		  //now that it exits update it to the front (if it isn't already)
		  if(file != NULL) {
		    file->using++;
		    cache_fix(sv, file->hash_num);
		  }
		  pthread_mutex_unlock(&sv->table->lo);

		  request_sendfile(rq);

		  pthread_mutex_lock(&sv->table->lo);
		  if(file != NULL)
		    file->using--;
		  pthread_mutex_unlock(&sv->table->lo);
		  goto out;
                }
        }
        else {                                                                                                                                                    
	  /* read file,                                                                                                                                                                              
	   * fills data->file_buf with the file contents,                                                                                                                                             
	   * data->file_size with file size. */
	  ret = request_readfile(rq);
	  if (ret == 0) { /* couldn't read file */
	    goto out;
	  }
	  /* send file to client */
	  request_sendfile(rq);
        }
 out:
        request_destroy(rq);                                                                                                                                                 
}

/* entry point functions */

struct server * server_init(int nr_threads, int max_requests, int max_cache_size){
        struct server *sv;
        pthread_attr_t* attr = NULL;

        //initialize the members
	
        sv = Malloc(sizeof(struct server));
        sv->nr_threads = nr_threads;
        sv->max_requests = max_requests;
        sv->max_cache_size = max_cache_size;
        sv->exiting = 0;

        if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {

          //initialize the threads array, the lock, cv, and the requests buffer 

          sv->bufferq = (int*)malloc(sizeof(int)*(max_requests+1));
          sv->threads =(pthread_t*)malloc(sizeof(pthread_t)*nr_threads);
          pthread_mutex_init(&sv->l, NULL);
          sv->in = 0;
          sv->out = 0;
          pthread_cond_init(&sv->full, NULL);
          pthread_cond_init(&sv->empty, NULL);

          //initalize the worker threads here
	  
          for(int i = 0; i < nr_threads; i++){
            int f = pthread_create(&sv->threads[i],attr,(void*)server_response,(void*)sv);
            assert(!f);
          }

          if(max_cache_size > 0){

	    sv->table = (struct hash_table*)Malloc(sizeof(struct hash_table));
	    sv->table->size = 2200000;
	    sv->table->elements = (struct file_element**)Malloc(sv->table->size * sizeof(struct file_element*));
	    sv->table->recent_list = (struct cache_element*)Malloc(sizeof(struct cache_element));

	    sv->table->current_cache = 0;
	    //sv->table->max_cache = max_cache_size;
	    sv->table->recent_list->next = NULL;
	    pthread_mutex_init(&sv->table->lo, NULL);
	    
	    for(int i = 0; i < sv->table->size; i++) {
		sv->table->elements[i] = NULL;
	    }	    
          }
	  /* Lab 4: create queue of max_request size when max_requests > 0 */
	  /* Lab 5: init server cache and limit its size to max_cache_size */
	  /* Lab 4: create worker threads when nr_threads > 0 */
        }
          return sv;
}

void* server_response(struct server *sv){

  //exit if server makes exiting == 1, minimize the worker threads from exiting                                                                                                                            
  while(1){

    if(sv->exiting == 1){
      pthread_exit(0);
     
    }

    //this is the "consumer"
    pthread_mutex_lock(&sv->l);
    //printf("In thread worker %d\n", (int) pthread_self());
    
    while(sv->in == sv->out){
      pthread_cond_wait(&sv->empty, &sv->l);
      if(sv->exiting == 1){
        pthread_mutex_unlock(&sv->l);
        pthread_exit(0);
        //printf("Exiting\n");                                                                                                                                                                             
      }
    }
    
    int elem = sv->bufferq[sv->out];
    sv->out = (sv->out+1)%(sv->max_requests+1);
    pthread_cond_signal(&sv->full);

    pthread_mutex_unlock(&sv->l);

    do_server_request(sv, elem);

  }

  pthread_exit(0);
}

void server_request(struct server *sv, int connfd){
        if (sv->nr_threads == 0) { /* no worker threads */
                do_server_request(sv, connfd);
        } else {
                /*  Save the relevant info in a buffer and have one of the                                                                                                                                 
                 *  worker threads do the work. */

          //this is the "producer"                                                                                                                                                                         

	  pthread_mutex_lock(&sv->l);
	  //printf("In here\n");                                                                                                                                                                           

          while(((sv->in - sv->out + sv->max_requests+1)%(sv->max_requests+1) == (sv->max_requests))){
            pthread_cond_wait(&sv->full,&sv->l);
          }

          sv->bufferq[sv->in] = connfd;
          sv->in = (sv->in+1)%(sv->max_requests+1);

          pthread_cond_signal(&sv->empty);
          pthread_mutex_unlock(&sv->l);
      }
}



void server_exit(struct server *sv){
        /* when using one or more worker threads, use sv->exiting to indicate to                                                                                                                           
         * these threads that the server is exiting. make sure to call                                            
         * pthread_join in this function so that the main server thread waits                                                                                                                              
         * for all the worker threads to exit before exiting. */
        sv->exiting = 1;
	
        //multiple consumers, 1 producer problem                                                                                                                                                           
        //wake up all sleeping consumers so that they can exit                                                                                                                                             
        pthread_cond_broadcast(&sv->empty);
        //printf("B4 join\n");
	
        for(int i = 0; i < sv->nr_threads; i++){
          int f= pthread_join(sv->threads[i], NULL);
          assert(!f);
        }

	//deallocate both the hash table and the recent list so forgo the risk of leak
	
        if(sv->max_cache_size > 0) {
	  
                for(int i = 0; i < sv->table->size; i++) {
                        if(sv->table->elements[i] != NULL) {
			  file_data_free(sv->table->elements[i]->data);
			  free(sv->table->elements[i]->key);
			  sv->table->elements[i] = NULL;
                        }
                }
	                                                                       
                struct cache_element* current = sv->table->recent_list->next;
                struct cache_element* next;

                while (current != NULL) {
                        next = current->next;
                        free(current);
                        current = next;
                }

                sv->table->recent_list->next = NULL;
                free(sv->table->recent_list);
                free(sv->table->elements);
                free(sv->table);

		pthread_mutex_destroy(&sv->table->lo);
        }
        pthread_cond_destroy(&sv->full);
        pthread_cond_destroy(&sv->empty);
        pthread_mutex_destroy(&sv->l);


        //printf("EXITING\n");                                                                                                                                                                              
        /* make sure to free any allocated resources */
        free(sv->bufferq);
        free(sv->threads);
        free(sv);
}


void enqueue(struct server* sv, int loc) {

  struct cache_element* head;
  struct cache_element* new;

  if(sv->table->recent_list == NULL) return;

  head = sv->table->recent_list;

  new  = (struct cache_element*)Malloc(sizeof(struct cache_element));
  new->f_key = loc;
  new->next = NULL;

  if(head->next == NULL)
    head->next = new;

  else {
    while(head->next != NULL) head = head->next;
    head->next = new;
  }
  
}

void dequeue(struct server* sv, int loc){

  struct cache_element* head;
  struct cache_element* prev;
  
  if(sv->table->recent_list->next == NULL) return;

  head= sv->table->recent_list->next;
  prev= sv->table->recent_list;

  while(head->next != NULL){

    if(head->f_key == loc){
      
      prev->next = head->next;
      head->next = NULL;
      free(head);
      head=NULL;
      return;
    }
    
    prev = head;
    head= head->next;
  }
  
}

//same as lab 1: from stackoverflow                                                                                                                
unsigned long hash_key(struct server *sv, char *str) {
    unsigned long hash = 5381;
    int c;

    while (c < strlen(str)){
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	c++;
    }
    return hash % sv->table->size;
}

struct file_element* cache_lookup(struct server *sv, struct file_data* data) {

  int loc = hash_key(sv, data->file_name);

  //find an empty spot for file at/near its hash key location
  
  while(sv->table->elements[loc] != NULL){

    if(strcmp(sv->table->elements[loc]->key, data->file_name) == 0) return sv->table->elements[loc];
    loc++;
    loc %= sv->table->size;
  }
  
  return NULL;
}

struct file_element* cache_insert(struct server *sv, struct file_data *data) {

  //already exists
  if(cache_lookup(sv,data) != NULL){
    return cache_lookup(sv,data);
  }

  //doesn't exist
  if(cache_lookup(sv,data) == NULL){

    //need more space to enter the data
    if(data->file_size > sv->max_cache_size - sv->table->current_cache){
      cache_evict(sv,data->file_size);
      return table_insert(sv,data);
    }
    //fits
    else{ return table_insert(sv,data);}
  }

  //doesn't exist and is too big > max_cache_size
  
  return NULL;
}

void cache_evict(struct server *sv, int file_size) {

  //take out the files starting from the front, until we have enough space
  struct cache_element* ptr;
  
  ptr = sv->table->recent_list->next;
  
  while(ptr != NULL && (sv->max_cache_size - sv->table->current_cache) < file_size) {
    struct file_element *file = sv->table->elements[ptr->f_key];
    if(file != NULL && file->using == 0) {
      //if(file->using == 0){
      sv->table->current_cache -= file->data->file_size;
      free(file->key);
      file_data_free(file->data);
      sv->table->elements[ptr->f_key] = NULL;
      dequeue(sv,ptr->f_key);
    }
    ptr = ptr->next;
  }
}

struct file_element* table_insert(struct server *sv, struct file_data *data) {

  //to big do not insert it into table
  if(data->file_size > sv->max_cache_size) return NULL;
  
  struct file_element* insert;
  unsigned long location;

  location = hash_key(sv, data->file_name);

  // if(data->file_size > sv->max_cache_size) return NULL;

  //find a location near or at the hash key of string
  
  while(sv->table->elements[location] != NULL && strcmp(sv->table->elements[location]->key, data->file_name) != 0) {
    location++;
    location%= sv->table->size;
  }

  //make a new object and put it into hash table
  insert = (struct file_element*)Malloc(sizeof(struct file_element));
  
  insert->key = (char *)Malloc((strlen(data->file_name) + 1) * sizeof(char));
  strcpy(insert->key, data->file_name);                                                                                                            
  insert->key[strlen(data->file_name)] = '\0';
  
  insert->data = data;
  insert->using = 0;
  insert->hash_num = location;
	
  sv->table->elements[location] = insert;
  sv->table->current_cache += data->file_size;

	/*
	struct cache_element* start= sv->table->recent_list;
	struct cache_element* ptr= NULL;
	
	if(start == NULL) return sv->table->elements[location];
	
	struct cache_element* head = (struct cache_element*)Malloc(sizeof(struct cache_element));
	head->f_key = location; 
	head->next = NULL;

	if(start->next == NULL){

	  start->next = head;
	}
	
	else{

	  ptr = head;
          while(ptr->next != NULL) ptr = ptr->next;
	  ptr->next = head;
		
	}
	*/

  //put it at the back of the cache list
  enqueue(sv,location);
  
  return sv->table->elements[location];
}


void cache_fix(struct server *sv, int loc){

  //remove hash_key object from the list
  dequeue(sv,loc);

  //input it into the end (as it is most recent)
  struct cache_element* insert =(struct cache_element*)Malloc(sizeof(struct cache_element*));
  insert->f_key = loc;
  insert->next = NULL;

  if(sv->table->recent_list->next == NULL) sv->table->recent_list->next = insert;

  else{

    struct cache_element* start = sv->table->recent_list->next;

    while(start->next != NULL){
      start= start->next;

    }
    start->next = insert; 
  }                                                
}

