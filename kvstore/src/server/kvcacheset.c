#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "uthash.h"
#include "utlist.h"
#include "kvconstants.h"
#include "kvcacheset.h"

void kvcacheentry_init(kvcacheentry_t *cacheentry, char *key,char *value){
	cacheentry->key = (char*)malloc(strlen(key)+1);
	cacheentry->value = (char*)malloc(strlen(value)+1);
	strcpy(cacheentry->key,key);
	strcpy(cacheentry->value,value);
	cacheentry->refbit = 1;
	return;
}

/* Initializes CACHESET to hold a maximum of ELEM_PER_SET elements.
 * ELEM_PER_SET must be at least 2.
 * Returns 0 if successful, else a negative error code. */
int kvcacheset_init(kvcacheset_t *cacheset, unsigned int elem_per_set) {
  int ret;
  if (elem_per_set < 2)
    return -1;
  cacheset->elem_per_set = elem_per_set;
  if ((ret = pthread_rwlock_init(&cacheset->lock, NULL)) < 0)
    return ret;
  cacheset->num_entries = 0;
	cacheset->head = NULL;
  return 0;
}


/* Get the entry corresponding to KEY from CACHESET. Returns 0 if successful,
 * else returns a negative error code. If successful, populates VALUE with a
 * malloced string which should later be freed. */
int kvcacheset_get(kvcacheset_t *cacheset, char *key, char **value) {
	kvcacheentry_t *ptr = cacheset->head;
	while(ptr){
		if( strcmp(ptr->key,key) == 0 ){
			*value = (char*)malloc(strlen(ptr->value));
			strcpy(*value,ptr->value);
			ptr->refbit = 1;
			printf("get value from cache\n");
			return 0;
		}
		ptr = ptr->next;
	}
  return -1;
}

/* Add the given KEY, VALUE pair to CACHESET. Returns 0 if successful, else
 * returns a negative error code. Should evict elements if necessary to not
 * exceed CACHESET->elem_per_set total entries. */
int kvcacheset_put(kvcacheset_t *cacheset, char *key, char *value) {
	kvcacheentry_t *ite = cacheset->head;
	while(ite){
		if(ite->key == key){
			strcpy(ite->value,value);
			ite->refbit = 1;
			return 0;
		}
		ite = ite->next;
	}
	kvcacheentry_t *ptr,*tmp;
	kvcacheentry_t *ele = (kvcacheentry_t*)calloc(1,sizeof(kvcacheentry_t));
	if(ele == NULL){
		return ENOMEM;
	}
	kvcacheentry_init(ele,key,value);
	if(cacheset->elem_per_set == cacheset->num_entries){
		DL_FOREACH_SAFE(cacheset->head,ptr,tmp){
    	if(ptr->refbit == 0){
  			kvcacheset_del(cacheset,ptr->key);
				break;
			}
			else{
				kvcacheentry_t *replace = (kvcacheentry_t*)calloc(1,sizeof(kvcacheentry_t));
				kvcacheentry_init(replace,ptr->key,ptr->value);
				replace->refbit = 0;
				DL_APPEND(cacheset->head,replace);
				DL_DELETE(cacheset->head,ptr);
			}
		}
	}
	else{
		cacheset->num_entries++;
	}
	DL_APPEND(cacheset->head,ele);
	printf("put key: %s value: %s to cache\n",key,value);
  return 0;
}

/* Deletes the entry corresponding to KEY from CACHESET. Returns 0 if
 * successful, else returns a negative error code. */
int kvcacheset_del(kvcacheset_t *cacheset, char *key) {
	kvcacheentry_t *ptr = cacheset->head;
	while(ptr){
		if( strcmp(ptr->key,key) == 0 ){
			printf("del key: %s value: %s from cache\n",ptr->key,ptr->value);
			DL_DELETE(cacheset->head,ptr);
			return 0;
		}
		ptr = ptr->next;
	}
  return -1;
}

/* Completely clears this cache set. For testing purposes. */
void kvcacheset_clear(kvcacheset_t *cacheset) {
}

void kvcacheset_traverse(kvcacheset_t *cacheset){
	kvcacheentry_t *ptr = cacheset->head;
	while(ptr){
		printf("Key: %s, Value: %s, refbit: %d\n",ptr->key,ptr->value,ptr->refbit);
		ptr = ptr->next;
	}
}
