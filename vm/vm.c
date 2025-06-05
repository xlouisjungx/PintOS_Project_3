/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/threads/vaddr.h"

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

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
	}
err:
	return false;
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
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
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
	struct page *page = NULL;
	/* TODO: Fill this function */

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */

	return swap_in (page, frame->kva);
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
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}
