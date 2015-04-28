/******************************************************************************
*                                                                             *
*   File Name   : mem_pool.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "mem_pool.h"

//�ڴ�صĲ���
typedef struct _Queue_List      //ÿһ���û�����������ȡNat��Ԫ
{
    UINT4       total_len;          //�ܹ��ĵ�Ԫ����
    UINT4       usable_len;         //���õ�
    void        *data;
    void        **arr;
    UINT4       head;
    UINT4       tail;
    void        *mutex;
    char        *states;            //��ֹ�ظ��ͷ�,0:û��ռ��  1:�Ѿ�ռ��
    UINT2       block;
}Queue_List;

//�鷵��,�ȼ��ڷ�
static int Queue_In(void *_q_list, void *data)
{
    Queue_List *q_list;
    UINT4   tail;
//  UINT4   head;
    int     pos;
//  void *unit=NULL;
    q_list = (Queue_List *)(_q_list);

    if(data == NULL)
    {
        return -1;
    }

    //����Ϊ0
    if( (data - q_list->data) %  q_list->block )
    {
        printf("free add error,%p\n",data);
        return -1;
    }
    pos = (data - q_list->data) /  q_list->block ;      //�ڵ��λ��
    if(pos <0 || pos >= q_list->total_len )
    {
        printf("free add error over %d\n",pos);
        return -1;
    }
    if( *(q_list->states + pos ) == 1 )                 //˵���ظ��ͷ����ڴ�
    {
        printf("alread free \n");
        return -1;
    }

    pthread_mutex_lock(q_list->mutex );
    {
        tail = q_list->tail;
        q_list->arr[tail] = data;
        *(q_list->states + pos ) = 1;
        q_list->tail = ( tail + 1 ) % q_list->total_len;
        q_list->usable_len++;
    }
    pthread_mutex_unlock(q_list->mutex );

    return 1;
}



//�����õģ��ȳ��ڼ�
static void *Queue_Out(void *_q_list)      //�Ѷ���
{
    Queue_List *q_list;
    int head;
//  int tail;
    int pos;
    void *unit=NULL;
    q_list = (Queue_List *)(_q_list);

    pthread_mutex_lock(q_list->mutex );
    {
        if( q_list->usable_len >0 )
        {
            head = q_list->head;
            unit = q_list->arr[head];
            pos = (unit - q_list->data)  /  q_list->block;          //ȷ���ڵ��ַ
            *(q_list->states + pos) = 0;                            //û��ռ��
            q_list->usable_len--;
            q_list->head = (head+1)%q_list->total_len;
            memset(unit, 0, q_list->block);
        }
    }
    pthread_mutex_unlock(q_list->mutex );
    return unit;
}

static void *Queue_Init(UINT4 block ,UINT4 len)
{
    //UINT4   i;
	UINT8 i;
    char    *data;
    Queue_List *q_list;

    q_list = (Queue_List *)malloc(sizeof(Queue_List));
    if(q_list == NULL ) return q_list;
    memset(q_list, 0, sizeof(Queue_List));

    //��
    q_list->mutex = (pthread_mutex_t *)malloc( sizeof(pthread_mutex_t) );
    if(q_list->mutex == NULL)   return NULL;
    pthread_mutex_init(q_list->mutex , NULL);

    q_list->data = malloc(    block * len );  //�׵�ַ
    if( q_list->data == NULL )  return NULL;
    memset(q_list->data, 0, block * len);

    q_list->arr = (void **)malloc( len * sizeof(void *) );
    if( q_list->arr == NULL )   return NULL;
    memset(q_list->arr, 0, len * sizeof(void *));

    q_list->states = (char *)malloc( len * sizeof(char) );
    if( q_list->states == NULL )    return NULL;
    memset(q_list->states, 0, len * sizeof(char));


    q_list->head = 0;
    q_list->tail = 0;               //��ָ��0λ�ô�
    q_list->usable_len = 0;         //���õĸ���
    q_list->block = block;
    q_list->total_len = len;


    for(i=0; i < len ; i++)
    {
        data = q_list->data + i *block ;
        Queue_In(q_list, data);
    }
    return (void *)q_list;

}


void *mem_create(UINT4 block , UINT4 len)   //���С ���ڵ����
{
    void *pool;
    pool = Queue_Init(block,len);
    return  (void *)pool;
}

void *mem_get(void *pool)
{
    void *data;
    data = Queue_Out(pool);
    return data;
}

int mem_free(void *pool ,void *data)    //��ֹͬһ�ڴ��ͷ�����
{
    return Queue_In(pool , data);
}

void mem_destroy(void *pool)
{
    Queue_List *q_list = (Queue_List *)pool;
    free(q_list->arr);
    free(q_list->data);
    free(q_list->states);
    if(q_list->mutex)
    {
        pthread_mutex_destroy(q_list->mutex);
        free(q_list->mutex);
    }

    free(q_list);
}

UINT4 mem_num(void *_pool)              //���ýڵ�ĸ���
{
    Queue_List *pool;
    pool = (Queue_List *)(_pool);
    return (pool->total_len - pool->usable_len);
}
