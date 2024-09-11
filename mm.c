/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

// 매크로 정의 
#define WSIZE 4 // 워드, 헤더, 풋터 사이즈 (bytes)
#define DSIZE 8 // 더블워드 사이즈(bytes)
#define CHUNKSIZE (1<<12) // 힙 확장을 위한 기본 크기

#define MAX(x,y) ((x)>(y) ? (x) : (y)) // x,y 중 큰 값을 가진다.

#define PACK(size,alloc) ((size)|(alloc)) 
#define GET(p) (*(unsigned int*)(p)) // Read
#define PUT(p,val) (*(unsigned int*)(p)=(val))  // Write
#define GET_SIZE(p) (GET(p) & ~0x7) // 0x7를 2진수에서 역수를 취하면 11111000이 됨. 이것을 GET(p)를 통해 가져온 헤더 정보에 and 연산을 하면 block size만 가져올 수 있음
#define GET_ALLOC(p) (GET(p) & 0x1) // 위의 케이스와 반대로 00000001을 and해주면 헤더에서 가용여부만 가져올 수 있음

#define HDRP(bp) ((char*)(bp) - WSIZE) 
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))


/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "5team",
    /* First member's full name */
    "JINS",
    /* First member's email address */
    "JINS@GICK.COM",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* global variable & functions */
static char *heap_listp;    // 항상 prologue block을 가리키는 정적 전역 변수 설정

// 기본 함수 선언
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);

// 추가 함수 선언
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t newsize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   
/* create : 초기의 빈 heap 할당(mem_sbrk) */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0); // Alignment padding 생성
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // prologue header 생성
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); // prologue footer 생성
    PUT(heap_listp + (3*WSIZE), PACK(0,1)); // epilogue block header 생성
    heap_listp += (2*WSIZE); // prologue header와 footer 사이로 포인터를 옮김.

    // 힙을 CHUNKSIZE 만큼 확장하여 가용 블록을 생성
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    // bp = mem_sbrk(size) : size만큼 brk를 늘려주고, 이전 포인터(old_brk) 반환
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
	
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0)); 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); 
    return coalesce(bp);
}

void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }
	size_t asize; // 적정 블록 사이즈.
    size_t extendsize; // 
    char *bp;
    
    // 만약 용량을 요구하지 않을 경우 무시.
    if (size == 0)
    	return NULL;
    
    // overhead와 정렬 reqs를 포함한 사이즈.
    if (size <= DSIZE) 
    	asize = 2*DSIZE;
    else
    	asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
        
    // free list에서 딱 맞는 거 찾기.
    if ((bp = find_fit(asize)) != NULL){
    	place(bp, asize);
        return bp;
    }    
    // 딱 맞는거 못 찾았다면 메모리 더 갖고 오기.    
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    	return NULL;
    place(bp, asize);
    return bp;
    
    }

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    // 어느 시점에 있는 bp를 인자로 받음
    size_t size = GET_SIZE(HDRP(bp)); // bp의 주소를 가지고 헤더에 접근하여(HDRP) -> block의 사이즈를 얻음(GET_SIZE)
    PUT(HDRP(bp), PACK(size,0)); // header free -> 가용상태로 만들기
    PUT(FTRP(bp), PACK(size,0)); // footer free -> 가용상태로 만들기
    coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 이전 블록이 할당되었는지 확인 : bp의 포인터를 통해 이전 블록을 찾고, 이 이전블록의 footer를 통해 할당 여부를 확인한다.
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 다음 블록이 할당되었는지 확인 : bp의 포인터를 통해 다음 블록을 찾고, 이 다음블록의 header를 통해 할당 여부를 확인한다.
    size_t size = GET_SIZE(HDRP(bp)); // 현재 블록의 사이즈 확인

    // case 1 : 이전 블록과 다음 블록이 모두 할당된 케이스 -> 합칠 수 없음
    if (prev_alloc && next_alloc){
        return bp;
    }
    // case 2 : 이전 블록 : 할당 상태, 다음 블록 : 가용 상태 -> 다음 블록과 합침
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 다음 블록의 크기를 현재 size에 더해줘요.
        PUT(HDRP(bp), PACK(size, 0)); // header 갱신 (더 큰 크기로 PUT)
        PUT(FTRP(bp), PACK(size,0)); // footer 갱신
    }
    // case 3 : 이전 블록 : 가용 상태, 다음 블록 : 할당 상태 -> 이전 블록과 합침
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); // 이전 블록의 크기를 현재 size에 더해줘요.
        PUT(FTRP(bp), PACK(size,0)); // 현재 위치의 footer에 block size를 갱신해줌
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    // case 4 : 이전 블록과 다음 블록이 모두 가용 블록인 상태 -> 이전 및 다음 블록 모두 합칠 수 있다.
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전 블록 및 다음 블록의 크기를 현재 size에 더해줘요.
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *find_fit(size_t asize){
    void *bp;

    for (bp=heap_listp; GET_SIZE(HDRP(bp))>0; bp=NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    // 현재 할당할 수 있는 후보 가용 블록의 주소
    size_t csize = GET_SIZE(HDRP(bp));

    // 분할이 가능한 경우
    // => 남은 메모리가 최소한의 가용 블록을 만들 수 있는 4word(16byte)가 되느냐
    // => why? 적어도 16바이트 이상이어야 이용할 가능성이 있음 
    // header & footer: 1word씩, payload: 1word, 정렬 위한 padding: 1word = 4words
    if ((csize - asize) >= (2 * DSIZE))
    {
        // 앞의 블록은 할당 블록으로
        PUT(HDRP(bp), PACK(asize, 1));          // 헤더값 갱신
        PUT(FTRP(bp), PACK(asize, 1));          // 푸터값 갱신
        // 뒤의 블록은 가용 블록으로 분할한다.
        bp = NEXT_BLKP(bp);                     // 다음블록 위치로 bp 이동
        PUT(HDRP(bp), PACK(csize - asize, 0));  // 남은 가용 블록의 헤더 갱신
        PUT(FTRP(bp), PACK(csize - asize, 0));  // 남은 가용 블록의 푸터 갱신
    }
    // 분할할 수 없다면 남은 부분은 padding한다.
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
