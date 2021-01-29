#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

enum{
     RUNNING = 0,
     READY = 1,
     BLOCKED = 2,
     EXITED = 3

};    

struct thread* current_thread;
struct thread* existing_threads[THREAD_MAX_THREADS];
struct thread* next_t;

struct ready_queue* readyq;
struct morto_queue* killq;
volatile int setcontext_called = 0;

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */

  struct thread* head;
  int size;
};

/* This is the thread control block */
struct thread {
  Tid id;
  int state;
  ucontext_t* context; 
  void* stack;
  struct thread* next;
  struct wait_queue* waitq;
};

struct ready_queue{

  struct thread* head;
  //struct thread* next;
  int size;

};

struct morto_queue{

  struct thread* head;
  int size;
};

void thread_stub(void (*thread_main)(void *), void* arg){

  // Tid ret;
  interrupts_on();
  //  unintr_printf("%d in thread stub\n", interrupts_enabled());
  thread_main(arg);
  //interrupts_off();
  //printf("done function\n");
  thread_exit();
  //  interrupts_off();
}

void dequeue(struct thread* leave){

  if(leave == NULL) return;
  
  struct thread* find = readyq->head;

  //only one element
  if(find->next == NULL){
    readyq->head = readyq->head->next;
    return;
  }
  
  //head of queue
  if(readyq->head->id == leave->id){
    readyq->head = readyq->head->next;
    find->next= NULL;
    readyq->size-=1;
    return;
  }
  
  else{
    struct thread*prev;
    //for specific one that is not the head
    while(find != NULL){
      if(find->id == leave->id){
	prev->next = find->next;
	find->next = NULL;
	readyq->size-=1;
	return;
      }
      prev = find;
      find = find->next;
    }

    //    return THREAD_INVALID;
  }
  
  //readyq->size-= 1;
  //return;
}

Tid new_id(){

  for(int id =0; id < THREAD_MAX_THREADS; id++){
    if(existing_threads[id] == NULL) return id;
  }

  return THREAD_NOMORE;

}

void enqueue(struct thread* enter){

  //  int enabled = interrupts_off();
  struct thread* old_head = readyq->head;

  if(readyq->head == NULL){
    readyq->head = enter;
    enter->next = NULL;
    readyq->size= 1;
    //interrupts_set(enabled);
    return;
  }

  if(old_head->next == NULL){
    old_head->next = enter;
    enter->next = NULL;
    readyq->size = 2;
    //interrupts_set(enabled);
    return;
  }
  
 
    while(old_head!= NULL){
      
      if(old_head->next == NULL){
	old_head->next = enter;
	enter->next = NULL;
        readyq->size+=1;
	//interrupts_enabled();
	return;
      }
      
      old_head = old_head->next;
    }
    //old_head->next = enter;
    //enter->next = NULL;
    //return;
 
}

struct thread* search_t(Tid tid){

  struct thread* find = readyq->head;

  if(find == NULL) return NULL;

  if(find->next == NULL && readyq->head->id == tid) return find;
  else if(find->next == NULL && readyq->head->id != tid) return NULL;
  
  while(find->id != tid && find != NULL){

    find = find->next;

  }

  if(find != NULL && find->id == tid) return find;

  return NULL;

}

void kill_exit_threads(){

  struct thread* killed = killq->head;

  struct thread* next;

  if(killq->head == NULL) return;
  
  //another case maybe?
  
  while(killed != NULL){

    next = killed->next;

    free(killed->stack);
    free(killed->context);
    existing_threads[killed->id] = NULL;
    free(killed);
    killed = next;
    
    killq->size-=1;
  }

  killq->head = NULL;
  return;
}

void enqueue_kill(struct thread* morto){

  struct thread* tail = killq->head;

  //put at the tail of the queue

  if(killq->head == NULL){
    killq->head = morto;
    killq->head->next = NULL;
    killq->size+=1;
    return;
  }
  
  else{
    while(tail != NULL){
      if(tail->next == NULL){
        tail->next = morto;
       	morto->next = NULL;
	//	killq->size+=1;
        return;
      }
      tail = tail->next;
    }

  }
                                                                                                                                                                          
  //morto->next = NULL;
  //killq->size+=1;
  //return;
}

void thread_init()
{
  
  struct ready_queue* ready = (struct ready_queue*)malloc(sizeof(struct ready_queue));
  ready->head = NULL;
  ready->size = 0;

  struct morto_queue* kill = (struct morto_queue*)malloc(sizeof(struct morto_queue));
  kill->head = NULL;
  kill->size = 0;

  struct wait_queue* wait = wait_queue_create();
  
  next_t = NULL;
  struct thread* kernel = (struct thread*)malloc(sizeof(struct thread));
  //init stack pointer styll
  kernel->id = 0;
  kernel->state = RUNNING;
  kernel->context = (ucontext_t*)malloc(sizeof(ucontext_t));
  kernel->next = NULL;
  kernel->waitq = wait;
  //getcontext(kernel->context);
  // printf("made it past kernel init");
  for(int i = 0; i < THREAD_MAX_THREADS; i++) existing_threads[i] = NULL;
  
  existing_threads[0] = kernel;
  current_thread = kernel;
  readyq = ready;
  killq= kill;
  //waitq= wait;
}

Tid thread_id()
{
  return current_thread->id;
}

Tid thread_create(void (*fn) (void *), void *parg)
{
  int enabled = interrupts_off();
  Tid count = new_id();
  
  if(count == THREAD_NOMORE){
    //    free(new_t);
    interrupts_set(enabled);
    return THREAD_NOMORE;
  }

  struct thread* new_t = (struct thread*)malloc(sizeof(struct thread));
  new_t->context = (ucontext_t*)malloc(sizeof(ucontext_t));
                                                                                                                  
  void* stack = malloc(THREAD_MIN_STACK);

  if(stack == NULL){
    free(new_t->context);
    free(new_t);
    interrupts_set(enabled);
    return THREAD_NOMEMORY;
  }

  struct wait_queue* wait = wait_queue_create();
  wait->head = NULL;
  wait->size = 0;

  if(wait == NULL){
    free(new_t->context);
    free(new_t);
    interrupts_set(enabled);
    return THREAD_NOMEMORY;
  }
  
  int check=getcontext(new_t->context);
  assert(check == 0);
  //need to change stack pointer to point to the stack and also set the registers
  new_t->context->uc_mcontext.gregs[REG_RIP] = (unsigned long)thread_stub;
  new_t->context->uc_mcontext.gregs[REG_RBP] = (unsigned long)(stack);
  new_t->context->uc_mcontext.gregs[REG_RSP] = (unsigned long)(stack + THREAD_MIN_STACK - 8);
  new_t->context->uc_mcontext.gregs[REG_RDI] = (unsigned long)fn;
  new_t->context->uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;

  new_t->id = count;
  new_t->state = READY;
  new_t->next = NULL;
  new_t->stack = stack;
  new_t->waitq = wait;
  enqueue(new_t);
  //readyq->size+=1;
  existing_threads[new_t->id] = new_t;  

  interrupts_set(enabled);
  return new_t->id;
}

Tid thread_yield(Tid want_tid)
{

  int enabled = interrupts_off();
  
  kill_exit_threads();
  
  if(want_tid < THREAD_SELF || want_tid > THREAD_MAX_THREADS){
    interrupts_set(enabled);
    return THREAD_INVALID;
  }

  else if(want_tid > -1 && readyq->head == NULL && current_thread->id != want_tid){
    interrupts_set(enabled);
    return THREAD_INVALID;

  }

  else if(want_tid == THREAD_ANY && readyq->head == NULL){
    interrupts_set(enabled);
    return THREAD_NONE;
    
  }
  
  //its a valid thread

    if(want_tid == THREAD_ANY){
      
      setcontext_called = 0;
      struct thread* ptr = current_thread;
      next_t = readyq->head;
      
      int ret =getcontext(current_thread->context);
      assert(ret == 0);

      //if(next_t->state == EXITED){
      // thread_exit();
      //}

      if(setcontext_called == 1){
       
	interrupts_set(enabled);
	return current_thread->id;
      }
      
      readyq->head = next_t->next;
      current_thread->state = READY;
      enqueue(ptr);
      readyq->size-=1;
      current_thread = next_t;
      next_t->next = NULL;
      next_t->state = RUNNING;
      
      //  retx = current_thread->id;
      setcontext_called = 1;
      setcontext(next_t->context);
    }
  
    else if(want_tid == current_thread->id || want_tid == THREAD_SELF){
    //printf("%d\n", setcontext_called);                                                                                                                                                             
      setcontext_called = 0;
      int saveContext = getcontext(current_thread->context);
      if(saveContext == -1) printf("error\n");

      //PC arrives here after restoring itself, with local variable set_contextcalled == 1 in stack                                                                                                    
      if(setcontext_called == 1){

	interrupts_set(enabled);
	return current_thread->id;

      }
      setcontext_called = 1;
      setcontext(current_thread->context);
                                                                                                                                                                            
    }
    
    /*
    //same thread
    else if(want_tid == THREAD_SELF || current->id == tid){
      setcontext_called = 0;
      int save = getcontext(current_thread->context);
      assert(save == 0);
      if(setcontext_called == 1){
	 return current->id;
      }
      
      setcontext_called = 1;
      setcontext(current_thread->context);
      
    }
    */
    
    //search for specific one
      
    else{

      setcontext_called = 0;
      next_t = search_t(want_tid);

      if(next_t == NULL){

	interrupts_set(enabled);
	return THREAD_INVALID; 
      }
      //if(next_t->state == EXITED) thread_exit();
      getcontext(current_thread->context);    
      
      if(setcontext_called == 1){
      
	interrupts_set(enabled);
	return want_tid;
      }
      
      dequeue(next_t);
      enqueue(current_thread);
      current_thread->state = READY;
      next_t->state = RUNNING;
      current_thread = next_t;
     
      setcontext_called = 1;
      setcontext(next_t->context);
      
      
    }
    
    //if(tid > 1000 || tid < -2)return THREAD_INVALID;

  return THREAD_FAILED;
}

void thread_exit()
{
 
  //  kill_exit_threads(); //need to make

  int enabled = interrupts_off();

  thread_wakeup(existing_threads[current_thread->id]->waitq, 1);
  struct thread* kill = killq->head;
  struct thread* nexter;
  
  while(kill != NULL){

    nexter = kill->next;
    free(kill->context);
    free(kill->stack);
    wait_queue_destroy(kill->waitq);
    existing_threads[kill->id] = NULL;
    free(kill);
    killq->size-=1;

    kill= nexter;
  }

  killq->head= NULL;
  
  //dequeue(next_t);
  //  enqueue_kill(current_thread); // shouldnt this be current?? //add

  if(readyq->head == NULL){

    existing_threads[current_thread->id] = NULL;
    wait_queue_destroy(current_thread->waitq);
    free(current_thread);
    current_thread = NULL;
    interrupts_set(enabled);
    exit(0);
  }

  enqueue_kill(current_thread);
  //dequeue(current_thread);
  
  //make another thread run
  next_t = readyq->head;

  readyq->head = next_t->next;
  next_t->next = NULL;
  current_thread = next_t;
  current_thread->state = RUNNING;
  //interrupts_set(enabled);
  setcontext(next_t->context);
  interrupts_set(enabled);
  
  //will not return and will go to the next intruction (which ends up being its last run)
  // return THREAD_FAILED;
}

Tid thread_kill(Tid tid)
{

  int enabled = interrupts_off();
  
  if(tid < 0 || current_thread->id == tid || existing_threads[tid] == NULL || tid > THREAD_MAX_THREADS){

    //    printf("%d for kill\n", interrupts_enabled());
    interrupts_set(enabled);
    return THREAD_INVALID;
  }
  struct thread* morto = existing_threads[tid];

  morto->state = EXITED;
  morto->context->uc_mcontext.gregs[REG_RIP] = (unsigned long)thread_exit;
  //  morto->context->uc_mcontext.gregs[REG_RDI] = (unsigned long)NULL; //just in case first arg is a garbage value, shouldnt affect the program tho
  //might need to add it to the exit queue list here

  interrupts_set(enabled);
  return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue * wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->head = NULL;
	wq->size = 0;

	return wq;
}

void wait_queue_destroy(struct wait_queue *wq)
{

  //free all of the nodes in the queue first ?
  thread_wakeup(wq, 1);
  free(wq);
}

void wait_enqueue(struct wait_queue *queue, struct thread *enter){

  struct thread* tail = queue->head;

  if(queue->head == NULL){

    queue->head = enter;
    queue->size = 1;
    return;
  }

  while(tail->next != NULL){

    tail = tail->next;

  }

  tail->next = enter;
  queue->size+=1;
}

Tid thread_sleep(struct wait_queue *queue)
{
  //assert(interrupts_enabled());
  //  printf("in sleep curernt thread id: %d\n", current_thread->id);
  int enabled = interrupts_off();
  assert(!interrupts_enabled());
  //  assert(enabled);
 
  if(queue == NULL){

    interrupts_set(enabled);
    return THREAD_INVALID;
  }
   
  if(readyq->head == NULL){

    interrupts_set(enabled);
    return THREAD_NONE;
  }
  
  //struct thread* tail = queue->head;
  volatile int setcontext_called = 0;
  volatile Tid tid_called = 0;

  getcontext(current_thread->context);

  if(setcontext_called == 1){

    //put interrupts dummy
    interrupts_set(enabled);
    return tid_called;
    
  }
  
  //get the next ready thread

  next_t = readyq->head;
  readyq->head = next_t->next;
  readyq->size-= 1;
  current_thread->state = BLOCKED;
  next_t->state = RUNNING;

  //put the current thread into the tail of the wait queue
  /*
  if(tail != NULL){

    while(tail->next != NULL){

      tail= tail->next;
    }

    tail->next = current_thread;
    queue->size+=1;
    //printf("leaving current thread to sleep %d\n", tail->next->id);
   }

   else if(tail == NULL){
     queue->head = current_thread;
     queue->size = 1;
   }
  */

  wait_enqueue(queue, current_thread);
  current_thread = next_t;
  next_t->next= NULL;
  tid_called = next_t->id;
  setcontext_called = 1;
  //printf("leaving current thread to sleep %d\n", tail->next->id);
  setcontext(next_t->context);
  
  return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int thread_wakeup(struct wait_queue *queue, int all)
{

  int enabled = interrupts_off();
  assert(!interrupts_enabled());
  if(queue == NULL || queue->head == NULL){

    interrupts_set(enabled);
    return 0;
  }

  else if(all == 1){

    //wake up all threads in  the queue

    struct thread* head = queue->head;
    struct thread* rtail = readyq->head;

    int sizeq = queue->size;
    
    if(rtail == NULL){
      readyq->head = head;
    }
    
    else{
      while(rtail!= NULL){

	if(rtail->next == NULL){
	  rtail->next = head;
	  break;
	}
	rtail = rtail->next;

      }
    }

    while(head != NULL){

      head->state= READY;
      head= head->next;

    }
    
    //enqueue(head);
    //rtail->next = head;
    queue->head = NULL;
    queue->size = 0;
    readyq->size+= sizeq;
    interrupts_set(enabled);
    return sizeq;

  }

  else if(all == 0){

    //wake up head of the queue
    struct thread* head = queue->head;
    struct thread* rtail = readyq->head;

    queue->head = queue->head->next;
    head->next = NULL;
    //int sizeq= queue->size;

    if(rtail == NULL){

      readyq->head = head;
      head->state = READY;
    }

    else{
      while(rtail != NULL){

	if(rtail->next == NULL){
	  rtail->next = head;
	  head->state = READY;
	  break;
	}
	rtail= rtail->next;
      }
    }

    //queue->head= head->next;
    //head->next = NULL;
    queue->size-= 1;
    readyq->size+=1;
    
    interrupts_set(enabled);
    return 1;

  }
  
  interrupts_set(enabled);
  return THREAD_FAILED; 
}

/* suspend current thread until Thread tid exits */
Tid thread_wait(Tid tid)
{
  int enabled = interrupts_off();
  if(tid == current_thread->id || existing_threads[tid] == NULL || tid > THREAD_MAX_THREADS || tid < 0){

    interrupts_set(enabled);
    return THREAD_INVALID;
  }

  thread_sleep(existing_threads[tid]->waitq);
  kill_exit_threads();
  interrupts_set(enabled);
  return tid;
  
}

struct lock {
	/* ... Fill this in ... */

  struct wait_queue* waitq;
  int lock;
  int holder_id;
};

struct lock * lock_create()
{
  int enabled = interrupts_off();
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	struct wait_queue* wq = wait_queue_create();
	
	lock->holder_id = THREAD_NONE; //no body is holding it
	
	lock->waitq= wq;
	lock->lock = 0;
	interrupts_set(enabled);
	return lock;
}

void lock_destroy(struct lock *lock)
{
  int enabled = interrupts_off();
	assert(lock != NULL);

	wait_queue_destroy(lock->waitq);
	interrupts_set(enabled);
	free(lock);
}

void lock_acquire(struct lock *lock)
{
	assert(lock != NULL);
	int enabled = interrupts_off();

	while(lock->lock == 1 && lock->holder_id != THREAD_NONE){

	  thread_sleep(lock->waitq);
	}

	lock->lock = 1;
	lock->holder_id = current_thread->id;
	interrupts_set(enabled);
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);
	int enabled = interrupts_off();
	
	if(lock->lock == 1){
	  lock->lock = 0;
	  lock->holder_id = THREAD_NONE;
	  thread_wakeup(lock->waitq, 1);
	}
	
	interrupts_set(enabled);
}

struct cv {
	/* ... Fill this in ... */
  struct wait_queue* waitq;
  
};

struct cv * cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	cv->waitq= wait_queue_create();

	return cv;
}

void cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	wait_queue_destroy(cv->waitq);

	free(cv);
}

void cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	lock_release(lock);
	thread_sleep(cv->waitq);
	lock_acquire(lock);
}

void cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	if(lock->holder_id == current_thread->id){
	  thread_wakeup(cv->waitq, 0);
	}
}

void cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	if(lock->holder_id == current_thread->id){
	thread_wakeup(cv->waitq, 1);
	}
}
