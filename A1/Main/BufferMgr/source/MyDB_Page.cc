#ifndef PAGE_C
#define PAGE_C

#include <memory>
#include "MyDB_Page.h"
#include "MyDB_Table.h"
#include "MyDB_BufferManager.h"

MyDB_Page::MyDB_Page(MyDB_TablePtr table, long pageNum, MyDB_BufferManager* mgr)
    : table(table), pageNum(pageNum), bufferMgr(mgr) {
    dirty = false; 
    pinned = false;
    anomalous = false;
    pageData = nullptr; 
    tempOffset = -1;
}

MyDB_Page::~MyDB_Page() {
    if (pageData != nullptr) {
        bufferMgr->cleanFrame(this);
    }
}

#endif