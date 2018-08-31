#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#define TRI_DATA  5
#define TRI_EXIT  6

typedef struct BufferOps{
	int (*push)(struct BufferOps*,void *,int);
	int (*pull)(struct BufferOps*,void *,int);
	int (*deleteLeft)(struct BufferOps*,int);
	int (*wait)(struct BufferOps*);
	int (*exitWait)(struct BufferOps*);
}BufferOps,*pBufferOps;

pBufferOps createBufferServer(int size);
void destroyBufferServer(pBufferOps *buffer);







#endif
