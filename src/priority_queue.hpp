#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 *
 * Implemented with a leftist heap so that merge runs in O(log n).
 * All mutating operations (push / pop / merge) are implemented in a
 * copy-then-swap fashion: the operation is first performed on a detached
 * copy, and only committed to the container if it completes without any
 * exception thrown by `Compare`. This provides the strong exception
 * guarantee required above.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct node {
		T data;
		node *left, *right;
		int dist; // rank of the leftist heap: length of the rightmost path to a null node
		node(const T &value) : data(value), left(nullptr), right(nullptr), dist(1) {}
	};

	node *root;
	size_t node_count;

	static int get_dist(const node *p) {
		return p == nullptr ? 0 : p->dist;
	}

	// Merge two leftist heaps and return the new root.
	// Strong guarantee: if Compare throws, both heaps are left untouched.
	static node *merge_nodes(node *a, node *b) {
		if (a == nullptr) return b;
		if (b == nullptr) return a;
		if (Compare()(a->data, b->data)) { // b is strictly greater: b should be the root
			node *t = a; a = b; b = t;
		}
		a->right = merge_nodes(a->right, b);
		if (get_dist(a->left) < get_dist(a->right)) {
			node *t = a->left; a->left = a->right; a->right = t;
		}
		a->dist = get_dist(a->right) + 1;
		return a;
	}

	// Deep copy of a subtree.
	static node *copy_nodes(const node *p) {
		if (p == nullptr) return nullptr;
		node *q = new node(p->data);
		try {
			q->left = copy_nodes(p->left);
			q->right = copy_nodes(p->right);
		} catch (...) {
			delete q;
			throw;
		}
		q->dist = p->dist;
		return q;
	}

	// Recursively destroy a subtree. The depth of a leftist heap is O(log n).
	static void destroy_nodes(node *p) {
		if (p == nullptr) return;
		destroy_nodes(p->left);
		destroy_nodes(p->right);
		delete p;
	}

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), node_count(0) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other)
		: root(copy_nodes(other.root)), node_count(other.node_count) {}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() {
		destroy_nodes(root);
	}

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		node *new_root = copy_nodes(other.root); // strong guarantee
		destroy_nodes(root);
		root = new_root;
		node_count = other.node_count;
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (root == nullptr) throw container_is_empty();
		return root->data;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		node *fresh = new node(e);
		try {
			node *new_root = merge_nodes(root, fresh);
			root = new_root;
		} catch (...) {
			delete fresh; // the original heap is untouched
			throw;
		}
		++node_count;
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (root == nullptr) throw container_is_empty();
		node *new_root = nullptr;
		try {
			new_root = merge_nodes(root->left, root->right);
		} catch (...) {
			// The subtrees are detached copies; merge_nodes leaves them
			// untouched when Compare throws, so *this is fully restored.
			throw;
		}
		node *old_root = root;
		root = new_root;
		--node_count;
		delete old_root;
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const {
		return node_count;
	}

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const {
		return node_count == 0;
	}

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other) {
			// Merging a queue with itself: the result equals the original,
			// which also satisfies "the other queue is cleared".
			destroy_nodes(root);
			root = nullptr;
			node_count = 0;
			return;
		}
		node *new_root = merge_nodes(root, other.root); // throws: both queues untouched
		root = new_root;
		node_count += other.node_count;
		other.root = nullptr;
		other.node_count = 0;
	}
};

}

#endif
