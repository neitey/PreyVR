#ifndef _HH_STACK_
#define _HH_STACK_

/*
================
idListSortCompare<type>
================
*/
#ifdef __INTEL_COMPILER
// the intel compiler doesn't do the right thing here
template< class type >
ID_INLINE int hhStackSortCompare(const type *a, const type *b)
{
	assert(0);
	return 0;
}
#else
template< class type >
ID_INLINE int hhStackSortCompare(const type *a, const type *b)
{
	return *a - *b;
}
#endif

/*
================================================================================
 hhStack:
 List based stack
================================================================================
*/

template< class type >
class hhStack {
public:
							hhStack(int newgranularity=16);
							~hhStack<type>();
	type					Top(void);
	type					Pop(void);
	void					Push(type &object);
	bool					Empty(void);
	void					Clear(void);
	int				Num(void) const;									// returns number of elements in list
	int				NumAllocated(void) const;							// returns number of elements allocated for
	void			SetGranularity(int newgranularity);				// set new granularity
	int				GetGranularity(void) const;						// get the current granularity

	size_t			Allocated(void) const;							// returns total size of allocated memory
	size_t			Size(void) const;									// returns total size of allocated memory including size of list type
	size_t			MemoryUsed(void) const;							// returns size of the used elements in the list

	hhStack<type> &	operator=(const hhStack<type> &other);
	const type 	&operator[](int index) const;
	type 			&operator[](int index);

	void			Condense(void);									// resizes list to exactly the number of elements it contains
	void			Resize(int newsize);								// resizes list to the given number of elements
	void			Resize(int newsize, int newgranularity);			// resizes list and sets new granularity
	void			SetNum(int newnum, bool resize = true);			// set number of elements in list and resize to exactly this number if necessary
	void			AssureSize(int newSize);							// assure list has given number of elements, but leave them uninitialized
	void			AssureSize(int newSize, const type &initValue);	// assure list has given number of elements and initialize any new elements
	void			AssureSizeAlloc(int newSize, idPlaneSet::new_t *allocator);	// assure the pointer list has the given number of elements and allocate any new elements

	type 			*Ptr(void);										// returns a pointer to the list
	const type 	*Ptr(void) const;									// returns a pointer to the list
	type 			&Alloc(void);										// returns reference to a new data element at the end of the list
	int				Append(const type &obj);							// append element
	int				Append(const hhStack<type> &other);				// append list
	int				AddUnique(const type &obj);						// add unique element
	int				Insert(const type &obj, int index = 0);			// insert the element at the given index
	int				FindIndex(const type &obj) const;				// find the index for the given element
	type 			*Find(type const &obj) const;						// find pointer to the given element
	int				FindNull(void) const;								// find the index for the first NULL pointer in the list
	int				IndexOf(const type *obj) const;					// returns the index for the pointer to an element in the list
	bool			RemoveIndex(int index);							// remove the element at the given index
	bool			Remove(const type &obj);							// remove the element
	void			Sort(idPlaneSet::cmp_t *compare = (idPlaneSet::cmp_t *)&hhStackSortCompare<type>);
	void			SortSubSection(int startIndex, int endIndex, idPlaneSet::cmp_t *compare = (idPlaneSet::cmp_t *)&hhStackSortCompare<type>);
	void			Swap(hhStack<type> &other);						// swap the contents of the lists
	void			DeleteContents(bool clear);						// delete the contents of the list
#ifdef _RAVEN
	// gcc 4.0: see ListGame.h
	void			RemoveContents( bool clear );
	void                    RemoveNull();
// ddynerman: range remove
	bool			RemoveRange( int low, int high );
	int				TypeSize() const { return sizeof(type); }

// cdr : added Heap & Stack & Sort functionality
	void			StackAdd( const type & obj );						// add to the stack
	void			StackPop( void );									// remove from the stack
	type &			StackTop( void );									// get the top element of the stack
	void			HeapAdd( const type & obj );						// add to the heap, and resort
	void			HeapPop( void );									// pop off the top of the heap & resort
#endif

#ifdef _HUMANHEAD
	protected:
#else
private:
#endif
	int				num;
	int				size;
	int				granularity;
	type 			*list;
};

template< class type >
hhStack<type>::hhStack(int newgranularity) {
	SetGranularity(newgranularity);
}

template< class type >
hhStack<type>::~hhStack(void) {
}

template< class type >
ID_INLINE type hhStack<type>::Top(void) {
	assert(Num() > 0);

	return this->list[Num()-1];
}

template< class type >
ID_INLINE type hhStack<type>::Pop(void) {

	assert(Num() > 0);

	type obj = Top();
	SetNum(Num()-1, false);
	return obj;
}

template< class type >
ID_INLINE void hhStack<type>::Push(type &object) {
	Append(object);
}

template< class type >
ID_INLINE bool hhStack<type>::Empty(void) {
	return Num() == 0;
}

template< class type >
ID_INLINE void hhStack<type>::Clear(void) {
	if (list) {
		delete[] list;
	}

	list	= NULL;
	num		= 0;
	size	= 0;
}

/*
================
hhStack<type>::DeleteContents

Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
Note that this only works on lists containing pointers to objects and will cause a compiler error
if called with non-pointers.  Since the list was not responsible for allocating the object, it has
no information on whether the object still exists or not, so care must be taken to ensure that
the pointers are still valid when this function is called.  Function will set all pointers in the
list to NULL.
================
*/
template< class type >
ID_INLINE void hhStack<type>::DeleteContents(bool clear)
{
	int i;

	for (i = 0; i < num; i++) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if (clear) {
		Clear();
	} else {
		memset(list, 0, size * sizeof(type));
	}
}

/*
================
hhStack<type>::Allocated

return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t hhStack<type>::Allocated(void) const
{
	return size * sizeof(type);
}

/*
================
hhStack<type>::Size

return total size of list in bytes, but doesn't take into account additional memory allocated by type
================
*/
template< class type >
ID_INLINE size_t hhStack<type>::Size(void) const
{
	return sizeof(hhStack<type>) + Allocated();
}

/*
================
hhStack<type>::MemoryUsed
================
*/
template< class type >
ID_INLINE size_t hhStack<type>::MemoryUsed(void) const
{
	return num * sizeof(*list);
}

/*
================
hhStack<type>::Num

Returns the number of elements currently contained in the list.
Note that this is NOT an indication of the memory allocated.
================
*/
template< class type >
ID_INLINE int hhStack<type>::Num(void) const
{
	return num;
}

/*
================
hhStack<type>::NumAllocated

Returns the number of elements currently allocated for.
================
*/
template< class type >
ID_INLINE int hhStack<type>::NumAllocated(void) const
{
	return size;
}

/*
================
hhStack<type>::SetNum

Resize to the exact size specified irregardless of granularity
================
*/
template< class type >
ID_INLINE void hhStack<type>::SetNum(int newnum, bool resize)
{
	assert(newnum >= 0);

	if (resize || newnum > size) {
		Resize(newnum);
	}

	num = newnum;
}

/*
================
hhStack<type>::SetGranularity

Sets the base size of the array and resizes the array to match.
================
*/
template< class type >
ID_INLINE void hhStack<type>::SetGranularity(int newgranularity)
{
	int newsize;

	assert(newgranularity > 0);
	granularity = newgranularity;

	if (list) {
		// resize it to the closest level of granularity
		newsize = num + granularity - 1;
		newsize -= newsize % granularity;

		if (newsize != size) {
			Resize(newsize);
		}
	}
}

/*
================
hhStack<type>::GetGranularity

Get the current granularity.
================
*/
template< class type >
ID_INLINE int hhStack<type>::GetGranularity(void) const
{
	return granularity;
}

/*
================
hhStack<type>::Condense

Resizes the array to exactly the number of elements it contains or frees up memory if empty.
================
*/
template< class type >
ID_INLINE void hhStack<type>::Condense(void)
{
	if (list) {
		if (num) {
			Resize(num);
		} else {
			Clear();
		}
	}
}

/*
================
hhStack<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void hhStack<type>::Resize(int newsize)
{
	type	*temp;
	int		i;

	assert(newsize >= 0);

	// free up the list if no data is being reserved
	if (newsize <= 0) {
		Clear();
		return;
	}

	if (newsize == size) {
		// not changing the size, so just exit
		return;
	}

	temp	= list;
	size	= newsize;

	if (size < num) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];

	for (i = 0; i < num; i++) {
		list[ i ] = temp[ i ];
	}

	// delete the old list if it exists
	if (temp) {
		delete[] temp;
	}
}

/*
================
hhStack<type>::Resize

Allocates memory for the amount of elements requested while keeping the contents intact.
Contents are copied using their = operator so that data is correnctly instantiated.
================
*/
template< class type >
ID_INLINE void hhStack<type>::Resize(int newsize, int newgranularity)
{
	type	*temp;
	int		i;

	assert(newsize >= 0);

	assert(newgranularity > 0);
	granularity = newgranularity;

	// free up the list if no data is being reserved
	if (newsize <= 0) {
		Clear();
		return;
	}

	temp	= list;
	size	= newsize;

	if (size < num) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];

	for (i = 0; i < num; i++) {
		list[ i ] = temp[ i ];
	}

	// delete the old list if it exists
	if (temp) {
		delete[] temp;
	}
}

/*
================
hhStack<type>::AssureSize

Makes sure the list has at least the given number of elements.
================
*/
template< class type >
ID_INLINE void hhStack<type>::AssureSize(int newSize)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize(newSize);
	}

	num = newNum;
}

/*
================
hhStack<type>::AssureSize

Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
================
*/
template< class type >
ID_INLINE void hhStack<type>::AssureSize(int newSize, const type &initValue)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize(newSize);

		for (int i = num; i < newSize; i++) {
			list[i] = initValue;
		}
	}

	num = newNum;
}

/*
================
hhStack<type>::AssureSizeAlloc

Makes sure the list has at least the given number of elements and allocates any elements using the allocator.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE void hhStack<type>::AssureSizeAlloc(int newSize, idPlaneSet::new_t *allocator)
{
	int newNum = newSize;

	if (newSize > size) {

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize(newSize);

		for (int i = num; i < newSize; i++) {
			list[i] = (*allocator)();
		}
	}

	num = newNum;
}

/*
================
hhStack<type>::operator=

Copies the contents and size attributes of another list.
================
*/
template< class type >
ID_INLINE hhStack<type> &hhStack<type>::operator=(const hhStack<type> &other)
{
	int	i;

	Clear();

	num			= other.num;
	size		= other.size;
	granularity	= other.granularity;

	if (size) {
		list = new type[ size ];

		for (i = 0; i < num; i++) {
			list[ i ] = other.list[ i ];
		}
	}

	return *this;
}

/*
================
hhStack<type>::operator[] const

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE const type &hhStack<type>::operator[](int index) const
{
	assert(index >= 0);
	assert(index < num);

	return list[ index ];
}

/*
================
hhStack<type>::operator[]

Access operator.  Index must be within range or an assert will be issued in debug builds.
Release builds do no range checking.
================
*/
template< class type >
ID_INLINE type &hhStack<type>::operator[](int index)
{
	assert(index >= 0);
	assert(index < num);

	return list[ index ];
}

/*
================
hhStack<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
ID_INLINE type *hhStack<type>::Ptr(void)
{
	return list;
}

/*
================
hhStack<type>::Ptr

Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.

Note: may return NULL if the list is empty.

FIXME: Create an iterator template for this kind of thing.
================
*/
template< class type >
const ID_INLINE type *hhStack<type>::Ptr(void) const
{
	return list;
}

/*
================
hhStack<type>::Alloc

Returns a reference to a new data element at the end of the list.
================
*/
template< class type >
ID_INLINE type &hhStack<type>::Alloc(void)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		Resize(size + granularity);
	}

	return list[ num++ ];
}

/*
================
hhStack<type>::Append

Increases the size of the list by one element and copies the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int hhStack<type>::Append(type const &obj)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		int newsize;

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newsize = size + granularity;
		Resize(newsize - newsize % granularity);
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}


/*
================
hhStack<type>::Insert

Increases the size of the list by at leat one element if necessary
and inserts the supplied data into it.

Returns the index of the new element.
================
*/
template< class type >
ID_INLINE int hhStack<type>::Insert(type const &obj, int index)
{
	if (!list) {
		Resize(granularity);
	}

	if (num == size) {
		int newsize;

		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		newsize = size + granularity;
		Resize(newsize - newsize % granularity);
	}

	if (index < 0) {
		index = 0;
	} else if (index > num) {
		index = num;
	}

	for (int i = num; i > index; --i) {
		list[i] = list[i-1];
	}

	num++;
	list[index] = obj;
	return index;
}

/*
================
hhStack<type>::Append

adds the other list to this one

Returns the size of the new combined list
================
*/
template< class type >
ID_INLINE int hhStack<type>::Append(const hhStack<type> &other)
{
	if (!list) {
		if (granularity == 0) {	// this is a hack to fix our memset classes
			granularity = 16;
		}

		Resize(granularity);
	}

	int n = other.Num();

	for (int i = 0; i < n; i++) {
		Append(other[i]);
	}

	return Num();
}

/*
================
hhStack<type>::AddUnique

Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
================
*/
template< class type >
ID_INLINE int hhStack<type>::AddUnique(type const &obj)
{
	int index;

	index = FindIndex(obj);

	if (index < 0) {
		index = Append(obj);
	}

	return index;
}

/*
================
hhStack<type>::FindIndex

Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
================
*/
template< class type >
ID_INLINE int hhStack<type>::FindIndex(type const &obj) const
{
	int i;

	for (i = 0; i < num; i++) {
		if (list[ i ] == obj) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
hhStack<type>::Find

Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
================
*/
template< class type >
ID_INLINE type *hhStack<type>::Find(type const &obj) const
{
int i;

i = FindIndex(obj);

if (i >= 0) {
return &list[ i ];
}

return NULL;
}

/*
================
hhStack<type>::FindNull

Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.

NOTE: This function can only be called on lists containing pointers. Calling it
on non-pointer lists will cause a compiler error.
================
*/
template< class type >
ID_INLINE int hhStack<type>::FindNull(void) const
{
	int i;

	for (i = 0; i < num; i++) {
		if (list[ i ] == NULL) {
			return i;
		}
	}

	// Not found
	return -1;
}

/*
================
hhStack<type>::IndexOf

Takes a pointer to an element in the list and returns the index of the element.
This is NOT a guarantee that the object is really in the list.
Function will assert in debug builds if pointer is outside the bounds of the list,
but remains silent in release builds.
================
*/
template< class type >
ID_INLINE int hhStack<type>::IndexOf(type const *objptr) const
{
	int index;

	index = objptr - list;

	assert(index >= 0);
	assert(index < num);

	return index;
}

/*
================
hhStack<type>::RemoveIndex

Removes the element at the specified index and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool hhStack<type>::RemoveIndex(int index)
{
	int i;

	assert(list != NULL);
	assert(index >= 0);
	assert(index < num);

	if ((index < 0) || (index >= num)) {
		return false;
	}

	num--;

	for (i = index; i < num; i++) {
		list[ i ] = list[ i + 1 ];
	}

	return true;
}

/*
================
hhStack<type>::Remove

Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
================
*/
template< class type >
ID_INLINE bool hhStack<type>::Remove(type const &obj)
{
	int index;

	index = FindIndex(obj);

	if (index >= 0) {
		return RemoveIndex(index);
	}

	return false;
}

/*
================
hhStack<type>::Sort

Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
list, so any pointers to data within the list may no longer be valid.
================
*/
template< class type >
ID_INLINE void hhStack<type>::Sort(idPlaneSet::cmp_t *compare)
{
	if (!list) {
		return;
	}

	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)list, (size_t)num, sizeof(type), vCompare);
}

/*
================
hhStack<type>::SortSubSection

Sorts a subsection of the list.
================
*/
template< class type >
ID_INLINE void hhStack<type>::SortSubSection(int startIndex, int endIndex, idPlaneSet::cmp_t *compare)
{
	if (!list) {
		return;
	}

	if (startIndex < 0) {
		startIndex = 0;
	}

	if (endIndex >= num) {
		endIndex = num - 1;
	}

	if (startIndex >= endIndex) {
		return;
	}

	typedef int cmp_c(const void *, const void *);

	cmp_c *vCompare = (cmp_c *)compare;
	qsort((void *)(&list[startIndex]), (size_t)(endIndex - startIndex + 1), sizeof(type), vCompare);
}

/*
================
hhStack<type>::Swap

Swaps the contents of two lists
================
*/
template< class type >
ID_INLINE void hhStack<type>::Swap(hhStack<type> &other)
{
	idSwap(num, other.num);
	idSwap(size, other.size);
	idSwap(granularity, other.granularity);
	idSwap(list, other.list);
}

#ifdef _RAVEN

/*
================
hhStack<type>::StackAdd

Adds a value to the list as if the list was a stack

================
*/
template< class type >
ID_INLINE void hhStack<type>::StackAdd( const type & obj ) {
	Append( obj );
}

/*
================
hhStack<type>::StackPop

Removes a value to the list as if the list was a stack

================
*/
template< class type >
ID_INLINE void hhStack<type>::StackPop( void ) {
#if 0
	assert(num>0);
	if (num<=0)
	{
		return;
	}

// RAVEN BEGIN
// jsinger: Without this, this container will leak any dynamically allocated members from
//          contained class instances
// TTimo: that would cause a double destructor when the list is deleted in Clear()
	list[num].~type();
// RAVEN END
	num--;
#else
	RemoveIndex( num - 1 );
#endif
}

template< class type >
ID_INLINE type& hhStack<type>::StackTop( void ){
	assert( num > 0 );
	return list[ num-1 ];
}

/*
================
hhStack<type>::HeapAdd

Adds a value to the list as if the list was a heap by doing a bubble swap sort
First it appends the object, then swaps up the heap at successive parent positions
as needed

Complexity: O[n log n]
================
*/
template< class type >
ID_INLINE void hhStack<type>::HeapAdd( const type & obj ) {

	int pos = Append( obj );

	while (pos && list[((pos-1)/2)]<list[pos])
	{
		idSwap(list[((pos-1)/2)], list[pos]);
		pos = ((pos-1)/2);
	}
}

/*
================
hhStack<type>::HeapPop

Removes the top element from the heap and
First swaps the top element of the heap with the lowest
element, destroys the lowest element (wich was the top),
and then sorts the new top element down the heap as
needed

Complexity: O[n log n]
================
*/
template< class type >
ID_INLINE void hhStack<type>::HeapPop( void ) {

	assert(num>0);
	if (num<=0)
	{
		return;
	}

	idSwap(list[0], list[num-1]);
	num--;

	int pos = 0;

	int largestChild = pos;
	if (((2*pos)+1)<num)
	{
		if (((2*pos)+2)<num)
		{
			largestChild = ( (list[((2*pos)+2)] < list[((2*pos)+1)]) ? (((2*pos)+1)) : (((2*pos)+2)) );
		}
		largestChild = ((2*pos)+1);
	}

	while (largestChild!=pos && list[pos]<list[largestChild])
	{
		idSwap(list[largestChild], list[pos]);
		pos = largestChild;

		largestChild = pos;
		if (((2*pos)+1)<num)
		{
			if (((2*pos)+2)<num)
			{
				largestChild = ( (list[((2*pos)+2)] < list[((2*pos)+1)]) ? (((2*pos)+1)) : (((2*pos)+2)) );
			}
			largestChild = ((2*pos)+1);
		}
	}
}

/*
================
hhStack<type>::RemoveRange

Removes the specified range of elements [low, high]
Only copies down the array once.
================
*/
template< class type >
ID_INLINE bool hhStack<type>::RemoveRange( int low, int high ) {
	int i;

	assert( list != NULL );
	assert( low >= 0 );
	assert( high < num );
	assert( low <= high );

	if ( ( low < 0 ) || ( high >= num ) || ( low > high ) ) {
		return false;
	}

	int range = (high - low) + 1;
	num -= range;
	for( i = low; i < num; i++ ) {
		list[ i ] = list[ i + range ];
	}

	return true;
}

#endif

#endif
