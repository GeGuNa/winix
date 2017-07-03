#include <winix/kernel.h>
/**
 * sends a message.
 *
 * Parameters:
 *   m				Pointer to write the message to.
 *
 * Returns:
 *   0 on success
 *   -1 if destination is invalid
 **/

int wini_send(int dest, message_t *m) {
	proc_t *pDest;

	current_proc->message = m; //save for later
	// kprintf("to %d from %d type %d| ",dest,current_proc->pid,m->type);
	//Is the destination valid?
	if (pDest = get_proc(dest)) {
		if(pDest->state == ZOMBIE || pDest->state == DEAD)
			return ERR;

		if (pDest->flags & RECEIVING) {
			*(pDest->message) = *m;
			//Unblock receiver
			pDest->flags &= ~RECEIVING;
			enqueue_head(ready_q[pDest->priority], pDest);
		}else {
			
			//Otherwise, block current process and add it to head of sending queue of the destination.
			current_proc->flags |= SENDING;
			current_proc->next_sender = pDest->sender_q;
			pDest->sender_q = current_proc;
			// kprintf(" | send t %d nexts %d senderh %d| ",m->type,current_proc->next_sender->pid,current_proc->sender_q->pid);
		}

		return OK;
	}
	return ERR;
}

/**
 * Receives a message.
 *
 * Parameters:
 *   m				Pointer to write the message to.
 *
 * Returns:			0
 **/
int wini_receive(message_t *m) {
	proc_t *p = current_proc->sender_q;

	//If a process is waiting to send to this process, deliver it immediately.
	if (p != NULL) {

		//Dequeue head node
		current_proc->sender_q = p->next_sender;

		//Copy message to this process
		*m = *(p->message);

		//Unblock sender
		p->flags &= ~SENDING;
		enqueue_head(ready_q[p->priority], p);
		//kprintf("$at receive sender %d enqueued || ",p->pid);
	}
	else {
		current_proc->message = m;
		current_proc->flags |= RECEIVING;
	}
	return OK;
}
/**
 * non-block send
 *
 * Parameters:
 *   m				Pointer to write the message to.
 *
 * Returns:
 *   0 on success
 *   -1 if destination is invalid
 **/
int wini_notify(int dest, message_t *m) {
	proc_t *pDest;

	current_proc->message = m; //save for later

	//Is the destination valid?
	if (pDest = get_proc(dest)) {

		//If destination is waiting, deliver message immediately.
		if (pDest->flags & RECEIVING) {

			//Copy message to destination
			*(pDest->message) = *m;

			//Unblock receiver
			pDest->flags &= ~RECEIVING;
			enqueue_head(ready_q[pDest->priority], pDest);
			//if the destination rejects any message it receives,
			//do not deliver the message, but add it to the scheduling queue
		} else {
			//do nothing
		}

		//do nothing if it's not waiting
		return OK;
	}
	return ERR;
}


int winix_send_err(int dest, message_t *m){
	memset(m,-1,MESSAGE_LEN);
	winix_send(dest,m);
}