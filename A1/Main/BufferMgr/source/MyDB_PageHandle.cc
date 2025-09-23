#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C
#include "MyDB_PageHandle.h"
#include "MyDB_BufferManager.h"
#include "MyDB_Page.h"
#include <iostream>

void* MyDB_PageHandleBase :: getBytes () {
    page->bufferMgr->materialize(page.get());
    return page->pageData;
}

void MyDB_PageHandleBase :: wroteBytes () {
	page->dirty = true; 
}

MyDB_PageHandleBase :: MyDB_PageHandleBase (std::shared_ptr<MyDB_Page> page) : page(page) {
}

MyDB_PageHandleBase :: ~MyDB_PageHandleBase () {
}
#endif

