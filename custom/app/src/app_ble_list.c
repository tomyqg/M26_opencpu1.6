#include "ql_stdlib.h"
#include "ql_memory.h"
#include "app_ble_list.h"

bool SL_Creat(SList *p_list)
{

	*p_list = (SList)Ql_MEM_Alloc(sizeof(Node));
	if(*p_list==NULL)
		return FALSE;
	
	(*p_list)->next = NULL;
	return TRUE;
}

bool SL_Insert(SList list,Item item)
{
	PNode p,q;

	p = list;
	if(p==NULL)
		return FALSE;

	while(p->next != NULL)
	{
		p = p->next;
	}
	
	q = (Node *)Ql_MEM_Alloc(sizeof(Node));
	if(q!=NULL)
	{
		Ql_memcpy(&(q->item), &item, sizeof(UART_MSG));
		q->next = p->next;
		p->next = q;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool SL_GetItem(SList list,Item *p_item)
{
	PNode p;

	p = list->next;
	
	if(p==NULL)
	{
		return FALSE;
	}
	
	*p_item = p->item;
	return TRUE;
}

bool SL_Delete(SList list)
{
	PNode p,q;
	
	p = list;
	
	if(p==NULL || p->next==NULL)
		return FALSE;
	
	q = p->next;
	p->next = q->next;
	
	Ql_MEM_Free(q);
	return TRUE;
}

bool SL_Empty(SList list)
{
	PNode p;
	p = list;
	if(p->next == NULL)
		return TRUE;
	return FALSE;
}
