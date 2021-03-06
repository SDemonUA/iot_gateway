#ifndef IOT_UTILS_H
#define IOT_UTILS_H

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define ECB_NO_LIBM
#include "ecb.h"

#define expect_false(cond) ecb_expect_false (cond)
#define expect_true(cond)  ecb_expect_true  (cond)

#include "uv.h"

//Functions to be used during serialization/deserialization to get fixed (little endian) byte order
#if ECB_LITTLE_ENDIAN
#define repack_uint64(v) (v)
#define repack_int64(v) (v)
#define repack_uint32(v) (v)
#define repack_int32(v) (v)
#define repack_uint16(v) (v)
#define repack_int16(v) (v)
#define repack_float(v) (v)
#define repack_double(v) (v)
#elif ECB_BIG_ENDIAN
#define repack_uint64(v) ecb_bswap64(v)
#define repack_int64(v) ((int64_t)ecb_bswap64(v))
#define repack_uint32(v) ecb_bswap32(v)
#define repack_int32(v) ((int32_t)ecb_bswap32(v))
#define repack_uint16(v) ecb_bswap16(v)
#define repack_int16(v) ((int16_t)ecb_bswap16(v))
ecb_inline ecb_const float repack_float(float v) {uint32_t u=ecb_bswap32(*(uint32_t*)((char*)&(v)));return *(float*)((char*)&u);}
ecb_inline ecb_const double repack_double(double v) {uint64_t u=ecb_bswap64(*(uint64_t*)((char*)&(v)));return *(double*)((char*)&u);}
#else
#error Byte order not constantly determined (TODO)
#endif

#ifdef _MSC_VER
	#define PACKED( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#elif defined(__GNUC__)
	#define PACKED( __Declaration__ ) __Declaration__ __attribute__((packed))
#endif
//Find correct event loop for specified thread
//Returns 0 on success and fills loop with correct pointer
//On error returns negative error code:
//IOT_ERROR_INVALID_ARGS - thread is unknown or loop is NULL
uv_loop_t* kapi_get_event_loop(uv_thread_t thread);


#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))


//on overflow UINT64_MAX is returned and errno is set to ERANGE 
inline uint64_t iot_strtou64(const char* str, char **endptr, int base) {
	errno=0;
#if ULONG_MAX == 0xFFFFFFFFUL
	return strtoull(str, endptr, base);
#else
	return strtoul(str, endptr, base);
#endif
}

//on overflow UINT32_MAX is returned and errno is set to ERANGE 
inline uint32_t iot_strtou32(const char* str, char **endptr, int base) {
	errno=0;
#if ULONG_MAX == 0xFFFFFFFFUL
	//32-bit system
	return strtoul(str, endptr, base);
#else
	//64-bit system
	uint64_t rval=strtoul(str, endptr, base);
	if(errno==ERANGE || rval>UINT32_MAX) {
		errno=ERANGE;
		rval=UINT32_MAX;
	}
	return rval;
#endif
}


//Uni-linked list without tail (NULL value of next field tells about EOL). headvar is of the same type as itemptr. Empty list has headvar==NULL
//Insert item with address itemptr at head of list. nextfld - name of field inside item struct for linking to next item
#define ULINKLIST_INSERTHEAD(itemptr, headvar, nextfld)	\
	do {												\
		(itemptr)->nextfld = headvar;					\
		headvar = itemptr;								\
	} while(0)

//removing of any element. itemptr CANNOT BE same expression as headvar
#define ULINKLIST_REMOVE(itemptr, headvar, nextfld)	\
	do {											\
		if((itemptr)==headvar) {					\
			headvar=(itemptr)->nextfld;				\
			(itemptr)->nextfld=NULL;				\
		} else {									\
			auto t=headvar;							\
			while(t->nextfld) {						\
				if(t->nextfld==(itemptr)) {			\
					t->nextfld=(itemptr)->nextfld;	\
					(itemptr)->nextfld=NULL;		\
					break;							\
				}									\
				t=t->nextfld;						\
			}										\
		}											\
	} while(0)

//version of remove without cleaning next field value. itemptr CANNOT BE same expression as headvar
#define ULINKLIST_REMOVE_NOCL(itemptr, headvar, nextfld)	\
	do {											\
		if((itemptr)==headvar) {					\
			headvar=(itemptr)->nextfld;				\
		} else {									\
			auto t=headvar;							\
			while(t->nextfld) {						\
				if(t->nextfld==(itemptr)) {			\
					t->nextfld=(itemptr)->nextfld;	\
					break;							\
				}									\
				t=t->nextfld;						\
			}										\
		}											\
	} while(0)

//removing of head element. must NOT be used without check that headvar is not NULL
#define ULINKLIST_REMOVEHEAD(headvar, nextfld)		\
	do {											\
		auto tmp=headvar;							\
		headvar=(headvar)->nextfld;					\
		tmp->nextfld=NULL;							\
	} while(0)

//version of head remove without cleaning next field value. must NOT be used without check that headvar is not NULL
#define ULINKLIST_REMOVEHEAD_NOCL(headvar, nextfld)	\
	do {											\
		headvar=(headvar)->nextfld;					\
	} while(0)


//Bi-linked list without tail (NULL value of next field tells about EOL, prev value of first item contains special mark and address of head variable)
//Have important features:
// - head pointer is NULL for empty list
// - prev field is NULL for disconnected items only;
// - can be quickly removed from parent list without necessity to explicitely keep any identification of parent list

//variant without cleaning next and prev pointers to be used just before inserting into another bi-linked list or freeing
#define BILINKLIST_REMOVE_NOCL(itemptr, nextfld, prevfld) \
	do {																									\
		if(!(itemptr)->prevfld) { /*disconnected item*/														\
			assert((itemptr)->nextfld==NULL); /*cannot have next*/											\
			break;																							\
		}																									\
		if((itemptr)->nextfld) (itemptr)->nextfld->prevfld=(itemptr)->prevfld;								\
		if(!(uintptr_t((itemptr)->prevfld) & 1)) (itemptr)->prevfld->nextfld=(itemptr)->nextfld;			\
		else { /*this is first item, so it contains address of head pointer to be updated*/					\
			assert(*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) == itemptr); /*head must point to itemptr*/	\
			*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) = (itemptr)->nextfld;							\
		}																									\
	} while(0)

//clears next and prev pointers after removal from list, so entry looks disconnected
#define BILINKLIST_REMOVE(itemptr, nextfld, prevfld) \
	do {																									\
		BILINKLIST_REMOVE_NOCL(itemptr, nextfld, prevfld);													\
		(itemptr)->prevfld = (itemptr)->nextfld = NULL;														\
	} while(0)

//adds item to the head of list
#define BILINKLIST_INSERTHEAD(itemptr, headvar, nextfld, prevfld) \
	do {																									\
		/*itemptr and address of headvar must be aligned to some non-zero power of 2 bytes*/				\
		assert((uintptr_t(itemptr) & 1)==0 && (uintptr_t(&(headvar)) & 1)==0);								\
		if(headvar) (headvar)->prevfld=itemptr;																\
		(itemptr)->nextfld=headvar;																			\
		*((void**)(&((itemptr)->prevfld)))=(void*)(uintptr_t(&(headvar)) | 1);								\
		headvar=itemptr;																					\
	} while(0)

//adds item after another specific non-NULL item
#define BILINKLIST_INSERTAFTER(itemptr, afteritemptr, nextfld, prevfld) \
	do {																									\
		if((afteritemptr)->nextfld) (afteritemptr)->nextfld->prevfld=itemptr;								\
		(itemptr)->nextfld=(afteritemptr)->nextfld;															\
		(itemptr)->prevfld=afteritemptr;																	\
		(afteritemptr)->nextfld=itemptr;																	\
	} while(0)



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//Bi-linked list WITH TAIL (prev value of first item contains special mark and address of head variable, next value of last item contains special mark and address of tail variable)
//Have important features:
// - head and tail pointers are NULL for empty list
// - prev or next field is NULL for disconnected items only;
// - can be quickly removed from parent list without necessity to explicitely keep any identification of parent list

//variant without cleaning next and prev pointers to be used just before inserting into another bi-linked list or freeing
#define BILINKLISTWT_REMOVE_NOCL(itemptr, nextfld, prevfld) \
	do {																									\
		if(!(itemptr)->nextfld || !(itemptr)->prevfld) { /*disconnected item*/								\
			assert((itemptr)->nextfld==(itemptr)->prevfld); /*check that both posinters are NULL*/			\
			break;																							\
		}																									\
		if(!(uintptr_t((itemptr)->nextfld) & 1)) (itemptr)->nextfld->prevfld=(itemptr)->prevfld;/*not last*/	\
		else { /*this is last item, so it contains address of tail pointer to be updated*/					\
			assert(*((void**)(uintptr_t((itemptr)->nextfld) ^ 1)) == itemptr);/*tail must point to itemptr*/	\
			if(uintptr_t((itemptr)->prevfld) & 1) { /*this is also first item, so list becomes empty*/		\
				assert(*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) == itemptr);/*head must point to itemptr*/\
				*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) = NULL;/*head becomes NULL*/					\
				*((void**)(uintptr_t((itemptr)->nextfld) ^ 1)) = NULL;/*tail becomes NULL*/					\
				break;																						\
			}																								\
			*((void**)(uintptr_t((itemptr)->nextfld) ^ 1)) = (itemptr)->prevfld;							\
		}																									\
		if(!(uintptr_t((itemptr)->prevfld) & 1)) (itemptr)->prevfld->nextfld=(itemptr)->nextfld;/*not first*/	\
		else { /*this is first item, so it contains address of head pointer to be updated*/					\
			assert(*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) == itemptr); /*head must point to itemptr*/	\
			*((void**)(uintptr_t((itemptr)->prevfld) ^ 1)) = (itemptr)->nextfld;							\
		}																									\
	} while(0)

//clears next and prev pointers after removal from list, so entry looks disconnected
#define BILINKLISTWT_REMOVE(itemptr, nextfld, prevfld) \
	do {																									\
		BILINKLISTWT_REMOVE_NOCL(itemptr, nextfld, prevfld);												\
		(itemptr)->prevfld = (itemptr)->nextfld = NULL;														\
	} while(0)

//adds item to the head of list
#define BILINKLISTWT_INSERTHEAD(itemptr, headvar, tailvar, nextfld, prevfld) \
	do {																									\
		/*itemptr and address of headvar and tailvar must be aligned to some non-zero power of 2 bytes*/	\
		assert((uintptr_t(itemptr) & 1)==0 && (uintptr_t(&(headvar)) & 1)==0 && (uintptr_t(&(tailvar)) & 1)==0);\
		if(headvar) { /*list is not empty*/																	\
			(headvar)->prevfld=itemptr;																		\
			(itemptr)->nextfld=headvar;																		\
		} else { /*list is empty, so tail must be assigned*/												\
			tailvar=itemptr;																					\
			*((void**)(&((itemptr)->nextfld)))=(void*)(uintptr_t(&(tailvar)) | 1);							\
		}																									\
		*((void**)(&((itemptr)->prevfld)))=(void*)(uintptr_t(&(headvar)) | 1);								\
		headvar=itemptr;																					\
	} while(0)

//adds item to the tail of list
#define BILINKLISTWT_INSERTTAIL(itemptr, headvar, tailvar, nextfld, prevfld) \
	do {																									\
		/*itemptr and address of headvar and tailvar must be aligned to some non-zero power of 2 bytes*/	\
		assert((uintptr_t(itemptr) & 1)==0 && (uintptr_t(&(headvar)) & 1)==0 && (uintptr_t(&(tailvar)) & 1)==0);\
		if(tailvar) { /*list is not empty*/																	\
			(tailvar)->nextfld=itemptr;																		\
			(itemptr)->prevfld=tailvar;																		\
		} else { /*list is empty, so head must be assigned*/												\
			headvar=itemptr;																					\
			*((void**)(&((itemptr)->prevfld)))=(void*)(uintptr_t(&(headvar)) | 1);							\
		}																									\
		*((void**)(&((itemptr)->nextfld)))=(void*)(uintptr_t(&(tailvar)) | 1);								\
		tailvar=itemptr;																					\
	} while(0)




//Bitmaps built of arrays of uint32_t
static inline uint32_t bitmap32_test_bit(const uint32_t* _map, unsigned _bit) {
	return _map[_bit>>5] & (1U << (_bit & (32-1)));
}
static inline void bitmap32_set_bit(uint32_t* _map, unsigned _bit) {
	_map[_bit>>5] |= (1U << (_bit & (32-1)));
}
static inline void bitmap32_clear_bit(uint32_t* _map, unsigned _bit) {
	_map[_bit>>5] &= ~(1U << (_bit & (32-1)));
}

/*
//Bitmaps built of arrays of uint16_t
static inline uint16_t bitmap16_test_bit(uint16_t* _map, unsigned _bit) {
	return _map[_bit>>4] & (1 << (_bit & (16-1)));
}
static inline void bitmap16_set_bit(uint16_t* _map, unsigned _bit) {
	_map[_bit>>4] |= (1 << (_bit & (16-1)));
}
static inline void bitmap16_clear_bit(uint16_t* _map, unsigned _bit) {
	_map[_bit>>4] &= ~(1 << (_bit & (16-1)));
}

//Bitmaps built of arrays of uint8_t
static inline uint8_t bitmap8_test_bit(uint8_t* _map, unsigned _bit) {
	return _map[_bit>>3] & (1U << (_bit & (8-1)));
}
static inline void bitmap8_set_bit(uint8_t* _map, unsigned _bit) {
	_map[_bit>>3] |= (1 << (_bit & (8-1)));
}
static inline void bitmap8_clear_bit(uint8_t* _map, unsigned _bit) {
	_map[_bit>>3] &= ~(1 << (_bit & (8-1)));
}
*/


//Class to manage chain of memory buffers and treat it like simple buffer
struct iot_membuf_chain;
struct iot_membuf_chain {
	iot_membuf_chain *next;
	uint32_t len, total_len; //len - size of useful space in this struct, total_len - sum of all 'len's in this and all following structs (connected with 'next')
	char buf[2]; //determines minimal allowed size of added buffer

	uint32_t init(uint32_t sz) { //sz - size of buffer on which this struct is put over
		if(sz<sizeof(iot_membuf_chain)) return 0;
		next=NULL;
		len=total_len=get_increment(sz);
		return len;
	}
	static unsigned int get_increment(uint32_t sz) { //for provided size of buffer returns increment of useful space
		if(sz<sizeof(iot_membuf_chain)) return 0;
		return sz-offsetof(iot_membuf_chain, buf);
	}
	uint32_t add_buf(char *newbuf, uint32_t sz) { //adds one more buffer to the end of chain. Operation is non-destructive for data in buf
		//returns 0 if provided buffer is too small or new total len otherwise
		iot_membuf_chain* n=(iot_membuf_chain *)newbuf;
		if(!n->init(sz)) return 0;
		uint32_t d=n->len;
		iot_membuf_chain* t=this;
		total_len+=d;
		while(t->next) {
			t=t->next;
			t->total_len+=d;
		}
		t->next=n;
		return total_len;
	}
	char *drop_nextbuf(void) {//removes next buf reference (thus decreasing chain length by 1), connecting its child to current struct as next. 
							//Destroys!!! data in buf if data length is larger than 'len'
		//returns removed buffer address or NULL if no next buffer and nothing was done
		if(!next) return NULL;
		iot_membuf_chain* t=next;
		next=t->next;
		total_len-=t->len;
		return (char*)t;
	}
	bool has_children(void) { //checks if there are any children
		return next!=NULL;
	}
	unsigned count_children(void) { //counts number of connected bufs
		unsigned c=0;
		iot_membuf_chain *p=next;
		while(p) {
			c++;
			p=p->next;
		}
		return c;
	}
};

//abstraction class over uv_timer_t
class iot_timer {
	uv_timer_t timer;
	uv_thread_t thread;
	void (*notify)(iot_timer*, void *);
	static void ontimer(uv_timer_t *w) {
		iot_timer* t=(iot_timer*)w->data;
		t->notify(t, t->param);
	}
public:
	void *param;
	iot_timer(void) {
		timer.loop=NULL;
		param=NULL;
		notify=NULL;
	}
	~iot_timer(void) {
		if(timer.loop && uv_is_active((uv_handle_t*)&timer)) { //inited and started
			stop();
		}
	}
	void init(uv_thread_t th=0) { // only in provided thread can start() and stop() be called. can be called several times if operating thread must be changed, but will assert if timer is still active in another thread
		if(!th) th=uv_thread_self();
		uv_loop_t *loop=kapi_get_event_loop(th);
		assert(loop!=NULL);

		if(timer.loop && uv_is_active((uv_handle_t*)&timer)) { //inited and started. hope thread is the same as was
			stop();
		}
		thread=th;
		uv_timer_init(loop, &timer);
		timer.data=this;
	}
	bool start(void (*notify_)(iot_timer*, void *), uint64_t timeout, uint64_t period=0) {
		if(!timer.loop || !notify_) return false;
		assert(thread==uv_thread_self());
		if(thread!=uv_thread_self()) return false;

		notify=notify_;
		uv_timer_start(&timer, iot_timer::ontimer, timeout, period);
		return true;
	}
	bool stop(void) {
		if(!timer.loop) return false;
		assert(thread==uv_thread_self());
		if(thread!=uv_thread_self()) return false;

		uv_timer_stop(&timer);
		return true;
	}
	bool is_active(void) {
		if(!timer.loop) return false;
		assert(thread==uv_thread_self());
		return uv_is_active((uv_handle_t*)&timer);
	}
};

class iot_atimer_item;
class iot_atimer_item {
	friend class iot_atimer;
	iot_atimer_item *next, *prev;
	uint64_t timesout; //corelates with result of uv_now, so is not unix time
public:
	void (*notify)(void *);
	void* param;

	iot_atimer_item(void* param_=NULL, void (*notify_)(void *)=NULL) : notify(notify_), param(param_) {
		next=prev=NULL;
	}
	~iot_atimer_item(void) {
		unschedule();
	}
	void init(void* param_, void (*notify_)(void *)) {
		notify=notify_;
		param=param_;
		next=prev=NULL;
	}
	void unschedule(void) { //should not be called in thread other than of corresponding iot_atimer object
		BILINKLISTWT_REMOVE(this, next, prev);
	}
	bool is_on(void) { //checks if timer is scheduled
		return next!=NULL;
	}
	uint64_t get_timeout(void) const {
		return timesout;
	}
};

//Action timer which works with uv library. Manages queue of EQUAL relative timer events (period is set during init) using minimal resources. Size of queue is unlimited
class iot_atimer {
	iot_atimer_item *head, *tail;
	uv_timer_t timer;
	uv_thread_t thread;
	uint64_t period; //zero if no timer inited
public:
	iot_atimer(void) {
		period=0;
	}
	~iot_atimer(void) {
		deinit();
	}
	void init(uint64_t period_, uv_loop_t *loop) {
		assert(period==0); //forbid double init
		assert(loop!=NULL);
		thread=uv_thread_self();

		if(period_==0) period_=1;
		head=tail=NULL;
		uv_timer_init(loop, &timer);
		timer.data=this;

		period=period_;
		uv_timer_start(&timer, iot_atimer::ontimer, period_, period_);
	}
	void set_thread(uv_thread_t thread_) {
		thread=thread_;
	}
	void deinit(void) {
		if(period) {
			while(head)
				head->unschedule(); //here head is moved to next item!!!
			uv_close((uv_handle_t*)&timer, NULL);
			period=0;
		}
	}
	static void ontimer(uv_timer_t *w) {
		iot_atimer* th=(iot_atimer*)w->data;

		uint64_t now=uv_now(w->loop);
		//loop through all items in queue while they seems to be timed out
		iot_atimer_item * cur;
		iot_atimer_item * &head=th->head;
		while(head && head->timesout <= now) {
			cur=head;
			cur->unschedule(); //here head is moved to next item!!!
			//signal about timeout
			cur->notify(cur->param);
		}
		if(head) { //restart timer
			uv_timer_set_repeat(w, head->timesout - now);
		} else {
			if(uv_timer_get_repeat(w)==th->period) return; //optimization actual for libuv to avoid unnecessary calls which already done before this callback
			uv_timer_set_repeat(w, th->period);
		}
		uv_timer_again(w);
	}
	void schedule(iot_atimer_item &it) {
		assert(thread==uv_thread_self());
		assert(period!=0);
		assert(it.notify!=NULL);
		if(expect_false(!it.notify)) return;

		it.unschedule();
		it.timesout=uv_now(timer.loop)+period;
		//add to the tail
		BILINKLISTWT_INSERTTAIL(&it, head, tail, next, prev);
	}
};

//Keeps references to allocated (using malloc()) memory blocks.
class mempool {
	void **memchunks=NULL; //array of allocated memory chunks
	int nummemchunks=0, maxmemchunks=0; //current quantity of chunks pointers in memchunks, number of allocated items in memchunks array

public:
	~mempool(void) {
		deinit();
	}
	void *allocate(uint32_t size, bool zero=false) { //allocate next chunk of memory
	//returns NULL on allocation error
		if(nummemchunks+1>maxmemchunks) { //reallocate memchunks array to make it bigger
			int newmax=nummemchunks+1+50;
			void **t=(void**)malloc(sizeof(void*)*newmax);
			if(!t) return NULL;
			if(nummemchunks>0) memmove(t, memchunks, sizeof(void*)*nummemchunks);
			if(memchunks) free(memchunks);
			memchunks=t;
			maxmemchunks=newmax;
		}
		void *t=malloc(size);
		if(!t) return NULL;
		memchunks[nummemchunks]=t;
		nummemchunks++;
		if(zero) memset(t, 0, size);
		return t;
	}
	void deinit(void) { //free all allocated chunks
		if(!memchunks) return;
		for(int i=0;i<nummemchunks;i++) free(memchunks[i]);
		free(memchunks);
		memchunks=NULL;
		nummemchunks=0;
		maxmemchunks=0;
	}
};


//allocate specified minimal quantity of items and add them to uni- or bidirectional list whose head is in 'ret' (updating 'ret' as necessary)
template<class Item, bool UsePrev=false, uint32_t OPTIMAL_BLOCK=256*1024, uint32_t MAX_BLOCK=2*1024*1024> inline bool alloc_free_items(mempool* mpool, uint32_t &n, uint32_t sz, Item * &ret, uint32_t maxn=0xFFFFFFFFu) {
	//'n' - minimal amount to be allocated for success. on exit it is updated to show quantity of allocated items
	//'sz' - size of each item (if Item is base class for allocated items then this size can be larger than sizeof(Item))
	//'ret' - head of allocated unidirectional list of items will be put here. if 'ret' already has pointer to unidirectional list, this list will be prepended
	//'maxn' - optional maximum quantity of allocated items
	//returns false if 'n' was not satisfied (but less structs can be allocated and returned with 'n' updated to show quantity of allocated)
		uint32_t perchunk;
		uint32_t nchunks;
		uint32_t chunksize;
		
		if(n>maxn) n=maxn;

		perchunk=OPTIMAL_BLOCK/sz;
		if(perchunk < n) {
			//optimal block is not enough for n items, try to take bigger chunk up to MAX_BLOCK
			chunksize=sz*n;
			if(chunksize>MAX_BLOCK && sz<MAX_BLOCK) { //avoid allocation chunks larger than MAX_BLOCK
				perchunk=MAX_BLOCK/sz;
				chunksize=perchunk*sz; //will be >OPTIMAL_BLOCK and <=MAX_BLOCK
				nchunks=(n+perchunk-1)/perchunk; //emulate integer ceil()
				//chunks must be recalculated after first allocation
			} else {
				nchunks=1;
				perchunk=n;
			}
		} else { //optimal block is enough for n and more
			nchunks=1;
			if(perchunk>maxn) perchunk=maxn;
			chunksize=perchunk*sz;
		}

		uint32_t n_good=0;
		while(nchunks>0) {
			char *t=(char*)mpool->allocate(chunksize);
			if(!t) { //malloc failure
				if(n_good>=n || perchunk<=1) break; //stop if minimum quantity reached or chunksize cannot be decreased
				perchunk>>=1; //decrease chunksize to have the half of items
				chunksize=perchunk*sz;
				nchunks=((n-n_good)+perchunk-1)/perchunk;
				continue;
			}
			Item *first=(Item *)t;
			if(UsePrev) {
				Item* prevt=NULL;
				for(uint32_t i=0;i<perchunk-1;i++) {
					((Item *)t)->init_allocated_item((Item *)(t+sz), prevt);
					prevt=(Item *)t;
					t+=sz;
				}
				((Item *)t)->init_allocated_item(ret, prevt);
				if(ret) ret->set_allocated_item_prev((Item *)t);
			} else {
				for(uint32_t i=0;i<perchunk-1;i++) {
					((Item *)t)->init_allocated_item((Item *)(t+sz));
					t+=sz;
				}
				((Item *)t)->init_allocated_item(ret);
			}
			ret=first;
			nchunks--;
			n_good+=perchunk;
			if(nchunks>0 && n_good+perchunk>maxn) { //another samesized chunk will be allocated which overflows maxn
				assert(n_good<=maxn); //problem with some math above
				perchunk=maxn-n_good;
				if(!perchunk) break; //maxn reached
				nchunks=1;
				chunksize=perchunk*sz;
			}
		}
		if(n_good>=n) {
			n=n_good;
			return true;
		}
		n=n_good;
		return false;
	}


#endif //IOT_UTILS_H