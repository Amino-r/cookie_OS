/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

/************************************************************************************/
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [0] GIVEN FUNCTIONS
//==================================
// [1] INITIALIZE BUDDY FREE LISTS:
//==================================
void buddy_init()
{
	//panic("buddy_init is not implemented yet");

	for (int l = BUDDY_LOWER_LEVEL; l <= BUDDY_UPPER_LEVEL; ++l)
	{
		LIST_INIT(&(freeBlockLists[l]));
	}
}
//====================================
// [2] PRINT SIZE OF FREE LISTS:
//====================================
void PrintFreeBlockLists()
{
	for (int i = BUDDY_LOWER_LEVEL; i <= BUDDY_UPPER_LEVEL; ++i)
	{
		cprintf("Level#%d: %d\n", i, LIST_SIZE(&freeBlockLists[i]));
	}
}
/************************************************************************************/

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] GET BLOCK SIZE:
//===========================
__inline__ uint32 get_block_size(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [1] get_block_size
	//Your code is here
	struct BlockMetaData *blockMD = (struct BlockMetaData *)((char *)va - META_DATA_SIZE);
	return BLOCK_SIZE(blockMD->level);
	//Comment the following line
	//panic("get_block_size() not implemented");

}
//===========================
// [2] ALLOCATE BLOCK:
//===========================
void *alloc_block(uint32 requiredSize)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [2] alloc_block
	//Your code is here

	/*
		actualRecuiredSize = requiredSize + META_DATA_SIZE
	edge cases : 
		actualRecuiredSize > BLOCK_SIZE(BUDDY_UPPER_LEVEL) or 0 ==> return NULL **potentially calls buddy space??
	from lower up to upper l  find blocksize(level) >= actualRecuiredSize.
		the implementation of that is to search the level itself 
	checking is by level not blocksize, if theres a free block return its address
	if not go to the next level until a free block is found (or loop ends)
	splitting to buddies untill the previous required level is reached 
	return the address of the block

		mathematics  & funcs
		 block size -> 2^level
		 the step up in the loop is by 2^i (level) 
		 free block check --> LIST_SIZE(&freeBlockLists[i])
		 splitting step
		 removing it from the free list (previous level) LIST_REMOVE(&freeBlockLists[i], block)

		 while i > level decrease i by 1 and split the block into two blocks of size BLOCK_SIZE i after decrease
		 and add them to the free list of level i then return the address of the right buddy block that im using as my block

		 calculate address of right buddy--> struct BlockMetaData *right_buddy = (struct BlockMetaData *)((char *)block + BLOCK_SIZE(i));
		 level of right buddy = i, is_free = 1, add to free list of level i using LIST_INSERT_HEAD(&freeBlockLists[i], right_buddy);
		  return the left buddy address ( + META_DATA_SIZE)

		 needed variables so far:
		 actualRequiredSize, level, i, block, right_buddy

	pseduo code
	 calculate actualRequiredSize = requiredSize + META_DATA_SIZE
	 if actualRequiredSize > BLOCK_SIZE(BUDDY_UPPER_LEVEL) or actualRequiredSize == 0 return NULL
	 find the level of the block that can fit this size, level = ceil(log2(actualRequiredSize))
	 for each level from level to BUDDY_UPPER_LEVEL
		if there is a free block in that level
			 split the block until we reach the required level
			return the address of the block
		else
			continue to the next level
		if we reach the end of the loop and we didnt find a free block, return NULL






	*/
	uint32 actualRequiredSize = requiredSize + META_DATA_SIZE;
	//check if the actualRequiredSize is greater than the maximum block size or equal to 0
	if ((actualRequiredSize > DYN_ALLOC_MAX_BLOCK_SIZE) || actualRequiredSize == 0)
	{
		return NULL;
	}
	else
	{
		int level = BUDDY_LOWER_LEVEL;
		while (BLOCK_SIZE(level) < actualRequiredSize)
	{
			level++;
		}
	for (int i = level; i <= BUDDY_UPPER_LEVEL; i++)
	{
		if (LIST_SIZE(&freeBlockLists[i]) > 0)
		{
			struct BlockMetaData *currentblock = LIST_FIRST(&freeBlockLists[i]);
			LIST_REMOVE(&freeBlockLists[i], currentblock);
			while (i > level)
			{
				i--;
				struct BlockMetaData *rightbuddy = (struct BlockMetaData *)((char *)currentblock + BLOCK_SIZE(i)); 
				rightbuddy->level = i;
				rightbuddy->is_free = 1;
				LIST_INSERT_HEAD(&freeBlockLists[i], rightbuddy);
				currentblock->level = i;
				currentblock->is_free = 1;
			}
			currentblock->is_free = 0;
			return (void *)((char *)currentblock + META_DATA_SIZE);
		}
	
	}
	return NULL;
}
	//Comment the following line
	//panic("alloc_block() not implemented");
}

//===================================================
// [3] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'26.GM#1] DYNAMIC ALLOCATOR - [3] free_block
	//Your code is here
	//Comment the following line
	//panic("free_block() not implemented");

	if(va == NULL){
		return; 
	}
	struct BlockMetaData *block;
	block = (struct BlockMetaData *)((char *)va - META_DATA_SIZE);
	if(block ->is_free)
			return;
	uint8 level = block ->level;
		block ->is_free = 1;
		LIST_INSERT_HEAD(&freeBlockLists[level], block);
		while(1)
		{
			uint32 size = BLOCK_SIZE(level);
			struct BlockMetaData *body;
		body = (struct BlockMetaData *) ((uint32)block ^ size );
		 if(!body ->is_free || body->level !=level)
			 break;
		 LIST_REMOVE(&freeBlockLists[level], block);
		 LIST_REMOVE(&freeBlockLists[level], body);
		 block ->is_free = 0;
		 body  ->is_free = 0;
		 if((uint32)body < (uint32)block)
			 block = body;
		 if (level == BUDDY_UPPER_LEVEL)
		        {
		            delete_buddy_space((void *)block);
		            return;
		        }

		        level++;

		        block->level = level;
		        block->is_free = 1;

		        LIST_INSERT_HEAD(&freeBlockLists[level], block);
			 

		}

}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//

//===========================
// [1] REALLOCATE BLOCK:
//===========================
void *realloc_block(void* va, uint32 newSize)
{
	//TODO: [PROJECT'26.BONUS#1] KERNEL REALLOC - realloc_block
	//Your code is here
	//Comment the following line
	panic("realloc_block() Not implemented yet");
}
