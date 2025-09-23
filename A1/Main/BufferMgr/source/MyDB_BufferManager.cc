#ifndef BUFFER_MGR_C
#define BUFFER_MGR_C

#include "MyDB_BufferManager.h"
#include "LRUCache.h"
#include "MyDB_Page.h"

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
#include <cstring>


using namespace std;

MyDB_PageHandle MyDB_BufferManager::getPage(MyDB_TablePtr table, long pageNum) {
    string key = table->getName() + "_" + std::to_string(pageNum);

    // Hit
    auto it = lookUp.find(key);
    if (it != lookUp.end()) {
        if (auto sp = it->second.lock()) {
            return make_shared<MyDB_PageHandleBase>(sp);
        } else {
            // Stale entry → drop it and fall through to create fresh
            lookUp.erase(it);
        }
    }
    auto sp = make_shared<MyDB_Page>(table, pageNum, this);
    lookUp[key] = sp;
    return make_shared<MyDB_PageHandleBase>(sp);
}


MyDB_PageHandle MyDB_BufferManager :: getPage () {
	auto page = std::make_shared<MyDB_Page>(nullptr,  -1, this);
	page->anomalous = true;
    return make_shared<MyDB_PageHandleBase>(page);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage (MyDB_TablePtr table, long pageNum) {
	string key = table->getName() + "_" + std::to_string(pageNum);
    auto it = lookUp.find(key);
    if (it != lookUp.end()) {
        if (auto sp = it->second.lock()) {
            sp->pinned = true;
            lru->protect(sp.get()); 
            if (sp->pageData == nullptr) {
                char* frame = getAFrame();
                sp->pageData = frame;
            }
            return make_shared<MyDB_PageHandleBase>(sp);
        } else {
            lookUp.erase(it);
        }
    }
    auto sp = make_shared<MyDB_Page>(table, pageNum, this);
    sp->pinned = true;
    char* frame = getAFrame();
    sp->pageData = frame;
    lookUp[key] = sp;
    return make_shared<MyDB_PageHandleBase>(sp);
}

MyDB_PageHandle MyDB_BufferManager :: getPinnedPage () {
	auto page = make_shared<MyDB_Page>(nullptr,  -1, this);
	page->anomalous = true;
	page->pinned = true;
    char* frame = getAFrame();
    page->pageData = frame;
    return make_shared<MyDB_PageHandleBase>(page);
}

void MyDB_BufferManager :: unpin (MyDB_PageHandle unpinMe) {
    auto pg = unpinMe->page;
	pg->pinned = false;
    lru->makeMRU(pg.get());
}

// should allocate a frame for page
void MyDB_BufferManager :: materialize (MyDB_Page* page) {
    if (page->pageData == nullptr) {
        char* frame = getAFrame();                          // may evict someone
        page->pageData = frame;                             // <-- assign first

        if (page->anomalous) {
            if (page->tempOffset < 0) {
                std::memset(page->pageData, 0, pageSize);
            } else {
                ::pread(fd, page->pageData, pageSize, page->tempOffset);
            }
        } else {
            std::string path = page->table->getStorageLoc();
            int dbfd = ::open(path.c_str(), O_CREAT | O_RDWR | O_FSYNC, 0666);
            off_t off = (off_t)page->pageNum * (off_t)pageSize;
            ::pread(dbfd, page->pageData, pageSize, off);
            ::close(dbfd);
        }
    }               

    if (!page->pinned) {
        lru->makeMRU(page); 
    }
}

char* MyDB_BufferManager :: getAFrame () {
    if (freeFrames.empty()) {
        MyDB_Page* victim = lru->evictLRU();
        if (victim->dirty) {
            if (victim->anomalous) {\
                off_t off;
                if (victim->tempOffset < 0) {
                    off = accum * pageSize;
                    victim->tempOffset = off; 
                    accum += 1;
                } else {
                    off = victim->tempOffset;
                }
                ::pwrite(fd, victim->pageData, pageSize, off);
            } else {
                string path = victim->table->getStorageLoc();
                int dbfd = ::open(path.c_str(), O_CREAT | O_RDWR | O_FSYNC, 0666);
                off_t off = static_cast<off_t>(victim->pageNum) * static_cast<off_t>(pageSize);
                ::pwrite(dbfd, victim->pageData, pageSize, off);
                ::close(dbfd);
            }
            victim->dirty = false;
        }      
        freeFrames.push_back(victim->pageData);
	    victim->pageData = nullptr;
    }
    char* frame = freeFrames.back();
    freeFrames.pop_back();
    return frame;
}

// should free the frame attach to page
void MyDB_BufferManager :: cleanFrame (MyDB_Page* victim) {
    lru->protect(victim);
    if (victim->dirty) {
        if (victim->anomalous) {\
            off_t off;
            if (victim->tempOffset < 0) {
                off = accum * pageSize;
                victim->tempOffset = off; 
                accum += 1;
            } else {
                off = victim->tempOffset;
            }
            ::pwrite(fd, victim->pageData, pageSize, off);
        } else {
            string path = victim->table->getStorageLoc();
            int dbfd = ::open(path.c_str(), O_CREAT | O_RDWR | O_FSYNC, 0666);
            off_t off = static_cast<off_t>(victim->pageNum) * static_cast<off_t>(pageSize);
            ::pwrite(dbfd, victim->pageData, pageSize, off);
            ::close(dbfd);
        }
        victim->dirty = false;
    }      
    freeFrames.push_back(victim->pageData);
}

MyDB_BufferManager::MyDB_BufferManager(size_t pageSize, size_t numPages, string tempFile) : pageSize(pageSize), tempFile(std::move(tempFile)) {
	poolBytes = pageSize * numPages;
    baseAddr = new (std::nothrow) char[poolBytes];
    // open/create the temp file for anonymous pages
    fd = ::open(tempFile.c_str(), O_CREAT | O_RDWR | O_FSYNC, 0666);
    accum = 0;
    // available memory pointers in buffer
	for (size_t i = 0; i < numPages; i++) {
		char* framePtr = baseAddr + i * pageSize;
		freeFrames.push_back(framePtr);
	}
	// lru
	this->lru = new LRUCache();
}

MyDB_BufferManager :: ~MyDB_BufferManager () {
    if (fd > 0) {
        ::close(fd);
    }
	fd = -1;
	::unlink(tempFile.c_str());
	
	delete[] baseAddr;
    baseAddr = nullptr;
    poolBytes = 0;
	delete lru;
}
	
#endif