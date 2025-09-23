#ifndef STACK_H
#define STACK_H

#include <stdexcept>
// this is the node class used to build up the LIFO stack
template <class Data>
class Node {

private:

	Data holdMe;
	Node* next;
	
public:

	Node(Data val, Node* nextNode = nullptr) : holdMe(val), next(nextNode) {}

	Data getVal() {
		return holdMe;
	}

	Node* getNext() {
		return next;
	}

	void setNext(Node* next) {
		this->next = next;
	}

};

// a simple LIFO stack
template <class Data> 
class Stack {

	Node<Data>* head;

public:

	// destroys the stack
	~Stack () {
		Node<Data>* curr = head;
		while (curr != nullptr) {
			Node<Data>* next = curr->getNext();
			delete curr;
			curr = next;
		}
	}

	// creates an empty stack
	Stack () {
		head = nullptr;
	}

	// adds pushMe to the top of the stack
	void push (Data val) {
		Node<Data>* newHead = new Node<Data>(val, head);
		head = newHead;
	}

	// return true if there are not any items in the stack
	bool isEmpty () {
		return head == nullptr;
	}

	// pops the item on the top of the stack off, returning it...
	// if the stack is empty, the behavior is undefined
	Data pop () {
		if (head == nullptr) {
			throw std::runtime_error("Cannot pop from empty stack");
		}
		Data val = head->getVal();
		Node<Data>* newHead = head->getNext();
		delete head;
		head = newHead;
		return val;
	}
};

#endif
