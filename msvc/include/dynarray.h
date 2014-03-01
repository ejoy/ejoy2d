#pragma once

#include <exception>
#include <limits>
#include <iterator>

namespace msvc {

	class bad_array_length : public std::bad_alloc
	{
	public:
		bad_array_length() throw()
		{ }

		virtual ~bad_array_length() throw() 
		{ }

		virtual const char* what() const throw()
		{ 
			return "std::bad_array_length"; 
		}
	};

	template <class T>
	class dynarray
	{
	public:
		typedef       T                               value_type;
		typedef       T&                              reference;
		typedef const T&                              const_reference;
		typedef       T*                              iterator;
		typedef const T*                              const_iterator;
		typedef std::reverse_iterator<iterator>       reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef size_t                                size_type;
		typedef ptrdiff_t                             difference_type;

		dynarray();
		const dynarray operator=(const dynarray&);

		explicit dynarray(size_type c)
			: store_(alloc(c))
			, count_(c)
		{ 
			size_type i = 0;
			try {
				for (i = 0; i < count_; ++i)
				{
					new (store_+i) T;
				}
			}
			catch (...) {
				for (; i > 0; --i)
					(store_+(i-1))->~T();
				throw;
			} 
		}

		dynarray(const dynarray& d)
			: store_(alloc(d.count_))
			, count_(d.count_)
		{ 
			try { 
				uninitialized_copy(d.begin(), d.end(), begin());
			}
			catch (...) { 
				delete[] store_; 
				throw; 
			} 
		}

		~dynarray()
		{
			for (size_type i = 0; i < count_; ++i)
			{
				(store_+i)->~T();
			}
			delete[] store_;
		}

		iterator               begin()                       { return store_; }
		const_iterator         begin()                 const { return store_; }
		const_iterator         cbegin()                const { return store_; }
		iterator               end()                         { return store_ + count_; }
		const_iterator         end()                   const { return store_ + count_; }
		const_iterator         cend()                  const { return store_ + count_; }
		reverse_iterator       rbegin()                      { return reverse_iterator(end()); }
		const_reverse_iterator rbegin()                const { return reverse_iterator(end()); }
		reverse_iterator       rend()                        { return reverse_iterator(begin()); }
		const_reverse_iterator rend()                  const { return reverse_iterator(begin()); }
		size_type              size()                  const { return count_; }
		size_type              max_size()              const { return count_; }
		bool                   empty()                 const { return count_ == 0; }
		//reference              operator[](size_type n)       { return store_[n]; }
		//const_reference        operator[](size_type n) const { return store_[n]; }
		operator T*            ()                            { return data(); }
		operator const T*      ()                      const { return data(); }
		reference              front()                       { return store_[0]; }
		const_reference        front()                 const { return store_[0]; }
		reference              back()                        { return store_[count_-1]; }
		const_reference        back()                  const { return store_[count_-1]; }
		const_reference        at(size_type n)         const { check(n); return store_[n]; }
		reference              at(size_type n)               { check(n); return store_[n]; }
		T*                     data()                        { return store_; }
		const T*               data()                  const { return store_; }
		void                   fill(const T& v)              { fill_n(begin(), size(), v); }
		
	private:
		T*        store_;
		size_type count_;

		void check(size_type n)
		{ 
			if (n >= count_)
			{
				throw out_of_range("dynarray"); 
			}
		}

		T* alloc(size_type n)
		{ 
			if (n > (std::numeric_limits<size_type>::max)()/sizeof(T))
			{
				throw bad_array_length();
			}
			return reinterpret_cast<T*>(new char[n*sizeof(T)]); 
		}

	};
}
