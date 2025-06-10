#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include <hash.h>

enum vm_type {
	/* page not initialized */
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page */
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)
#define MAX_STACK_SIZE (1 << 20)
/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. */
struct page {
	const struct page_operations *operations;
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */
	struct hash_elem hash_elem;
	
	bool writable;

	/* Your implementation *ß/

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit;
		struct anon_page anon;
		struct file_page file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

struct vm_load_arg
{
	struct file *file;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
};

/* The representation of "frame" */
struct frame {

	// 해당 프레임이 나타내는 물리 페이지에 접근하기 위한 커널 가상 주소
	void *kva;

	// 해당 프레임이 사용하는 struct page의 포인터 / 유저의 가상 페이지가 이 프레임을 통해 매핑됨
	struct page *page;
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	bool (*swap_in) (struct page *, void *);
	bool (*swap_out) (struct page *);
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. */
struct supplemental_page_table {
	struct hash hash;
};

#include "threads/thread.h"

// SPT 초기화 함수
void supplemental_page_table_init (struct supplemental_page_table *spt);

// src 프로세스의 SPT를 dst로 복사하는 함수
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);

// SPT를 정리하고 메모리를 해제하는 함수.
void supplemental_page_table_kill (struct supplemental_page_table *spt);

// 사용자 가상 주소 va에 해당하는 페이지가 SPT에 등록되어 있는지 찾고, 있다면 truct page *를 반환하는 함수
struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);

// 특정 페이지를 SPT에 삽입하는 함수
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);

// SPT에서 해당 페이지를 제거하고, 관련 리소스를 정리하는 함수
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

// 가상 메모리 서브시스템 전체를ㄹ 초기화하는 함수
void vm_init (void);

// 페이지 폴트 예외가 발생했을 때 이를 처리하려 시도하는 함수
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

// 주어진 사용자 주소 upage에 대해 페이지를 할당함
#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)

// SPT에 새 페이지를 할당하면서, Lazy Initialization를 위한 초기 화자 정보를 함께 설정하는 함수.
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);

// 물리 메모리 및 페이지를 해제하고 SPT에서도 제거하는 함수
void vm_dealloc_page (struct page *page);

// 해당 주소에 대한 페이지를 실제로 메모리에 로딩하여 사용할 수 있도록 하는 함수
bool vm_claim_page (void *va);

// 해당 페이지의 타입(익명, 파일 백업, stack 등)을 반환하는 함수
enum vm_type page_get_type (struct page *page);

// 인덱스 값을 계산하기 위한 함수
uint64_t hash_func(const struct hash_elem *e, void *aux);

// 두 해시 요소를 순서 비교하는 함수
bool hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);

void hash_page_destroy(struct hash_elem *e, void *aux);

#endif  /* VM_VM_H */
