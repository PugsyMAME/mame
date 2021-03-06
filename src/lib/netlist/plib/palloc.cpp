// license:GPL-2.0+
// copyright-holders:Couriersud
/*
 * palloc.c
 *
 */

#include "pconfig.h"
#include "palloc.h"
#include "pfmtlog.h"
#include "pexception.h"

#include <algorithm>

namespace plib {

//============================================================
//  Memory pool
//============================================================

mempool::mempool(size_t min_alloc, size_t min_align)
: m_min_alloc(min_alloc), m_min_align(min_align)
{
}
mempool::~mempool()
{
	for (auto & b : m_blocks)
	{
		if (b->m_num_alloc != 0)
		{
			fprintf(stderr, "Found block with %d dangling allocations\n", static_cast<int>(b->m_num_alloc));
		}
		::operator delete(b->data);
	}
	m_blocks.clear();
}

mempool::block * mempool::new_block()
{
	block *b = new block();
	b->data = static_cast<char *>(::operator new(m_min_alloc));
	b->cur_ptr = b->data;
	b->m_free = m_min_alloc;
	b->m_num_alloc = 0;
	m_blocks.push_back(b);
	return b;
}

size_t mempool::mininfosize()
{
	size_t sinfo = sizeof(mempool::info);
#ifdef __APPLE__
	size_t ma = 16;
#else
	size_t ma = 8;
#endif
	return ((std::max(m_min_align, sinfo) + ma - 1) / ma) * ma;
}

void *mempool::alloc(size_t size)
{
	size_t rs = (size + mininfosize() + m_min_align - 1) & ~(m_min_align - 1);
	for (auto &b : m_blocks)
	{
		if (b->m_free > rs)
		{
			b->m_free -= rs;
			b->m_num_alloc++;
			auto i = reinterpret_cast<info *>(b->cur_ptr);
			i->m_block = b;
			auto ret = reinterpret_cast<void *>(b->cur_ptr + mininfosize());
			b->cur_ptr += rs;
			return ret;
		}
	}
	{
		block *b = new_block();
		b->m_num_alloc = 1;
		b->m_free = m_min_alloc - rs;
		auto i = reinterpret_cast<info *>(b->cur_ptr);
		i->m_block = b;
		auto ret = reinterpret_cast<void *>(b->cur_ptr + mininfosize());
		b->cur_ptr += rs;
		return ret;
	}
}

void mempool::free(void *ptr)
{
	auto p = reinterpret_cast<char *>(ptr);

	auto i = reinterpret_cast<info *>(p - mininfosize());
	block *b = i->m_block;
	if (b->m_num_alloc == 0)
		plib::pexception("mempool::free - double free was called\n");
	else
	{
		//b->m_free = m_min_alloc;
		//b->cur_ptr = b->data;
	}
	b->m_num_alloc--;
}

}
