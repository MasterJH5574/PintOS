#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "devices/block.h"

/*Jiaxin Begin*/

/*Initialize swap when kernel starts*/
void swap_init(void);

/* Store a KPAGE to a swap slot
* return a block_sector_t index for later use
*/
block_sector_t swap_store(void *kpage);

/* Load a swap slot into a KPAGE
*  index should come from the return of a swap_store
*/
void swap_load(block_sector_t index, void *kpage);

/* Free a swap slot whose identifier is index 
*  which is obtained from swap_store
*/
void swap_free(block_sector_t index);

/*Jiaxin End*/

#endif /*vm/swap.h*/