
#ifndef CATALOG_UNIT_H
#define CATALOG_UNIT_H

#include "MyDB_BufferManager.h"
#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"
#include "QUnit.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h> 

using namespace std;

// these functions write a bunch of characters to a string... used to produce data
void writeNums (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = '0' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeLetters (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = 'i' + (i % 10);
	}
	bytes[len - 1] = 0;
}

void writeSymbols (char *bytes, size_t len, int i) {
	for (size_t j = 0; j < len - 1; j++) {
		bytes[j] = '!' + (i % 10);
	}
	bytes[len - 1] = 0;
}

int main () {

	QUnit::UnitTest qunit(cerr, QUnit::verbose);

	// UNIT TEST 1: A BIG ONE!!
	{

		// create a buffer manager 
		MyDB_BufferManager myMgr (64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared <MyDB_Table> ("tempTable", "foobar");

		// allocate a pinned page
		cout << "allocating pinned page\n";
		MyDB_PageHandle pinnedPage = myMgr.getPinnedPage (table1, 0);
		char *bytes = (char *) pinnedPage->getBytes ();
		writeNums (bytes, 64, 0);
		pinnedPage->wroteBytes ();

		
		// create a bunch of pinned pages and remember them
		vector <MyDB_PageHandle> myHandles;
		for (int i = 1; i < 10; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage (table1, i);
			char *bytes = (char *) temp->getBytes ();
			writeNums (bytes, 64, i);
			temp->wroteBytes ();
			myHandles.push_back (temp);
		}

		// now forget the pages we created
		vector <MyDB_PageHandle> temp;
		myHandles = temp;

		// now remember 8 more pages
		for (int i = 0; i < 8; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle temp = myMgr.getPinnedPage (table1, i);
			char *bytes = (char *) temp->getBytes ();

			// write numbers at the 0th position
			if (i == 0)
				writeNums (bytes, 64, i);
			else
				writeSymbols (bytes, 64, i);
			temp->wroteBytes ();
			myHandles.push_back (temp);
		}

		// now correctly write nums at the 0th position
		cout << "allocating unpinned page\n";
		MyDB_PageHandle anotherDude = myMgr.getPage (table1, 0);
		bytes = (char *) anotherDude->getBytes ();
		writeSymbols (bytes, 64, 0);
		anotherDude->wroteBytes ();
		
		// now do 90 more pages, for which we forget the handle immediately
		for (int i = 10; i < 100; i++) {
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage (table1, i);
			char *bytes = (char *) temp->getBytes ();
			writeNums (bytes, 64, i);
			temp->wroteBytes ();
		}

		// now forget all of the pinned pages we were remembering
		vector <MyDB_PageHandle> temp2;
		myHandles = temp2;

		// now get a pair of pages and write them
		for (int i = 0; i < 100; i++) {
			cout << "allocating pinned page\n";
			MyDB_PageHandle oneHandle = myMgr.getPinnedPage ();
			char *bytes = (char *) oneHandle->getBytes ();
			writeNums (bytes, 64, i);
			oneHandle->wroteBytes ();
			cout << "allocating pinned page\n";
			MyDB_PageHandle twoHandle = myMgr.getPinnedPage ();
			writeNums (bytes, 64, i);
			twoHandle->wroteBytes ();
		}

		// make a second table
		MyDB_TablePtr table2 = make_shared <MyDB_Table> ("tempTable2", "barfoo");
		for (int i = 0; i < 100; i++) {
			cout << "allocating unpinned page\n";
			MyDB_PageHandle temp = myMgr.getPage (table2, i);
			char *bytes = (char *) temp->getBytes ();
			writeLetters (bytes, 64, i);
			temp->wroteBytes ();
		}
		
	}

	// UNIT TEST 2
	{
		MyDB_BufferManager myMgr (64, 16, "tempDSFSD");
		MyDB_TablePtr table1 = make_shared <MyDB_Table> ("tempTable", "foobar");

		// look up all of the pages, and make sure they have the correct numbers
		for (int i = 0; i < 100; i++) {
			MyDB_PageHandle temp = myMgr.getPage (table1, i);
			char answer[64];
			if (i < 8)
				writeSymbols (answer, 64, i);
			else
				writeNums (answer, 64, i);
			char *bytes = (char *) temp->getBytes ();
			QUNIT_IS_EQUAL (string (answer), string (bytes));
		}
	}


	
	// UNIT TEST 3: Anonymous page spills to temp file and is restored
	{
		// tiny pool to force eviction quickly
		MyDB_BufferManager bm(64, 2, "tempAnonUT4");

		// 1) Create one anonymous (no-table) page, write a known pattern
		MyDB_PageHandle h = bm.getPage();            // anomalous page
		char* p = (char*)h->getBytes();
		writeLetters(p, 64, 7);                      // deterministic content
		h->wroteBytes();

		// 2) Thrash the pool with more anonymous pages to force eviction of 'h'
		for (int i = 0; i < 6; ++i) {
			MyDB_PageHandle t = bm.getPage();        // more anomalous pages
			char* b = (char*)t->getBytes();
			writeNums(b, 64, i);                     // different content
			t->wroteBytes();
			// t goes out of scope each iteration (unpinned)
		}

		// 3) Access 'h' again; it should rematerialize from temp and match the original bytes
		char expect[64];
		writeLetters(expect, 64, 7);
		char* p2 = (char*)h->getBytes();
		QUNIT_IS_EQUAL(std::string(expect), std::string(p2));
	}

	// --- Micro E: Cross-table isolation (t1:nums, t2:letters) ---
	{
		MyDB_BufferManager mgr(64, 2, "tmpXTable");
		auto t1 = make_shared<MyDB_Table>("T1", "fileT1");
		auto t2 = make_shared<MyDB_Table>("T2", "fileT2");

		// Write different patterns to the same page number (0) in different tables
		{
			auto h1 = mgr.getPage(t1, 0);
			char* b1 = (char*)h1->getBytes(); writeNums(b1, 64, 3); h1->wroteBytes();
		}
		{
			auto h2 = mgr.getPage(t2, 0);
			char* b2 = (char*)h2->getBytes(); writeLetters(b2, 64, 5); h2->wroteBytes();
		}

		// Thrash a bit to force evictions
		for (int i = 1; i <= 4; ++i) {
			auto k1 = mgr.getPage(t1, i); (void)k1->getBytes(); k1->wroteBytes();
			auto k2 = mgr.getPage(t2, i); (void)k2->getBytes(); k2->wroteBytes();
		}

		// Reload and verify they did not collide
		{
			char exp1[64]; writeNums(exp1, 64, 3);
			auto r1 = mgr.getPage(t1, 0);
			QUNIT_IS_EQUAL(std::string(exp1), std::string((char*)r1->getBytes()));
		}
		{
			char exp2[64]; writeLetters(exp2, 64, 5);
			auto r2 = mgr.getPage(t2, 0);
			QUNIT_IS_EQUAL(std::string(exp2), std::string((char*)r2->getBytes()));
		}
		cerr << "[Micro E] Passed\n";
	}

	// --- Micro F: Pin removes from LRU; unpin reinserts; eviction after unpin ---
	{
		MyDB_BufferManager mgr(64, 2, "tmpPinUnpin");
		auto tbl = make_shared<MyDB_Table>("Tp", "filePinUnpin");

		// Start with one unpinned page (0)
		{
			auto u0 = mgr.getPage(tbl, 0);
			char* b0 = (char*)u0->getBytes(); writeSymbols(b0, 64, 9); u0->wroteBytes();
		}

		// Pin the same page → must not be in LRU now
		auto p0 = mgr.getPinnedPage(tbl, 0);
		(void)p0->getBytes(); p0->wroteBytes();

		// Bring in another unpinned page; pool is full but pinned(0) must not be evicted
		{
			auto u1 = mgr.getPage(tbl, 1);
			char* b1 = (char*)u1->getBytes(); writeNums(b1, 64, 1); u1->wroteBytes();
		}

		// Drop the pin (let handle go out of scope) → should become unpinned resident & re-enter LRU
		p0 = nullptr;

		// Force an eviction by bringing a third page; now page 0 or 1 may be victim depending on recency,
		// but either way eviction must succeed (no "all pinned"), and data must persist.
		{
			auto u2 = mgr.getPage(tbl, 2);
			char* b2 = (char*)u2->getBytes(); writeLetters(b2, 64, 2); u2->wroteBytes();
		}

		// Verify both page 0 and 1 contents are still correct after eviction churn
		{
			char e0[64]; writeSymbols(e0, 64, 9);
			auto r0 = mgr.getPage(tbl, 0);
			QUNIT_IS_EQUAL(std::string(e0), std::string((char*)r0->getBytes()));
			char e1[64]; writeNums(e1, 64, 1);
			auto r1 = mgr.getPage(tbl, 1);
			QUNIT_IS_EQUAL(std::string(e1), std::string((char*)r1->getBytes()));
		}
		cerr << "[Micro F] Passed\n";
	}

	// --- Micro A2: Pin drops to zero handles → becomes unpinned & evictable ---
	{
		MyDB_BufferManager bm(64, 2, "tmp_pin_drop");
		auto t = make_shared<MyDB_Table>("T", "file_pin_drop");

		// Pin page 0 and write
		MyDB_PageHandle h = bm.getPinnedPage(t, 0);
		{ char* b = (char*)h->getBytes(); writeNums(b, 64, 7); h->wroteBytes(); }

		// Drop last handle → should auto-unpin internally (per spec)
		h = nullptr;

		// Bring two other pages to force eviction of page 0 if unpinned
		{ auto p1 = bm.getPage(t, 1); (void)p1->getBytes(); p1->wroteBytes(); }
		{ auto p2 = bm.getPage(t, 2); (void)p2->getBytes(); p2->wroteBytes(); }

		// Reload page 0 and verify data persisted (i.e., it was evictable & wrote back)
		char exp[64]; writeNums(exp, 64, 7);
		auto r0 = bm.getPage(t, 0);
		QUNIT_IS_EQUAL(std::string(exp), std::string((char*)r0->getBytes()));
	}

	// --- Micro A4: Same page, multiple handles → single materialization ---
	{
		MyDB_BufferManager bm(64, 2, "tmp_single_mat");
		auto t = make_shared<MyDB_Table>("T", "file_single_mat");

		// First handle materializes & writes
		auto h1 = bm.getPage(t, 5);
		char* b1 = (char*)h1->getBytes(); writeLetters(b1, 64, 4); h1->wroteBytes();

		// Second handle to same page (no new materialize)
		auto h2 = bm.getPage(t, 5);
		char* b2 = (char*)h2->getBytes();

		// Both must see the same bytes
		char exp[64]; writeLetters(exp, 64, 4);
		QUNIT_IS_EQUAL(std::string(exp), std::string((char*)b1));
		QUNIT_IS_EQUAL(std::string(exp), std::string((char*)b2));

		// Drop h1; page should remain while h2 exists
		h1 = nullptr;
		QUNIT_IS_EQUAL(std::string(exp), std::string((char*)h2->getBytes()));
	}

	// --- Micro A5: Evict clean (no write), evict dirty (does write) ---
	{
		const char* F = "file_clean_dirty";
		::unlink(F); // ensure fresh file
		MyDB_BufferManager bm(64, 1, "tmp_cd");
		auto t = make_shared<MyDB_Table>("T", F);

		// (a) CLEAN: materialize page 0, DO NOT call wroteBytes(), then evict it
		{
			auto c0 = bm.getPage(t, 0);
			(void)c0->getBytes(); // read only
			// Evict by touching another page
			auto tmp = bm.getPage(t, 1); (void)tmp->getBytes(); tmp->wroteBytes();
		}
		// Reopen BM and ensure file at page 0 is still zeros
		{
			MyDB_BufferManager bm2(64, 1, "tmp_cd2");
			auto r0 = bm2.getPage(t, 0);
			char zeros[64]; memset(zeros, 0, 64);
			QUNIT_IS_EQUAL(std::string(zeros, 64), std::string((char*)r0->getBytes(), 64));
		}

		// (b) DIRTY: write to page 0, evict, reopen, verify persisted
		{
			auto d0 = bm.getPage(t, 0);
			char* b = (char*)d0->getBytes(); writeNums(b, 64, 6); d0->wroteBytes();
			auto tmp = bm.getPage(t, 1); (void)tmp->getBytes(); tmp->wroteBytes(); // evict 0
		}
		{
			MyDB_BufferManager bm3(64, 1, "tmp_cd3");
			auto r0 = bm3.getPage(t, 0);
			char exp[64]; writeNums(exp, 64, 6);
			QUNIT_IS_EQUAL(std::string(exp), std::string((char*)r0->getBytes()));
		}
	}

	// --- Micro G1: MRU bump on unpinned read ---
	{
		MyDB_BufferManager bm(64, 2, "tmp_mru_bump_plus");
		auto t = make_shared<MyDB_Table>("T", "file_mru_bump_plus");

		// Load two unpinned pages: 0 then 1
		auto h0 = bm.getPage(t, 0);
		char* p0 = (char*)h0->getBytes();         // frame for page 0
		writeNums(p0, 64, 0); h0->wroteBytes();

		auto h1 = bm.getPage(t, 1);
		char* p1 = (char*)h1->getBytes();         // frame for page 1
		writeSymbols(p1, 64, 1); h1->wroteBytes();

		// Touch page 0 to bump it to MRU (so page 1 becomes LRU)
		(void)h0->getBytes();

		// Bring in page 2 → must evict page 1 and reuse its frame
		auto h2 = bm.getPage(t, 2);
		char* p2 = (char*)h2->getBytes();         // frame selected for page 2
		writeLetters(p2, 64, 2); h2->wroteBytes();

		// Assert: page 2 reused page 1's old frame (strong evidence page 1 was evicted)
		QUNIT_IS_TRUE(p2 == p1);

		// Assert: page 0 was NOT evicted (same frame address and same bytes)
		char* p0_after = (char*)h0->getBytes();
		QUNIT_IS_TRUE(p0_after == p0);            // still resident in the same frame

		char exp0[64]; writeNums(exp0, 64, 0);
		QUNIT_IS_EQUAL(std::string(exp0), std::string(p0_after));
	}


	// --- Micro G2: Destructor flush persists data ---
	{
		const char* F = "file_bm_dtor_flush";
		::unlink(F);

		{
			MyDB_BufferManager bm(64, 2, "tmp_bm_dtor");
			auto t = make_shared<MyDB_Table>("T", F);
			auto h = bm.getPage(t, 7);
			char* b = (char*)h->getBytes(); writeLetters(b, 64, 2); h->wroteBytes();
			// bm goes out of scope here → should flush dirty page 7
		}

		// New BM instance should see the data persisted
		{
			MyDB_BufferManager bm2(64, 1, "tmp_bm_dtor2");
			auto t2 = make_shared<MyDB_Table>("T", F);
			auto r = bm2.getPage(t2, 7);
			char exp[64]; writeLetters(exp, 64, 2);
			QUNIT_IS_EQUAL(std::string(exp), std::string((char*)r->getBytes()));
		}
	}

	// --- Micro G4: Free-frame count stays consistent; no null frames ---
	{
		MyDB_BufferManager bm(64, 2, "tmp_ff_invariants");
		auto t = make_shared<MyDB_Table>("T", "file_ff");

		// Materialize two pages → freeFrames should hit 0 internally; third triggers eviction
		{ auto a = bm.getPage(t,0); (void)a->getBytes(); a->wroteBytes(); }
		{ auto b = bm.getPage(t,1); (void)b->getBytes(); b->wroteBytes(); }

		// Third page forces eviction; ensure it succeeds and does not use a nullptr frame
		auto c = bm.getPage(t,2);
		char* bc = (char*)c->getBytes();
		QUNIT_IS_TRUE(bc != nullptr);
	}

}

#endif
