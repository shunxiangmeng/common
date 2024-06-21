#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "define.h"
#include "ayqueue.h"
#include "ay_sdk.h"
#include "ayutil.h"

int ay_init_frm_buf(Chnl_Buf_Queue_head *pHead,unsigned buf_num)
{
    int i ,ret = 0;
    frm_buf_struct   *ptemp;

    memset((char *)pHead, 0, sizeof(*pHead));
    pthread_mutex_init(&pHead->q_lock, NULL);
    sem_init(&pHead->q_sem,0,0);

    ptemp = (frm_buf_struct *)calloc(buf_num,sizeof(frm_buf_struct));
    if(ptemp==NULL)
    {
		ERRORF("calloc frm buf mng[%d] fail!\n",buf_num);
		return -1;
    }

    pHead->phead = ptemp; // for destroy
    pHead->total_nums = buf_num;

    pHead->writeQueue.head = ptemp;
    pHead->writeQueue.tail = ptemp;
    pHead->readQueue.head = NULL;
    pHead->readQueue.tail = NULL;
    for ( i = 0 ; i < buf_num ; i++)
    {
		ptemp->buf = (char*) malloc(FRAME_SNED_SIZE);
		if(ptemp->buf==NULL)
		{
			ERRORF("malloc frame unit[%d] fail!\n",i);
			ret = -1;
			break;
		}

	memset(ptemp->buf,0,FRAME_SNED_SIZE);
	if(i == buf_num-1)
	{
	    ptemp->pnext = NULL; // last item
	    ret = 0;
	    break;
	}
	else
	{
	    ptemp->pnext = ptemp + 1;
	    ptemp = ptemp->pnext;
	}
    }

    pHead->writeQueue.tail = ptemp;
    return ret;
}

int ay_get_frame_buf_free(Chnl_Buf_Queue_head *pQHead)
{
    int free_size = 0;
    int frm_nums = 0,total_nums = 0;

    pthread_mutex_lock(&pQHead->q_lock);
    frm_nums = pQHead->frm_nums;
    total_nums = pQHead->total_nums; 
    pthread_mutex_unlock(&pQHead->q_lock);

    free_size = (total_nums - frm_nums)*FRAME_SNED_SIZE; 
    return free_size;
}

int  ay_add_chnl_frame(Chnl_ctrl_table_struct *ptable,Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pdata)
{
    int count = 0;
    int chn = pdata->channelnum - 1;
    frm_buf_struct   *ptemp,*ptemp1,*pre_ptemp;

    if(pdata->len > FRAME_SNED_SIZE)
    {
		return -1;
    }
    pthread_mutex_lock(&pQHead->q_lock);

    if(pdata->frm_type==CH_I_FRM && pdata->offset==0)
    {
	if(pQHead->frm_nums>0 && compare_send_I_ts(ptable,pdata->channelnum,pdata->frm_rate,pdata->frm_ts))
	{
	    int flag = 0;
	    count = 0;
	    pre_ptemp = NULL;
	    ptemp = pQHead->readQueue.head;
	    while(ptemp != NULL)
	    {
		if(ptemp->channelnum==pdata->channelnum && ptemp->frm_type==CH_I_FRM && ptemp->offset==0)
		{
		    flag = 1;
		}
		if(flag && ptemp->channelnum==pdata->channelnum && IS_LIVE_FRAME(ptemp->frm_type))
		{// move the node to write queue
		    count ++;
		    pQHead->frm_nums--;

		    //printf("### frm_nums = %u,ptemp = %p,pre_ptemp = %p\n",pQHead->frm_nums,ptemp,pre_ptemp);
		    //printf("### readQueue head = %p,tail = %p\n",pQHead->readQueue.head,pQHead->readQueue.tail);
		    //printf("### writeQueue head = %p,tail = %p\n",pQHead->writeQueue.head,pQHead->writeQueue.tail);
		    if(ptemp == pQHead->readQueue.head)
		    {// because we would remove this node,so we should move read to next node
			pQHead->readQueue.head = pQHead->readQueue.head->pnext;
		    }
		    if(ptemp == pQHead->readQueue.tail)
		    {
			pQHead->readQueue.tail = pre_ptemp;
		    }
		    ptemp1 = ptemp->pnext;

		    ptemp->pnext = NULL;
		    if(pQHead->writeQueue.head==NULL)
		    {
			pQHead->writeQueue.head = ptemp;
			pQHead->writeQueue.tail = ptemp;
		    }
		    else
		    {
			pQHead->writeQueue.tail->pnext = ptemp;
			pQHead->writeQueue.tail = ptemp;
		    }

		    if(pre_ptemp!=NULL)
		    {
			pre_ptemp->pnext = ptemp1;
		    }
		    ptemp = ptemp1;
		}
		else
		{// next node
		    pre_ptemp = ptemp;
		    ptemp = ptemp->pnext;
		}
	    }
	    if(pQHead->readQueue.head == NULL)
	    {// read queue now is empty,so must clear tail!!!
		pQHead->readQueue.tail = NULL;
	    }
	    if(count > 0)
	    {
		DEBUGF("del delay frms,cnt = %d,nums = %d\n",count,pQHead->frm_nums);
		printf("del delay frms,cnt = %d,nums = %d\n",count,pQHead->frm_nums);
#if 0
		ptemp = pQHead->readQueue.head;
		while(ptemp!=NULL)
		{
		    printf("$$ readQueue %p,next %p\n",ptemp,ptemp->pnext);
		    ptemp = ptemp->pnext;
		}
		ptemp = pQHead->writeQueue.head;
		while(ptemp!=NULL)
		{
		    printf("$$ writeQueue %p,next %p\n",ptemp,ptemp->pnext);
		    ptemp = ptemp->pnext;
		}
#endif
	    }
	}
    }

    if(pQHead->frm_nums == pQHead->total_nums) // buf full
    {
	int flag = 0;

	count = 0;
	printf("--->[aysdk]frame buf is full,num = %d!!!\n",pQHead->frm_nums);
	DEBUGF("[f0]frame buf is full,num = %d!!!\n",pQHead->frm_nums);
	pre_ptemp = NULL;
	ptemp = pQHead->readQueue.head;
	do{
	    if(!flag && ptemp->channelnum==pdata->channelnum && ptemp->offset==0 && (ptemp->frm_type==CH_I_FRM||ptemp->frm_type==CH_P_FRM))
	    {
		flag = 1;
	    }
	    if(flag == 1 && ptemp->channelnum==pdata->channelnum && IS_LIVE_FRAME(ptemp->frm_type))
	    {
		count ++;
		pQHead->frm_nums--;

		if(ptemp == pQHead->readQueue.head)
		{// because we would remove this node,so we should move read to next node
		    pQHead->readQueue.head = pQHead->readQueue.head->pnext;
		}
		if(ptemp == pQHead->readQueue.tail)
		{
		    pQHead->readQueue.tail = pre_ptemp;
		}

		ptemp1 = ptemp->pnext;

		ptemp->pnext = NULL;
		if(pQHead->writeQueue.head==NULL)
		{
		    pQHead->writeQueue.head = ptemp;
		    pQHead->writeQueue.tail = ptemp;
		}
		else
		{
		    pQHead->writeQueue.tail->pnext = ptemp;
		    pQHead->writeQueue.tail = ptemp;
		}

		if(pre_ptemp!=NULL)
		{
		    pre_ptemp->pnext = ptemp1;
		}
		ptemp = ptemp1;
	    }
	    else
	    {
		pre_ptemp = ptemp;
		ptemp = ptemp->pnext;
	    }
	}while(ptemp!=NULL);
	if(pQHead->readQueue.head == NULL)
	{// read queue now is empty,so must clear tail!!!
	    pQHead->readQueue.tail = NULL;
	}
	if(count > 0)
	{
	    pQHead->need_Iframe_flag[chn] = 1;
	    printf("[f1]del I-P-group cnt = %d,nums = %d\n",count,pQHead->frm_nums);
	    DEBUGF("[f1]del I-P-group cnt = %d,nums = %d\n",count,pQHead->frm_nums);
	}
    }

    if(pQHead->need_Iframe_flag[chn] && pdata->frm_type==CH_I_FRM && pdata->offset==0)
    {
	//printf("-------Iframe come,sdk frm nums = %d\n",pQHead->frm_nums);
	pQHead->need_Iframe_flag[chn] = 0;
    }
    else if(pQHead->need_Iframe_flag[chn] && IS_LIVE_FRAME(pdata->frm_type))
    {// just discard
	//printf("-------not Iframe, sdk frm nums = %d\n",pQHead->frm_nums);
	pthread_mutex_unlock(&pQHead->q_lock);
	return pQHead->frm_nums;
    }
    else if(pQHead->frm_nums == pQHead->total_nums)
    {// still full,we can do nothing now!
	//printf("-------still full,sdk frm nums = %d\n",pQHead->frm_nums);
	pthread_mutex_unlock(&pQHead->q_lock);
	return pQHead->frm_nums;
    }

    ptemp = pQHead->writeQueue.head;
    if(ptemp!=NULL)
    {
	memcpy(ptemp->buf, pdata->buf, pdata->len);
	ptemp->channelnum = pdata->channelnum;
	ptemp->frm_av_id = pdata->frm_av_id;
	ptemp->frameid = pdata->frameid;
	ptemp->frm_rate = pdata->frm_rate;
	ptemp->media_audio_type = pdata->media_audio_type;
	ptemp->media_video_type = pdata->media_video_type;
	ptemp->frm_ts_sec = pdata->frm_ts_sec;
	ptemp->frm_ts= pdata->frm_ts;
	ptemp->frm_type = pdata->frm_type;
	ptemp->len = pdata->len;
	ptemp->offset = pdata->offset;
	ptemp->whole_len = pdata->whole_len;
	ptemp->msg_serial_num = pQHead->last_serial_num++;
	

	pQHead->frm_nums++;

	// move write queue to next
	pQHead->writeQueue.head = pQHead->writeQueue.head->pnext;
	if(pQHead->writeQueue.head==NULL)
	{// empty
	    pQHead->writeQueue.tail = NULL;
	}
	// add this to read queue tail
	ptemp->pnext = NULL;
	if(pQHead->readQueue.head==NULL)
	{// frm_nums is 1.
	    pQHead->readQueue.head = ptemp;
	    pQHead->readQueue.tail = ptemp;

	    sem_post(&pQHead->q_sem);
	    sem_post(&sdk_get_handle(0)->stream.data_ok_sem);
	}
	else
	{
	    pQHead->readQueue.tail->pnext = ptemp;
	    pQHead->readQueue.tail = ptemp;
	}
    }
#if 0 // only for debug
    ptemp = pQHead->readQueue.head;
    count = 0;
    while(ptemp!=NULL)
    {
	//printf("$$ readQueue %p,next %p\n",ptemp,ptemp->pnext);
	count ++;
	ptemp = ptemp->pnext;
    }
    printf("$$$$$$$$$$$$$$ readQueue head = %p,tail = %p,count = %d\n",pQHead->readQueue.head,pQHead->readQueue.tail,count);
    ptemp = pQHead->writeQueue.head;
    count = 0;
    while(ptemp!=NULL)
    {
	//printf("$$ writeQueue %p,next %p\n",ptemp,ptemp->pnext);
	count ++;
	ptemp = ptemp->pnext;
    }
    printf("$$$$$$$$$$$$$$ writeQueue head = %p,tail = %p,count = %d\n",pQHead->writeQueue.head,pQHead->writeQueue.tail,count);
#endif
    pthread_mutex_unlock(&pQHead->q_lock);
    return pQHead->frm_nums;
}

int ay_get_frame_buf_first(Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pinfo)
{
    frm_buf_struct   *ptemp;
    int rt = -1,ret = 0;

    ret = pthread_mutex_trylock(&pQHead->q_lock);
    if(ret == 0)
    {
	rt = pQHead->frm_nums; 
	if ( pQHead->frm_nums > 0)
	{
	    ptemp = pQHead->readQueue.head;
	    memcpy((char*)pinfo, ptemp, sizeof(frm_buf_struct));
	}

		pthread_mutex_unlock(&pQHead->q_lock);
    }
    return rt;
}

int ay_get_frame_buf_first_timedwait(Chnl_Buf_Queue_head *pQHead,frm_buf_struct *pinfo,int ms)
{
    frm_buf_struct   *ptemp;
    int rt = -1;

    if(pthread_mutex_trylock(&pQHead->q_lock) == 0)
    {
	rt = pQHead->frm_nums; 
	if ( pQHead->frm_nums > 0)
	{
	    ptemp = pQHead->readQueue.head;
	    memcpy((char*)pinfo, ptemp, sizeof(frm_buf_struct));
	}
	pthread_mutex_unlock(&pQHead->q_lock);
    }
    
    if(rt == 0)
    {
		ayutil_wait_sem(&pQHead->q_sem,ms);
    }
    return (rt>0)?0:-1;
}

int  ay_del_frame_buf_first(Chnl_Buf_Queue_head *pQHead,uint32  frmid)
{
    frm_buf_struct   *ptemp;

    pthread_mutex_lock(&pQHead->q_lock);
    if(pQHead->frm_nums > 0)
    {
	ptemp = pQHead->readQueue.head;
	/*此处id如果相等表明可以删除否则可能缓冲区溢出被新的id覆盖掉了.则无需在删除了*/
	if(ptemp->frameid == frmid)
	{
	    pQHead->frm_nums--;

	    pQHead->readQueue.head = ptemp->pnext;
	    if(pQHead->readQueue.head==NULL)
	    {// empty
		pQHead->readQueue.tail = NULL;
	    }

	    ptemp->pnext = NULL;
	    if(pQHead->writeQueue.head==NULL)
	    {
		pQHead->writeQueue.head = ptemp;
		pQHead->writeQueue.tail = ptemp;
	    }
	    else
	    {
		pQHead->writeQueue.tail->pnext = ptemp;
		pQHead->writeQueue.tail = ptemp;
	    }
	}
    }
    pthread_mutex_unlock(&pQHead->q_lock);
    return 0;
}

int ay_destroy_frm_buf(Chnl_Buf_Queue_head *pQHead)
{
    int i = 0;
    frm_buf_struct	 *ptemp;

    pthread_mutex_lock(&pQHead->q_lock);

    ptemp = pQHead->phead;
    for(i=0;i<pQHead->total_nums;i++) 
    {
	ptemp = ptemp + 1;
	if (ptemp->buf != NULL)
	{
	    free(ptemp->buf);
	    ptemp->buf = NULL;
	}
    }
    free(pQHead->phead);
    pQHead->phead = NULL;
    sem_destroy(&pQHead->q_sem);
    pthread_mutex_unlock(&pQHead->q_lock);
    pthread_mutex_destroy(&pQHead->q_lock);
    return 0;
}

int init_msg_queue(Msg_Buf_Queue_head *queue, int capacity)
{
    msg_buf_struct   *ptemp, *ptemp1;
    int ret = 0,i;

    memset((char *)queue, 0, sizeof(Msg_Buf_Queue_head));
    pthread_mutex_init(&queue->q_lock, NULL);
    sem_init(&queue->q_sem,0,0);

    ptemp = (msg_buf_struct *)malloc(sizeof(msg_buf_struct));
    if(!ptemp)
    {
		ERRORF("malloc msg buf mng[0] fail\n");
		return -1;
    }
    memset(ptemp, 0, sizeof(msg_buf_struct));//清零
    queue->q_list = ptemp;
    queue->total_nums = capacity;
    for ( i = 1 ; i < capacity; i++)
    {
		ptemp1 = (msg_buf_struct *)malloc(sizeof(msg_buf_struct));
		if(!ptemp1)
		{
			ERRORF("malloc msg buf mng[%d] fail\n",i);
			ret = -1;
			break;
		}
		memset(ptemp1, 0, sizeof(msg_buf_struct));
		ptemp->pnext = ptemp1;//链接
		ptemp = ptemp->pnext;//后移
    }

    queue->q_p_end = ptemp;
    queue->q_p_end->pnext = queue->q_list;
    return ret;
}

int add_msg_queue_item(Msg_Buf_Queue_head *queue,char *data,int len,int flag)
{
    int i, ret = 0;
    msg_buf_struct   *ptemp;

    if(len > CMD_BUF_SIZE)
    {
		return -1;
    }

    pthread_mutex_lock(&queue->q_lock);
    ptemp = queue->q_list;
    if (queue->frm_nums < queue->total_nums)
    {
		for(i = 0; i < queue->frm_nums; i++)
		{
			ptemp = ptemp->pnext;
		}

		ptemp->len = len;
		memcpy(ptemp->buf, data, len);
		queue->frm_nums++;
		if(queue->frm_nums == 1)
		{
			sem_post(&queue->q_sem);
			sem_post(&sdk_get_handle(0)->stream.data_ok_sem);
		}
		ptemp->msg_serial_num = queue->last_serial_num++;
    }
    else if(ADD_MSG_FLAG_FHINT == flag)
    {
		ret = AY_ERROR_MSGFULL;
    }
    else
    {
		ptemp->len = len;
		memcpy(ptemp->buf, data, len);

		queue->q_list = queue->q_list->pnext;
		queue->q_p_end->pnext = ptemp;
		queue->q_p_end = ptemp;
		queue->frm_nums = queue->total_nums;

		ptemp->msg_serial_num = queue->last_serial_num++;
    }
    pthread_mutex_unlock(&queue->q_lock);

    return ret;
}

int get_msg_queue_item(Msg_Buf_Queue_head *queue,char *pmsg_out)
{
    msg_buf_struct   *ptemp;

	pthread_mutex_lock(&queue->q_lock);
    if ( queue->frm_nums > 0)
    {
	ptemp = queue->q_list;

	memcpy((char*)pmsg_out, ptemp->buf, ptemp->len);
	pthread_mutex_unlock(&queue->q_lock);
	return ptemp->len;
    }
	pthread_mutex_unlock(&queue->q_lock);
    return -1;
}

int del_msg_queue_first(Msg_Buf_Queue_head *queue)
{
    msg_buf_struct   *ptemp1;

    pthread_mutex_lock(&queue->q_lock);
    if ( queue->frm_nums > 0)
    {
		queue->frm_nums--;//帧数量计数器

		ptemp1 = queue->q_list;
		queue->q_list = queue->q_list->pnext;/*头后移*/
		queue->q_p_end->pnext = ptemp1;/*更新p_end*/
		queue->q_p_end = ptemp1;/*更新p_end*/
    }
    pthread_mutex_unlock(&queue->q_lock);

    return 0;
}

int destroy_msg_queue(Msg_Buf_Queue_head *queue)
{
    msg_buf_struct	 *ptemp, *ptemp1;

    pthread_mutex_lock(&queue->q_lock);
    ptemp = queue->q_list;

    queue->frm_nums = 0;
    queue->q_p_end->pnext = NULL; //断开链表

    while(ptemp)
    {
		ptemp1 = ptemp->pnext;
		free(ptemp);
		ptemp = ptemp1;
    }
    sem_destroy(&queue->q_sem);
    pthread_mutex_unlock(&queue->q_lock);
    pthread_mutex_destroy(&queue->q_lock);

    return 0;
}

///////////////////////////////////////////////// Just Wrapper APIs ////////////////////////////////////////////////
/****** Message Cmd Queue (Device->Server) *******************/
int  Msg_Cmd_Add_queue(char *data, int len)
{
    return add_msg_queue_item(&sdk_get_handle(0)->devobj[0].cmdUpQueue,data,len,0);
}
int  add_msg_cmd(char *data, int len,int flag)
{
    return add_msg_queue_item(&sdk_get_handle(0)->devobj[0].cmdUpQueue,data,len,flag);
}
int  __Msg_Cmd_Get_queue(char *pmsg_out)
{
    return get_msg_queue_item(&sdk_get_handle(0)->devobj[0].cmdUpQueue,pmsg_out);
}
int  Msg_Cmd_Del_queue_first(void)
{
    return del_msg_queue_first(&sdk_get_handle(0)->devobj[0].cmdUpQueue);
}
int   Msg_Cmd_Get_queue(char *pmsg_out, uint32  *msg_serialnum)
{
    msg_buf_struct *ptemp;
    if(sdk_get_handle(0)->devobj[0].cmdUpQueue.frm_nums > 0)
    {
		ptemp = sdk_get_handle(0)->devobj[0].cmdUpQueue.q_list;
		memcpy((char*)pmsg_out,ptemp->buf,ptemp->len);

		*msg_serialnum = ptemp->msg_serial_num;
		return ptemp->len;
	}
    return -1;
}
int   Msg_Cmd_Get_queue_timedwait(char *pmsg_out, int ms)
{
    msg_buf_struct *ptemp;
    if(sdk_get_handle(0)->devobj[0].cmdUpQueue.frm_nums > 0)
    {
		ptemp = sdk_get_handle(0)->devobj[0].cmdUpQueue.q_list;
		memcpy((char*)pmsg_out,ptemp->buf,ptemp->len);
		//DEBUG("msg nums = %d,this len = %d\n",sdk_get_handle(0)->devobj[0].cmdUpQueue.frm_nums,ptemp->len);
	
		return ptemp->len;
    }
    ayutil_wait_sem(&sdk_get_handle(0)->devobj[0].cmdUpQueue.q_sem,ms);
    return -1;
}

/****** Message Down Cmd Queue (Server -> Device) *******************/
int  Msg_Cmd_Add_down_queue(char *data, int len)
{
    return add_msg_queue_item(&sdk_get_handle(0)->devobj[0].cmdDownQueue,data,len,0);
}
int  Msg_Cmd_Get_down_queue(char *pmsg_out)
{
    return get_msg_queue_item(&sdk_get_handle(0)->devobj[0].cmdDownQueue,pmsg_out);
}
int  Msg_Cmd_Del_queue_down_first(void)
{
    return del_msg_queue_first(&sdk_get_handle(0)->devobj[0].cmdDownQueue);
}
int  Msg_Cmd_Destroy_down_queue(void)
{
    return destroy_msg_queue(&sdk_get_handle(0)->devobj[0].cmdDownQueue);
}

/*********** Audio Message Queue (Server -> Device) *****************/
int  Audio_msg_Add_queue(char *data, int len)
{
    return add_msg_queue_item(&sdk_get_handle(0)->devobj[0].talkQueue,data,len,0);
}
int  Audio_msg_Get_queue(char *pmsg_out)
{
    return get_msg_queue_item(&sdk_get_handle(0)->devobj[0].talkQueue,pmsg_out);
}
int  Audio_msg_Del_queue_first(void)
{
    return del_msg_queue_first(&sdk_get_handle(0)->devobj[0].talkQueue);
}
///////////////////////////////////////////////// Just Wrapper APIs ////////////////////////////////////////////////

