#include <InterCode.h>

InterCode head = NULL;
InterCode tail = NULL;

void insertInterCode(InterCode i) {
	i->prev = NULL;
	i->next = NULL;
	if(head) {
		tail->next = i;
		i->prev = tail;
		tail = i;
	} else {
		head = i;
		tail = i;
	}
}

void deleteInterCode(InterCode i) {

}

void printInterCode(void) {
	InterCode tmp = head;
	while(tmp) {
		switch(tmp->kind) {
			
		}
		printf("\n");
		tmp = tmp->next;
	}
}