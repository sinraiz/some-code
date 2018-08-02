#pragma once
#include <Windows.h>

template<typename T>
class SList
{
public:
	SList()
	{
		// Let Windows initialize an SList head
		m_stack_head = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER),
			MEMORY_ALLOCATION_ALIGNMENT);
		InitializeSListHead(m_stack_head); //UPD: 22.05.2014, thx to @gridem
	}
	~SList()
	{
		clear();
		_aligned_free(m_stack_head);
	}
	bool push(const T& obj)
	{
		// Allocate an SList node
		node* pNode = alloc_node();
		if (!pNode)
			return false;
		// Call the object's copy constructor
		init_obj(&pNode->m_obj, obj);
		// Push the node into the stack
		InterlockedPushEntrySList(m_stack_head, &pNode->m_slist_entry);
		return true;
	}
	bool pop(T& obj)
	{
		// Pop an SList node from the stack
		node* pNode = (node*)InterlockedPopEntrySList(m_stack_head);
		if (!pNode)
			return false;
		// Retrieve the node's data
		obj = pNode->m_obj;
		// Call the destructor
		free_obj(&pNode->m_obj);
		// Free the node's memory
		free_node(pNode);
		return true;
	}
	void clear()
	{
		for (;;)
		{
			// Pop every SList node from the stack
			node* pNode = (node*)InterlockedPopEntrySList(m_stack_head);
			if (!pNode)
				break;
			// Call the destructor
			free_obj(&pNode->m_obj);
			// Free the node's memory
			free_node(pNode);
		}
	}
private:
	PSLIST_HEADER m_stack_head;
	struct node
	{
		// The SList entry must be the first field
		SLIST_ENTRY m_slist_entry;
		// User type follows
		T m_obj;
	};
	node* alloc_node()
	{
		return (node*)_aligned_malloc(sizeof(node), MEMORY_ALLOCATION_ALIGNMENT);
	}
	void free_node(node* pNode)
	{
		_aligned_free(pNode);
	}
	T* init_obj(T* p, const T& init)
	{
		return new (static_cast<void*>(p)) T(init);
	}
	void free_obj(T* p)
	{
		p->~T();
	}
};
