#ifndef PAGE_H
#define PAGE_H

#include <memory>
#include "MyDB_Table.h"
class MyDB_BufferManager;

class MyDB_Page {

public:
    bool dirty;
    bool pinned;
    bool anomalous;

    MyDB_TablePtr table;
    long pageNum; 
    char* pageData;
    off_t tempOffset;
    MyDB_BufferManager* bufferMgr;

    MyDB_Page(MyDB_TablePtr table, long pageNum, MyDB_BufferManager* mgr);
    ~MyDB_Page();
};

#endif