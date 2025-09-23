
#ifndef STACK_TEST_CC
#define STACK_TEST_CC

#include "QUnit.h"
#include "Stack.h"
#include <memory>
#include <iostream>
#include <vector>

int main () {

	QUnit::UnitTest qunit(std :: cerr, QUnit :: verbose);
	
	// UNIT TEST 1
	// just make sure we can create an empty stack, and that it is empty
	{
		
		Stack <int> myStack;
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);

	}

	// UNIT TEST 2
	// make sure we can push/pop a bunch of items
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		// now, pop everything off
		for (int i = 0; i < 10; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}
	}

	// UNIT TEST 3
	// make sure that the destructor works correctly... unless the stack correctly deallocates
	// all of the items it contains, this test will fail
	{
		static int temp = 0;
		struct Tester {
			Tester () {temp++;}
			~Tester () {temp--;}
		};

		{
			Stack <std :: shared_ptr <Tester>> myStack;
			std :: shared_ptr <Tester> myGuy = std :: make_shared <Tester> ();
			for (int i = 0; i < 10; i++) {
				myStack.push (myGuy);
			}
		}

		QUNIT_IS_EQUAL (temp, 0);
	}

	// UNIT TEST 4
	// make sure we can push/pop a bunch of items and then do it again
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 5; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}

		for (int i = 5; i < 20; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 20; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 19 - i);
		}
		
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);
	}

	// UNIT TEST 5
	// Test with strings
	{
		Stack<std::string> myStack;
		std::vector<std::string> words = {"hello", "world", "stack", "test"};
		
		for (auto& word : words) {
			myStack.push(word);
		}
		
		for (int i = words.size() - 1; i >= 0; i--) {
			QUNIT_IS_EQUAL(myStack.pop(), words[i]);
		}
	}

	// UNIT TEST 6
	// Push/pop vectors; make sure values survive copies/moves in nodes
	{
		Stack<std::vector<int>> s;
		s.push({1,2,3});
		s.push(std::vector<int>{4,5});
		s.push({6});

		auto a = s.pop();  // {6}
		QUNIT_IS_EQUAL(a.size(), (size_t)1);
		QUNIT_IS_EQUAL(a.back(), 6);

		auto b = s.pop();  // {4,5}
		QUNIT_IS_EQUAL(b.size(), (size_t)2);
		QUNIT_IS_EQUAL(b[0], 4);
		QUNIT_IS_EQUAL(b[1], 5);

		auto c = s.pop();  // {1,2,3}
		QUNIT_IS_EQUAL(c.size(), (size_t)3);
		QUNIT_IS_EQUAL(c[2], 3);
		QUNIT_IS_EQUAL(s.isEmpty(), true);
	}

	// UNIT TEST 7
    // push/pop a user-defined struct to ensure templates work with complex types
    {
        struct Point {
            int x, y;
            Point(int a = 0, int b = 0) : x(a), y(b) {}
            bool operator==(const Point& other) const {
                return x == other.x && y == other.y;
            }
        };

        Stack<Point> myStack;

        // Push a few points
        myStack.push(Point(1, 2));
        myStack.push(Point(3, 4));
        myStack.push(Point(5, 6));

        // Pop and check LIFO + equality
        Point p;
        p = myStack.pop();  QUNIT_IS_EQUAL(p == Point(5, 6), true);
        p = myStack.pop();  QUNIT_IS_EQUAL(p == Point(3, 4), true);
        p = myStack.pop();  QUNIT_IS_EQUAL(p == Point(1, 2), true);

        QUNIT_IS_EQUAL(myStack.isEmpty(), true);
    }

    // UNIT TEST 8
    // stress test: push a large number of items, interleave pops, then finish
    {
        Stack<int> myStack;

        // Push 0..999
        for (int i = 0; i < 1000; i++) {
            myStack.push(i);
        }
        QUNIT_IS_EQUAL(myStack.isEmpty(), false);

        // Pop 300 items: expect 999..700
        for (int i = 0; i < 300; i++) {
            QUNIT_IS_EQUAL(myStack.pop(), 999 - i);
        }

        // Push 1000..1499
        for (int i = 1000; i < 1500; i++) {
            myStack.push(i);
        }

        // Now pop everything and verify strict LIFO
        // Remaining stack top should go: 1499..1000
        for (int expected = 1499; expected >= 1000; expected--) {
            QUNIT_IS_EQUAL(myStack.pop(), expected);
        }

		// Now pop the rest: 699..0 (700 items)
		for (int expected = 699; expected >= 0; --expected) {
			QUNIT_IS_EQUAL(myStack.pop(), expected);
		}

        QUNIT_IS_EQUAL(myStack.isEmpty(), true);
    }

}

#endif

