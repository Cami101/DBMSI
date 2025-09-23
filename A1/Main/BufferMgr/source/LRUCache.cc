#include "LRUCache.h"
#include "MyDB_Page.h"
#include <stdexcept>
#include <iostream>

using namespace std;

LRUCache::Node::Node(MyDB_Page* p) : page(p), prev(nullptr), next(nullptr) {}

LRUCache::LRUCache() {
    dummyHead = new Node(nullptr);
    dummyTail = new Node(nullptr);
    dummyHead->next = dummyTail;
    dummyTail->prev = dummyHead;
}

LRUCache::~LRUCache() {
    Node* cur = dummyHead;
    while (cur) {
        Node* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
}

void LRUCache::remove(Node* n) {
    n->prev->next = n->next;
    n->next->prev = n->prev;
    n->prev = nullptr;
    n->next = nullptr;
}

void LRUCache::insertTail(Node* n) {
    Node* p = dummyTail->prev;
    p->next = n;
    n->prev = p;
    n->next = dummyTail;
    dummyTail->prev = n;
}

void LRUCache::makeMRU(MyDB_Page* page) {
    auto it = nodeMap.find(page);
    if (it == nodeMap.end()) {
        Node* n = new Node(page);
        insertTail(n);
        nodeMap[page] = n;
    } else {
        Node* n = it->second;
        remove(n);
        insertTail(n);
    }
}

void LRUCache::protect(MyDB_Page* page) {
    auto it = nodeMap.find(page);
    if (it != nodeMap.end()) {
        Node* n = it->second;
        nodeMap.erase(page);
        remove(n);
        delete n;
    }
}

MyDB_Page* LRUCache::evictLRU() {
    Node* n = dummyHead->next;
    if (n != dummyTail) {
        MyDB_Page* pg = n->page;
        nodeMap.erase(pg);
        remove(n);
        delete n;
        return pg;
    }
    cout << "WRONG!!!\n";
    return nullptr;
}