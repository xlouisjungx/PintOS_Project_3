/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/threads/vaddr.h"
#include "threads/mmu.h"
#include "include/threads/thread.h"
#include <string.h>

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	void *va =pg_round_down(upage);

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) != NULL) return false;

	struct page *new_page = calloc(1, sizeof(struct page));

	if(new_page == NULL) return false;

	bool (*page_initializer)(struct page *, void *aux);
	switch (VM_TYPE(type)) {
		case VM_ANON:
		page_initializer = anon_initializer;
		break;
		case VM_FILE:
		page_initializer = file_backed_initializer;
		break;
		default:
		PANIC("Invalid VM type");
	}

	uninit_new(new_page, upage, init, type, aux, page_initializer);
	new_page->uninit.writable = writable;
	
	if(!spt_insert_page(spt, new_page)) {
		free(new_page);
		return false;
	}

	return true;
}

/* Find VA from spt and return page. On error, return NULL. */

// SPT에서 주어진 가상 주소 va에 해당하는 struct page를 검색하는 함수
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {

	va = pg_round_down(va);
	
	struct page p;
	p.va = va;

	/* TODO: Fill this function. */

	//hash_find (struct hash *h, struct hash_elem *e)
	struct hash_elem *e = hash_find(&spt->hash, &p.hash_elem);

	if(e == NULL) return NULL;

	return hash_entry(e, struct page, hash_elem);
}

/* Insert PAGE into spt with validation. */

// SPT에 중복된 주소가 없을 경우에만 struct page를 삽입하는 함수
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	if(spt_find_page(spt, page->va) != NULL) return false;

	//hash_insert (struct hash *, struct hash_elem *)
	
	if(hash_insert(&spt->hash, &page->hash_elem) == NULL) succ = true;

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/

// 사용자 공간 물리 프레임을 할당하고, 관리용 구조체까지 초기화하는 함수
static struct frame *
vm_get_frame (void) {

	/*
	
	vm_get_frame()
	│
	├── 1. palloc_get_page(PALLOC_USER)로 물리 메모리 할당
	│   └── 실패 시 PANIC("todo")
	│
	├── 2. struct frame * 구조체 동적 할당 (malloc)
	│   └── 실패 시 PANIC("todo") or ASSERT
	│
	├── 3. frame 구조체 멤버 초기화
	│    ├── frame->kva = palloc으로 받은 주소
	│    └── frame->page = NULL (처음엔 비워둠)
	│
	└── 4. frame 반환

	
	*/

	void *kva = palloc_get_page(PAL_USER);

	// 페이지를 얻지 못했다면, 커널 중단
	if(kva == NULL) PANIC("todo");

	struct frame *f = malloc(sizeof(struct frame));
	if(f == NULL) PANIC("todo");

	f->kva = kva;
	f->page = NULL;

	return f;
}

/* Growing the stack. */

// fault가 발생한 유저 스택 주소(addr)가 유효해지도록 새로운 페이지 할당
static void
vm_stack_growth (void *addr UNUSED) {
	// 1. 페이지 경계로 내림
	addr = pg_round_down(addr);

	// 2. SPT에 해당 주소가 이미 등록되어 있는지 확인
	if(spt_find_page(&thread_current()->spt, addr) != NULL) return;

	// 3. SPT에 익명 페이지로 등록 (lazy allocation)
	bool success = vm_alloc_page_with_initializer(VM_ANON, addr, true, NULL, NULL);

	// 4. 바로 물리 프레임 할당 및 매핑 (Claim)
	if(success) vm_claim_page(addr);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */

// 페이지 폴트가 발생했을 때, 이를 처리하려고 시도하는 함수
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {

	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
    struct page *page = NULL;
    if (addr == NULL)
        return false;

    if (is_kernel_vaddr(addr))
        return false;
	
    if (not_present) // 접근한 메모리의 physical page가 존재하지 않은 경우
    {
	
		//현재 스택
		void *rsp = user ? f->rsp : thread_current()->stack_pointer;

		/*
		
		왜 -32를 붙여줘야 하는가?
		-> x86_64 ABI에서는 함수 호출 시 최소 32바이트 이상의 공간을 미리 확보를 해야한다고 함.

		addr >= rsp - 32
		-> 페이지 폴트가 발생한 주소 addr이 현재 스택 포인터보다 너무 멀리 떨어져 있지 않아야 한다!
		
		*/

		void *stack_bottom_limit = USER_STACK - MAX_STACK_SIZE;

		if(addr >= rsp - 32 && addr >= stack_bottom_limit ) {
			vm_stack_growth(pg_round_down(addr));
			return true;
		}

        // /* TODO: Validate the fault */
        page = spt_find_page(spt, addr);
        if (page == NULL)
            return false;
        if (write == 1 && page->uninit.writable == 0) // write 불가능한 페이지에 write 요청한 경우
            return false;

        return vm_do_claim_page(page);
    }
    return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	/* TODO: Fill this function */
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt = &curr->spt;
	struct page *page = NULL;

	void *rounded_va = pg_round_down(va); 

	page = spt_find_page(spt, rounded_va);
	if(page == NULL) return false;
	

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {

	/*
	
	vm_do_claim_page(page)
	│
	├─> frame = vm_get_frame()
	│
	├─> frame->page = page
	│
	├─> page->frame = frame
	│
	├─> success = pml4_set_page(thread_current()->pml4, page->va, frame->kva, true)
	│
	└─> return success


	*/

	struct frame *frame = vm_get_frame ();

	if(frame == NULL) return false;

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	struct thread *curr = thread_current();

	//pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw)
	pml4_set_page(curr->pml4, page->va, frame->kva, page->uninit.writable);

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init (&spt->hash, hash_func, hash_less, NULL);
}

uint64_t hash_func(const struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, hash_elem);

	//hash_bytes(const void *buf_, size_t size);
	return hash_bytes(&page->va, sizeof(page->va));

}

bool hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page *page_1 = hash_entry(a, struct page, hash_elem);
	struct page *page_2 = hash_entry(b, struct page, hash_elem);

	//if(page_1->va > page_2->va) return page_1->va > page_2->va;
	// -> 불필요

	return page_1->va < page_2->va;
}


/* Copy supplemental page table from src to dst */

// 자식 프로세스를 위한 SPT를 복사하는 함수
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {

	struct hash_iterator i;
	hash_first(&i, &src->hash);
	while(hash_next(&i)) {
		struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		enum vm_type src_type = src_page->operations->type;

		if(src_type == VM_UNINIT) {
			vm_alloc_page_with_initializer(
				src_page->uninit.type,
				src_page->va,
				src_page->uninit.writable,
				src_page->uninit.init,
				src_page->uninit.aux
			);
		}

		else {
			if(vm_alloc_page(src_type, src_page->va, src_page->uninit.writable) && vm_claim_page(src_page->va)) {
				struct page *dst_page = spt_find_page(dst, src_page->va);
				memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
			}
		}
	}
	return true;
}

/* Free the resource hold by the supplemental page table */

// 프로세스가 종료될 떄 호출되며, SPT가 관리하는 모든 struct page들을 정리하는 함수
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	hash_clear(&spt->hash, hash_page_destroy);
	
}

void hash_page_destroy(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, hash_elem);
	destroy(page);
	free(page);
}
