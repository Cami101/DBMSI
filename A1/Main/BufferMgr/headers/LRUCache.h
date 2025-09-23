#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <string>

class MyDB_Page;

using namespace std;

class LRUCache {
public:
    LRUCache();
    ~LRUCache();

    void makeMRU(MyDB_Page* page);

    MyDB_Page* evictLRU();

    void protect(MyDB_Page* page);

private:
    struct Node {
        MyDB_Page* page;
        Node* prev;
        Node* next;
        Node(MyDB_Page* page);
    };

    void remove(Node* node);
    void insertTail(Node* node);

    Node* dummyHead;
    Node* dummyTail;
    unordered_map<MyDB_Page*, Node*> nodeMap;
};

#endif