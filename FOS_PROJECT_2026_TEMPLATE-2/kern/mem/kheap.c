#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"
#include "../conc/ksemaphore.h"
#include <kern/cpu/cpu.h>
struct Area {
    uint32 start;      
    uint32 pages;      
    LIST_ENTRY(FreeArea) prev_next_info;
};
LIST_HEAD(FreeAreaList, Area);
LIST_HEAD(UsedAreaList, Area) ;
struct FreeAreaList free_areas;
uint32 allocSize[NUM_OF_KHEAP_PAGES];
uint32 frameToVA[NUM_OF_KHEAP_PAGES]; 
/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [0] GIVEN FUNCTIONS
//==============================================
// [1] INITIALIZE KERNEL HEAP:
//==============================================
void kheap_init()
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		buddy_init();
		set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
		kheapPageAllocStart = KERNEL_HEAP_START;
		kheapPageAllocBreak = KERNEL_HEAP_START;
	}
	//==================================================================================
	//==================================================================================
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
/**************************************************/
/* BLOCK ALLOCATOR */
/**************************************************/
void* create_buddy_space()
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [4] create_buddy_space
	//Your code is here
	/void buddy_page = kmalloc(PAGE_SIZE);
	if (buddy_page == NULL)
		return NULL;*/
	uint32 buddy_page = kheapPageAllocBreak; //pointer to the next free page in kernel heap
	if(buddy_page + PAGE_SIZE > KERNEL_HEAP_MAX)// check if the page is byond the krnel heap limit if yes return null
		return NULL;
	int status =alloc_page(ptr_page_directory ,buddy_page,PERM_WRITEABLE,1);
	if (status !=0 )
		return NULL;
	kheapPageAllocBreak = kheapPageAllocBreak + PAGE_SIZE; //update the break pointer to the next free page
	uint32 *page_table = NULL;
	if(get_page_table(ptr_page_directory,buddy_page,&page_table)==TABLE_NOT_EXIST){
		unmap_frame(ptr_page_directory,buddy_page);
		return NULL;
	}
	page_table[PTX(buddy_page)] |= BUDDY_SYS_FLAG;
	kheapPageAllocBreak += PAGE_SIZE;
return (void *)buddy_page;
	//Comment the following line
	// panic("create_buddy_space() not implemented");
}

void  delete_buddy_space(void *va)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [5] delete_buddy_space
	//Your code is here
	uint32 page_va = ROUNDDOWN((uint32)va,PAGE_SIZE);
	uint32 *page_table = NULL;
	struct FrameInfo *ptr_frame = get_frame_info(ptr_page_directory,page_va,&page_table);
	if(ptr_frame != NULL){
	page_table[PTX(page_va)] &= ~BUDDY_SYS_FLAG;
	unmap_frame(ptr_page_directory,page_va);
	if (page_va + PAGE_SIZE == kheapPageAllocBreak)
		kheapPageAllocBreak -= PAGE_SIZE;
	}
	//Comment the following line
	// panic("delete_buddy_space() not implemented");
}

/**************************************************/
/* PAGE ALLOCATOR */
/**************************************************/
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [6] kmalloc
	//Your code is here
	if (size == 0)
		return NULL;

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		return alloc_block(size);

	uint32 numOfPages = size / PAGE_SIZE;
	if (size % PAGE_SIZE != 0)
		numOfPages++;

    if(LIST_EMPTY(&free_areas) && (kheapPageAllocBreak != KERNEL_HEAP_MAX)){
			uint32 va = kheapPageAllocBreak;
			kheapPageAllocBreak = kheapPageAllocBreak + numOfPages * PAGE_SIZE;
	}

    LIST_FOREACH(struct FreeArea *a, &free_areas , prev_next_info){

		struct Area *biggest = NULL;
		if(a->pages == numOfpages){
			
			uint32 va = a->start;
			LIST_REMOVE(&free_areas, a);
		}
		else if(a->pages > numOfPages){
			if(biggest == NULL ){
				biggest = a;
			}
			else(biggest->pages < a->pages){
				biggest = a;
			}
			if (a == LIST_LAST(&free_areas)){
				uint32 va = biggest->start;
				LIST_REMOVE(&free_areas, biggest);
			}
		} else {
			uint32 va = kheapPageAllocBreak;
			kheapPageAllocBreak = kheapPageAllocBreak + numOfPages * PAGE_SIZE;
		}
	}

	for (uint32 i = 0; i < numOfPages; i++)
	{
		struct FrameInfo *frame_info = NULL;
		int ret = allocate_frame(&frame_info);
		if (ret == 0)
			ret = map_frame(ptr_page_directory, frame_info, va + i * PAGE_SIZE, PERM_WRITEABLE | PERM_PRESENT);
			frameToVA[to_frame_number(ptr_frame_info)] = va + i * PAGE_SIZE;
		if (ret != 0)
		{
			// undo the pages already mapped
			for (uint32 j = 0; j < i; j++)
				unmap_frame(ptr_page_directory, va + j * PAGE_SIZE);
			return NULL;
		}
	}
    allocSize[(va - KERNEL_HEAP_START) / PAGE_SIZE] = numOfPages;
	return (void*) va;
	//Comment the following line
	// kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #ALLOCATED PAGES) IS REQUIRED */
}


void kfree(void* virtual_address)
{
	struct FrameInfo *b;
	b = get_frame_info(ptr_page_directory, virtual_address, NULL);
	frameToVA[to_frame_number(b)] = 0;
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [7] kfree
	//Your code is here
	//Comment the following line
	// panic("kfree() not implemented");
  size = allocSize[((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE];
  uint32 end = (uint32)virtual_address + size * PAGE_SIZE;
  struct Area *behind = NULL;
  struct Area *infront = NULL;
  LIST_FOREACH(struct FreeArea *a, &free_areas, prev_next_info) {
	if (end == a->start) {
       infront = a;
	   LIST_REMOVE(&free_areas, a);
	} else if((a->start + (a->pages * PAGE_SIZE)) == (uint32)virtual_address){
		behind = a;
		LIST_REMOVE(&free_areas, a);
	}
  }
  if(behind == NULL){
	if(infront == NULL){
     for (int i = 0; i < numOfPages; i++)
    {
        unmap_frame(ptr_page_directory, va + i * PAGE_SIZE);
    }
     allocSize[((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE] = 0
	}
	else{
      infront->start = (uint32)virtual_address;
	  infront->pages += size;
	  LIST_INSERT_TAIL(&free_areas, infront);
	  	
     for (int i = 0; i < numOfPages; i++)
    {
        unmap_frame(ptr_page_directory, va + i * PAGE_SIZE);
    }
     allocSize[((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE] = 0
	
  }
	/*EFFICIENT IMPLEMENTATION ~O(#FREE BLOCKS + #FREED PAGES) IS REQUIRED */

}
else{
	if(infront == NULL){
     behind->pages += size;
	 LIST_INSERT_TAIL(&free_areas, behind);
     for (int i = 0; i < numOfPages; i++)
    {
        unmap_frame(ptr_page_directory, va + i * PAGE_SIZE);
    }
     allocSize[((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE] = 0
	}
	else{
     behind->pages += size + infront->pages;
	 LIST_INSERT_TAIL(&free_areas, behind);
	 for (int i = 0; i < numOfPages; i++)
	{
		unmap_frame(ptr_page_directory, va + i * PAGE_SIZE);
	}
	 allocSize[((uint32)virtual_address - KERNEL_HEAP_START) / PAGE_SIZE] = 0
	}
}
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [8] kheap_physical_address
	//Your code is here
	uint32 *page_table=NULL;
	struct FrameInfo *frame = get_frame_info(ptr_page_directory , virtual_address ,&page_table); 
	if(frame == NULL)
		return 0;
	uint32 offset = virtual_address & (PAGE_SIZE - 1);
	return to_physical_address(frame) + offset;
	//Comment the following line
	// panic("kheap_physical_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'26.GM#2] KERNEL HEAP - [9] kheap_virtual_address
	//Your code is here
	unsigned int frameNum = physical_address / PAGE_SIZE;
    unsigned int offset = physical_address % PAGE_SIZE;
	if (frameToVA[frameNum] == 0)
        return 0;   
		   return frameToVA[frameNum] + offset;
	//Comment the following line
	// panic("kheap_virtual_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().
void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - krealloc
	//Your code is here
	//Comment the following line
	panic("krealloc() is not implemented yet...!!");
}
