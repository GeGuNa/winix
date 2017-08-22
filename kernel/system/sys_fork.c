#include "../winix.h"

/**
 * copy the pcb struct from parent to child
 * @param  parent 
 * @param  child  
 * @return        
 */
int copy_pcb(struct proc* parent, struct proc* child){
	int pbak;
	pbak = child->proc_nr;
	*child = *parent;
	child->proc_nr = pbak;
	child->ptable = child->protection_table;
	bitmap_clear(child->ptable, PTABLE_LEN);
	return OK;
}

/**
 * allocate new virtual address space for chlid, and 
 * copy the whole process image from parent to child
 * @param  parent 
 * @param  child  
 * @return        
 */
int copy_mm(struct proc* parent, struct proc* child){
	ptr_t *src, *dest;
	int j;

	child->rbase = dup_vm(parent,child);
	if(child->rbase == NULL)
		return ERR;

	src = (ptr_t *)parent->rbase;
	dest = (ptr_t *)child->rbase;
	while(src < parent->heap_bottom){
		copy_page(dest, src);
		src += PAGE_LEN;
		dest += PAGE_LEN;
	}
	return OK;
}


/**
 * copy physical registers. Physical registers are internally used by the kernel
 * Unlike virtual registers like $pc, $ra, some kernel space pointers point to the
 * actually physical memory for fast access
 * @param  parent 
 * @param  child  
 * @return        
 */
int copy_pregs(struct proc* parent, struct proc* child){
	ptr_t *sp;
	sp = get_physical_addr(parent->sp,parent);
	child->message = (struct message *)get_physical_addr(*( sp + 2 ), child);
	child->heap_break = get_physical_addr(get_virtual_addr(parent->heap_break, parent), child);
	child->heap_bottom = get_physical_addr(get_virtual_addr(parent->heap_bottom, parent), child);
	child->stack_top = get_physical_addr(get_virtual_addr(parent->stack_top, parent), child);
	return OK;
}




/**
 * syscall for fork
 * @param  parent 
 * @return        pid of the child, or -1 if forking is failed
 */
int sys_fork(struct proc *parent) {
	struct proc *child;
	// int tdb_page_len, sp_heap_page_len, page;

	if (child = get_free_proc_slot()) {
		copy_pcb(parent,child);

		if(copy_mm(parent,child) == ERR){
			free_slot(child);
			return ERR;
		}

		copy_pregs(parent,child);
		
		//Divide the quantum size between the parent and child
		//if quantum size is 1, quantum size is not changed
		if(parent->quantum != 1){
			child->quantum = (child->quantum + 1) / 2;
			parent->quantum /= 2;
		}

		child->parent = parent->proc_nr;
		return child->proc_nr;
	}
	return ERR;
}

int do_fork(struct proc *who, struct message *m){
	int child_pr;
	child_pr = sys_fork(who);
	
	if(child_pr == ERR)
		return EINVAL;
	
	m->i1 = 0;
	winix_send(child_pr,m);

	return child_pr;
}


