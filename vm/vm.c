/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "threads/mmu.h"
#include "kernel/hash.h"
// #include "vm/anon.h"
// #include "vm/file.h"

static struct hash_elem *find_va(struct hash *h, struct list *bucket, void *va);
unsigned page_hash(const struct hash_elem *p_, void *aux UNUSED);
bool page_less(const struct hash_elem *a_,const struct hash_elem *b_, void *aux UNUSED);
struct hash_elem * hash_insert_with_va (struct hash *h, struct hash_elem *new, void *va);

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void vm_init(void)
{
	vm_anon_init();
	vm_file_init();
#ifdef EFILESYS /* For project 4 */
	pagecache_init();
#endif
	register_inspect_intr();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type(struct page *page)
{
	int ty = VM_TYPE(page->operations->type);
	switch (ty)
	{
	case VM_UNINIT:
		return VM_TYPE(page->uninit.type);
	default:
		return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim(void);
static bool vm_do_claim_page(struct page *page);
static struct frame *vm_evict_frame(void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool vm_alloc_page_with_initializer(enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux)
{

	ASSERT(VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt, upage) == NULL)
	{
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		// struct page *p = vm_claim_page(upage);
		struct page *p= palloc_get_page(PAL_USER);
		if (VM_TYPE(type) == VM_ANON){
			uninit_new(p, NULL, init, type, aux, anon_initializer);
		}

		else if (VM_TYPE(type) == VM_FILE){
			uninit_new(p, NULL, init, type, aux, file_backed_initializer);
			/* TODO: Insert the page into the spt. */
		}

		pml4_set_writable(thread_current()->pml4,upage,writable); //set writable bit
		return vm_claim_page(upage);
	}
	err:
		return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page(struct supplemental_page_table *spt UNUSED, void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function. */
	uint64_t key = hash_bytes(&va, sizeof(va));
	struct hash_iterator i;

	// hash_first(&i, &spt->spt_hash);
	// while (hash_next(&i))
	// {
	// 	struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
	// 	if (p->va == va)
	// 	{
	// 		page = p;
	// 		break;
	// 	}
	// }
	// int idx = 0;
	// while (idx < key) {
	// 	hash_next(&i);
	// 	idx++;
	// }

	// struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
	// if (p->va == va) {
	// 	page = p;
	// }

	struct list *bucket = &spt->spt_hash.buckets[key];
	for (struct list_elem *e = list_begin(bucket); e != list_end(bucket); e = list_next(e))
	{
		struct hash_elem *he = list_elem_to_hash_elem(e);
		struct page *p = hash_entry(he, struct page, hash_elem);
		if (p->va == va)
			return p;
	}
	return page;
}

/* Insert PAGE into spt with validation. */
bool spt_insert_page(struct supplemental_page_table *spt UNUSED,
					 struct page *page UNUSED)
{
	bool succ = false;
	/* TODO: Fill this function. */
	// uint64_t key=hash_bytes(&page->va,sizeof(page->va));
	if (spt_find_page(spt, page->va) == NULL)
	{
		hash_insert_with_va(&spt->spt_hash, &page->hash_elem, &page->va);
		succ = true;
	}
	return succ;
}

void spt_remove_page(struct supplemental_page_table *spt, struct page *page)
{
	vm_dealloc_page(page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim(void)
{
	struct frame *victim = NULL;
	/* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame(void)
{
	struct frame *victim UNUSED = vm_get_victim();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame(void)
{
	// struct frame *frame = NULL;
	/* TODO: Fill this function. */
	struct frame *frame = palloc_get_page(PAL_USER);
	frame->kva = palloc_get_page(PAL_USER);
	frame->page = NULL;

	ASSERT(frame != NULL);
	ASSERT(frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth(void *addr UNUSED)
{
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp(struct page *page UNUSED)
{
}

/* Return true on success */
bool vm_try_handle_fault(struct intr_frame *f UNUSED, void *addr UNUSED,
						bool user UNUSED, bool write UNUSED, bool not_present UNUSED)
{
	struct supplemental_page_table *spt UNUSED = &thread_current()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	spt_find_page(spt,addr);

	return vm_do_claim_page(page);
	//uninut swap-in 함수(unin
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void vm_dealloc_page(struct page *page)
{
	destroy(page);
	free(page);
}

/* Claim the page that allocate on VA. */
bool vm_claim_page(void *va UNUSED)
{
	struct page *page = NULL;
	/* TODO: Fill this function */
	if (va == NULL)
		return false;
	if (page = spt_find_page(&thread_current()->spt, va) == NULL)
	{
		page = palloc_get_page(PAL_USER);
		page->va = va;
	}
	return vm_do_claim_page(page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page(struct page *page)
{
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;
	page->frame->kva = pml4_get_page(thread_current()->pml4, page->va);

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	// pml4에서 해당 pte를 갖고와라?
	if (!spt_insert_page(&thread_current()->spt, page))
	{
		return false;
	}

	return swap_in(page, frame->kva);
}

/* Initialize new supplemental page table */
void supplemental_page_table_init(struct supplemental_page_table *spt UNUSED)
{
	hash_init(&spt->spt_hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
// fork 시 부모 → 자식 SPT 복사
bool supplemental_page_table_copy(struct supplemental_page_table *dst UNUSED,
								  struct supplemental_page_table *src UNUSED)
{
	memcpy(&dst->spt_hash, &src->spt_hash, sizeof(struct supplemental_page_table));
}

/* Free the resource hold by the supplemental page table */
void supplemental_page_table_kill(struct supplemental_page_table *spt UNUSED)
{
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_destroy(&spt->spt_hash, &spt->spt_hash.hash);
}

static struct hash_elem *
find_va (struct hash *h, struct list *bucket, void *va) {
	struct list_elem *i;

	for (i = list_begin (bucket); i != list_end (bucket); i = list_next (i)) {
		struct hash_elem *hi = list_elem_to_hash_elem (i);
		// if (!h->less (hi, e, h->aux) && !h->less (e, hi, h->aux))
		// 	return hi;
		struct page *p = hash_entry(hi, struct page, hash_elem);
		if (p->va == va)
			return hi;
	}
	return NULL;
}

struct hash_elem *
hash_insert_with_va (struct hash *h, struct hash_elem *new, void *va) {
	// struct list *bucket = find_bucket (h, new);
	int key = hash_bytes(&va, sizeof(va)) % h->bucket_cnt;
	struct list *bucket = &h->buckets[key];
	struct hash_elem *old = find_va (h, bucket, va);

	if (old == NULL)
		insert_elem (h, bucket, new);

	rehash (h);

	return old;
}

/////////////////
unsigned
page_hash(const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry(p_, struct page, hash_elem);
	return hash_bytes(&p->va, sizeof p->va);
}

/* Returns true if page a precedes page b. */
bool page_less(const struct hash_elem *a_,
			   const struct hash_elem *b_, void *aux UNUSED)
{
	const struct page *a = hash_entry(a_, struct page, hash_elem);
	const struct page *b = hash_entry(b_, struct page, hash_elem);

	return a->va < b->va;
}

struct page *
page_lookup (const void *address) {
  struct page p;
  struct hash_elem *e;

  p.va = address;
  e = hash_find (&p, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}