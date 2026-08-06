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
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T val;
		Node *left;
		Node *right;
		int dist; // length of the right spine (null path length)
		Node(const T &v, Node *l = nullptr, Node *r = nullptr, int d = 0)
			: val(v), left(l), right(r), dist(d) {}
	};

	// A minimal self-contained dynamic-array based stack.
	// Used instead of std::vector / std::stack so that this header only
	// relies on facilities already provided by the starter framework.
	// It is used to make tree destruction / copying iterative, so that
	// arbitrarily shaped (e.g. very unbalanced) trees never cause a stack
	// overflow through deep recursion.
	template<typename U>
	class Stack {
	private:
		U *data;
		size_t cap;
		size_t sz;
	public:
		Stack() : data(new U[8]), cap(8), sz(0) {}
		~Stack() { delete[] data; }
		Stack(const Stack &) = delete;
		Stack &operator=(const Stack &) = delete;
		void push(const U &v) {
			if (sz == cap) {
				size_t ncap = cap * 2;
				U *ndata = new U[ncap];
				for (size_t i = 0; i < sz; ++i) {
					ndata[i] = data[i];
				}
				delete[] data;
				data = ndata;
				cap = ncap;
			}
			data[sz++] = v;
		}
		void pop() { --sz; }
		U &top() { return data[sz - 1]; }
		bool empty() const { return sz == 0; }
	};

	// Plain aggregate (no user-declared special members) so that it stays
	// trivially copyable/assignable and works smoothly inside Stack<>.
	struct CopyTask {
		Node *src;
		Node *dst;
	};

	Node *root;
	size_t sz;
	Compare cmp;

	// Merges two leftist-heap trees rooted at a and b, returning the new root.
	//
	// Exception safety: the *only* operation here that can throw is the
	// comparator call `cmp(a->val, b->val)`. Every mutation of a node
	// (a->right, the left/right swap, a->dist) happens strictly *after* the
	// recursive call that might throw has already completed successfully:
	// C++ always fully evaluates the right hand side of an assignment
	// before performing the assignment, so `a->right = mergeNodes(a->right, b);`
	// will not modify `a` at all if the recursive call throws. This applies
	// transitively all the way down the recursion, so if the comparator
	// throws anywhere, none of the nodes in either input tree have been
	// modified, and the exception simply propagates upward leaving both
	// trees completely untouched.
	Node *mergeNodes(Node *a, Node *b) {
		if (!a) return b;
		if (!b) return a;
		if (cmp(a->val, b->val)) {
			Node *tmp = a;
			a = b;
			b = tmp;
		}
		Node *newRight = mergeNodes(a->right, b);
		a->right = newRight;
		int leftDist = a->left ? a->left->dist : -1;
		int rightDist = a->right ? a->right->dist : -1;
		if (leftDist < rightDist) {
			Node *t = a->left;
			a->left = a->right;
			a->right = t;
			rightDist = leftDist;
		}
		a->dist = rightDist + 1;
		return a;
	}

	static void destroyTree(Node *node) {
		if (!node) return;
		Stack<Node *> stk;
		stk.push(node);
		while (!stk.empty()) {
			Node *cur = stk.top();
			stk.pop();
			if (cur->left) stk.push(cur->left);
			if (cur->right) stk.push(cur->right);
			delete cur;
		}
	}

	// Deep-copies the tree rooted at src, iteratively. If allocation or T's
	// copy constructor throws partway through, everything already built is
	// cleaned up before the exception is rethrown, and src is never touched.
	static Node *copyTree(Node *src) {
		if (!src) return nullptr;
		Node *newRoot = new Node(src->val, nullptr, nullptr, src->dist);
		Stack<CopyTask> stk;
		CopyTask initTask;
		initTask.src = src;
		initTask.dst = newRoot;
		stk.push(initTask);
		try {
			while (!stk.empty()) {
				CopyTask cur = stk.top();
				stk.pop();
				Node *s = cur.src;
				Node *n = cur.dst;
				if (s->left) {
					Node *nl = new Node(s->left->val, nullptr, nullptr, s->left->dist);
					n->left = nl;
					CopyTask t;
					t.src = s->left;
					t.dst = nl;
					stk.push(t);
				}
				if (s->right) {
					Node *nr = new Node(s->right->val, nullptr, nullptr, s->right->dist);
					n->right = nr;
					CopyTask t;
					t.src = s->right;
					t.dst = nr;
					stk.push(t);
				}
			}
		} catch (...) {
			destroyTree(newRoot);
			throw;
		}
		return newRoot;
	}

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), sz(0) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(nullptr), sz(0) {
		root = copyTree(other.root);
		sz = other.sz;
	}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() {
		destroyTree(root);
	}

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		Node *newRoot = copyTree(other.root);
		destroyTree(root);
		root = newRoot;
		sz = other.sz;
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (!root) throw container_is_empty();
		return root->val;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		Node *newNode = new Node(e);
		try {
			root = mergeNodes(root, newNode);
		} catch (...) {
			delete newNode;
			throw;
		}
		++sz;
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (!root) throw container_is_empty();
		Node *newRoot = mergeNodes(root->left, root->right);
		Node *old = root;
		root = newRoot;
		old->left = nullptr;
		old->right = nullptr;
		delete old;
		--sz;
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const {
		return sz;
	}

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const {
		return sz == 0;
	}

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other) return;
		Node *newRoot = mergeNodes(root, other.root);
		root = newRoot;
		sz += other.sz;
		other.root = nullptr;
		other.sz = 0;
	}
};

}

#endif
