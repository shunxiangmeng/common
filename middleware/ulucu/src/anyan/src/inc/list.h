#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <semaphore.h>
#include "Anyan_Device_SDK.h"

#ifndef LIST_MALLOC
#define LIST_MALLOC malloc
#endif

#ifndef LIST_FREE
#define LIST_FREE free
#endif

#define  CACHE_BLOCK_SIZE	(1024*2)
//#define  MAX_FRAME_SIZE		(1024*8*20)
#define  MAX_FRAME_SIZE		(1024*8*20*2)
#define  MAX_BLOCK_PER_FRAME	(MAX_FRAME_SIZE/CACHE_BLOCK_SIZE)

typedef enum
{
	LIST_HEAD, LIST_TAIL
} list_direction_t;

typedef struct
{
	int blockIdx;
	int status;	//1 used;0 free
	char* pBuf;

} data_block_t;

typedef struct list_node
{
	struct list_node *prev;
	struct list_node *next;
	void *val;	//not used,for extend

	uint32 msg_serial_num;
	int channelnum;
	uint16 frm_rate;
	uint16 frm_type;
	uint32 frameid;
	uint32 frm_av_id;
	uint32 frm_ts_sec;
	uint32 frm_ts;
	uint32 crc32_hash;
	int whole_len;
	int offset;
	int len;
	data_block_t* block[MAX_BLOCK_PER_FRAME];
	int block_count_used;
} list_node_t;

typedef struct
{
	list_node_t *head;
	list_node_t *tail;
	unsigned int len;
	void (*free)(void *val);
	int (*match)(void *a, void *b);
} list_t;

typedef struct
{
	list_node_t *next;
	list_direction_t direction;
} list_iterator_t;

list_t *list_new();
list_node_t *list_node_new(void *val);
list_node_t *list_rpush(list_t *self, list_node_t *node);
list_node_t *list_lpush(list_t *self, list_node_t *node);
list_node_t *list_rpop(list_t *self);
list_node_t *list_lpop(list_t *self);
list_node_t *list_find(list_t *self, void *val);
list_node_t *list_at(list_t *self, int index);
void list_remove(list_t *self, list_node_t *node);
void list_destroy(list_t *self);
void list_remove_all(list_t *self);

// list_t iterator prototypes.
list_iterator_t *list_iterator_new(list_t *list, list_direction_t direction);
list_iterator_t *list_iterator_new_from_node(list_node_t *node, list_direction_t direction);
list_node_t *list_iterator_next(list_iterator_t *self);
void list_iterator_destroy(list_iterator_t *self);

#ifdef __cplusplus
}
#endif

#endif /* LIST_H */
