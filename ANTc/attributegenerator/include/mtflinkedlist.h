/**
 * MOVE-TO-FRONT HEURISTIC LINKED LIST
 *
 * EVERYTIME WE UPDATE A FLOW WE MOVE IT TO THE HEAD OF THE LIST
 * THIS MEANS THAT REDUNDANT FLOWS ARE LEFT IN THE TAIL
**/
int mtflinkedlist_count = 0;

struct mtflinkedlist
{
	struct mtflinkedlistelement *head;
	struct mtflinkedlistelement *tail;
	int elementsin;
	int elementsout;
	int packetsseen;
} _mtflinkedlist;

struct mtflinkedlistelement
{
	struct mtflinkedlistelement *NEXT;
	struct mtflinkedlistelement *PREV;
	unsigned long ELEMENT;
} _mtflinkedlistelement;

int mtf_packets(struct mtflinkedlist *l)
{
	return l->packetsseen;
}

struct mtflinkedlistelement * 
makeelement(struct mtflinkedlist *list, struct mtflinkedlistelement *NEXT, struct mtflinkedlistelement *PREV, long x)
{
	struct mtflinkedlistelement *l = malloc(sizeof (struct mtflinkedlistelement));
	mtflinkedlist_count++;
	l->NEXT = NEXT;
	l->PREV = PREV;
	if (l->NEXT == l || l->PREV == l)
	{
		printf("SOMETHING WRONG\n");
		exit(1);
	}
	l->ELEMENT = x;
	list->elementsin++;
	return l;
}

void mtflinkedlist_print(struct mtflinkedlist *l)
	{
        struct mtflinkedlistelement *temp = l->head;
	printf("PRINTING LINKED LIST\n");
        while (temp != NULL)
        {

	    printf("%i\n",temp->ELEMENT);

            temp = temp->NEXT;
        }
	
	}

int mtflinkedlist_countin(struct mtflinkedlist *list)
{
	return list->elementsin;
}

int mtflinkedlist_countout(struct mtflinkedlist *list)
{
	return list->elementsout;
}


struct mtflinkedlist * mtflinkedlist_create()
{	
	struct mtflinkedlist *l;
	l = malloc(sizeof (struct mtflinkedlist));
	l->head = NULL;
	l->tail = NULL;
	l->elementsin = 0;
	l->elementsout = 0;
	l->packetsseen = 0;
	return l;
}


void mtflinkedlist_removehead(struct mtflinkedlist *l)
{
	struct mtflinkedlistelement *h = l->head;
//	printf("REMOVING HEAD\n");		
	// set the new head as the head
	l->head = l->head->NEXT;
	l->head->PREV = NULL;
	l->elementsout++;
	free(h);
	mtflinkedlist_count--;
}


void mtflinkedlist_emptylist(struct mtflinkedlist *l)
{
	struct mtflinkedlistelement *h = l->head;	
//	printf("REMOVING ONLY ELEMENT\n");		
	l->head = NULL;
	l->tail = NULL;
	l->elementsout++;
	free(h);
	mtflinkedlist_count--;
}

/**
 * Search the linked list for this element
 * If it is not in the list, add it to the front
 * If it is in the list, move it to the front
 **/
void mtflinkedlist_update(struct mtflinkedlist *l, unsigned long x)
{
	l->packetsseen++;
	// POPULATE AN EMPTY LIST
	if (l->head == NULL)
	{
		l->head = makeelement(l,NULL,NULL,x);
		l->tail = l->head;
		return;
	}

	// X is already at the head
	if (l->head->ELEMENT == x)
	{
		return;
	}
	
	// X is the tail
	if (l->tail->ELEMENT == x)
	{
		// assign temporary node to the tail
		struct mtflinkedlistelement *temp = l->tail;
		// set new tail as temp's previous
		l->tail = temp->PREV;
		l->tail->NEXT = NULL;
		// set temp's next as the head
		temp->NEXT = l->head;
		// set head's PREV as temp
		l->head->PREV = temp;
		// set temp as new head
		l->head = temp;
		temp->PREV = NULL; // temp has no PREV as it is the head
		return;
	}
		
	// ONE ELEMENT LIST
	if (l->head == l->tail)
	{
		if (l->head->ELEMENT != x)
		{
			// create a new element and set it as parent to current head
			struct mtflinkedlistelement *newe = makeelement(l,l->head,NULL,x);
			
			// set current head's parent to newe;
			l->head = newe;
			// set head as newe;
			l->head->NEXT->PREV = newe;
		}
		return; // we have updated or done nothing (x already head)
	}
	
	// LOOPING THROUGH THE WHOLE LIST
	struct mtflinkedlistelement *temp = l->head;
	struct mtflinkedlistelement *temp2;
	while (temp->NEXT != NULL)
	{
		temp2 = temp->NEXT;
		if (temp2->ELEMENT == x)
		{
			// move temp->NEXT to the head
			temp->NEXT = temp2->NEXT;
			temp2->NEXT->PREV = temp;
			// temp2 is no longer in linked list
			temp2->PREV = NULL;
			temp2->NEXT = l->head;
			l->head->PREV = temp2;
			l->head = temp2;
			return;
		}
		temp = temp->NEXT;
	}
	
	// IF WE GET HERE THEN WE HAVEN'T FOUND OUR X YET, ADD IT
	struct mtflinkedlistelement *newe = makeelement(l,l->head,NULL,x);
	l->head->PREV = newe;
	l->head = newe;
}


void mtflinkedlist_removetail(struct mtflinkedlist *l)
	{	
	mtflinkedlist_update(l,l->tail->ELEMENT);
	mtflinkedlist_removehead(l);
	return;
	struct mtflinkedlistelement * old_tail = l->tail;
	struct mtflinkedlistelement * new_tail = l->tail->PREV;
	l->tail = NULL;
	new_tail->NEXT = NULL;
	l->tail = new_tail;
	free(old_tail);
	}

void mtflinkedlist_remove(struct mtflinkedlist *l, long key)
{

	//printf("REMOVING\n");
	// key is in HEAD
	if (l->head == l->tail && l->head->ELEMENT == key)
	{
		mtflinkedlist_emptylist(l);
		return;
	}

	if (l->head != l->tail && l->head->ELEMENT == key)
	{
		mtflinkedlist_removehead(l);
		return;
	}

	// key is in TAIL
	if (l->tail->ELEMENT == key)
	{
		mtflinkedlist_removetail(l);
		return;
	}
	
//	return;

	// key is somewhere inside
	// LOOPING THROUGH THE WHOLE LIST
	 struct mtflinkedlistelement *temp = l->head;
	 struct mtflinkedlistelement *temp2;
	 while (temp->NEXT != NULL)
	 {
		 if (temp->NEXT->ELEMENT == key)
		 {
			 // printf("REMOVE SOMEWHERE IN THE LIST\n");
			 // move temp->NEXT to the head
			 temp2 = temp->NEXT;
			 temp->NEXT = temp2->NEXT;
			 temp->NEXT->PREV = temp;

			 // temp2 is no longer in linked list
			 free(temp2);
			 mtflinkedlist_count--;
			 l->elementsout++;
			 return;
		 }
		 temp = temp->NEXT;
	 }
	 //printf("SOMETHING WRONG");
	 //exit(1);
}
