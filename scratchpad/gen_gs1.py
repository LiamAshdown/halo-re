exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())

ELEM = '#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))\n'

# ---------------- darray.c
emit(0x61dba0, 86, 'ArrayNew', 'allocates the 0x18 byte array (grow-by 8 when 0), capacity = grow-by, and the element list.', '''
DArray ArrayNew(int elem_size, int grow_by, ArrayElementFreeFn elem_free_fn)
{
    DArray array = (DArray)malloc(sizeof(DArrayImplementation));

    if (grow_by == 0) {
        grow_by = 8;
    }
    array->count = 0;
    array->capacity = grow_by;
    array->elemsize = elem_size;
    array->growby = grow_by;
    array->elemfreefn = elem_free_fn;
    array->list = grow_by != 0 ? malloc(elem_size * grow_by) : 0;
    return array;
}
''')
emit(0x61dc00, 16, 'ArrayNth', 'the address of element n.', '''
void *ArrayNth(DArray array, int n)
{
    return (char *)array->list + array->elemsize * n;
}
''')
emit(0x61dc10, 60, 'ArrayRemoveAt', 'closes the gap over element n without freeing it.', ELEM + '''
void ArrayRemoveAt(DArray array, int n)
{
    if (n < array->count - 1) {
        memmove(ELEM(array, n), ELEM(array, n + 1), (array->count - n - 1) * array->elemsize);
    }
    array->count--;
}
''')
emit(0x61dc50, 56, 'ArrayMap', 'calls fn(element, client data) for every element, first to last.', ELEM + '''
void ArrayMap(DArray array, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < array->count; i++) {
        fn(ELEM(array, i), client_data);
    }
}
''')
emit(0x61dc90, 47, 'ArrayMapBackwards', 'calls fn(element, client data) for every element, last to first.', ELEM + '''
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        fn(ELEM(array, i), client_data);
    }
}
''')
emit(0x61dcc0, 58, 'ArrayMapBackwards2', 'last to first, returns the first element for which fn returns 0 (else NULL).', ELEM + '''
void *ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void *client_data)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        void *elem = ELEM(array, i);

        if (fn(elem, client_data) == 0) {
            return elem;
        }
    }
    return 0;
}
''')
emit(0x61dda0, 68, 'ArrayFree', 'frees every element through the element free function, then the list and the array.', ELEM + '''
void ArrayFree(DArray array)
{
    int i;

    for (i = 0; i < array->count; i++) {
        if (array->elemfreefn != 0) {
            array->elemfreefn(ELEM(array, i));
        }
    }
    free(array->list);
    free(array);
}
''')
emit(0x61ddf0, 137, 'ArrayInsertAt', 'a full array grows by grow-by (realloc); the tail from n moves up one and the element is copied in.', ELEM + '''
void ArrayInsertAt(DArray array, const void *new_elem, int n)
{
    if (array->count == array->capacity) {
        array->capacity += array->growby;
        array->list = realloc(array->list, array->elemsize * array->capacity);
    }
    array->count++;
    if (n < array->count - 1) {
        memmove(ELEM(array, n + 1), ELEM(array, n), (array->count - n - 1) * array->elemsize);
    }
    memcpy(ELEM(array, n), new_elem, array->elemsize);
}
''')
BS = '''
// 0x61dd40 (EAX count): binary search over count elements from base; comparator(element, key). Returns the
//   insertion point; *found is set when an equal element was seen.
static void *array_bsearch(int count, const void *key, void *base, int elem_size, ArrayCompareFn comparator, int *found)
{
    int low = 0;
    int high = count - 1;

    *found = 0;
    while (low <= high) {
        int mid = (high + low) >> 1;
        int result = comparator((char *)base + mid * elem_size, key);

        if (result == 0) {
            *found = 1;
            high = mid - 1;
        } else if (result > 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return (char *)base + low * elem_size;
}
'''
emit(0x61de80, 57, 'ArrayInsertSorted', 'inserts at the binary-search insertion point (0x61dd40, which takes the count in EAX and exists only for this and ArraySearch; a static here).', BS + '''
void ArrayInsertSorted(DArray array, const void *new_elem, ArrayCompareFn comparator)
{
    int found;
    char *at = (char *)array_bsearch(array->count, new_elem, array->list, array->elemsize, comparator, &found);

    ArrayInsertAt(array, new_elem, (int)(at - (char *)array->list) / array->elemsize);
}
''')
emit(0x61dec0, 87, 'ArrayDeleteAt', 'frees element n through the element free function, then closes the gap.', ELEM + '''
void ArrayDeleteAt(DArray array, int n)
{
    if (array->elemfreefn != 0) {
        array->elemfreefn(ELEM(array, n));
    }
    if (n < array->count - 1) {
        memmove(ELEM(array, n), ELEM(array, n + 1), (array->count - n - 1) * array->elemsize);
    }
    array->count--;
}
''')
emit(0x61df20, 72, 'ArrayReplaceAt', 'frees element n through the element free function and copies the new one over it.', ELEM + '''
void ArrayReplaceAt(DArray array, const void *new_elem, int n)
{
    if (array->elemfreefn != 0) {
        array->elemfreefn(ELEM(array, n));
    }
    memcpy(ELEM(array, n), new_elem, array->elemsize);
}
''')
emit(0x61df70, 140, 'ArraySearch', 'from from_index: a binary search when sorted (0x61dd40, EAX count) else a linear one (0x61dd00, EBX count, comparator(key, element)); both helpers exist only for this and are statics here. The index, or -1 for none / an empty or NULL array.', BS + '''
// 0x61dd00 (EBX count): the first element for which comparator(key, element) is 0, else NULL.
static void *array_lsearch(const void *key, void *base, int elem_size, ArrayCompareFn comparator, int count)
{
    int i;
    char *elem = (char *)base;

    for (i = 0; i < count; i++, elem += elem_size) {
        if (comparator(key, elem) == 0) {
            return (char *)base + i * elem_size;
        }
    }
    return 0;
}

int ArraySearch(DArray array, const void *key, ArrayCompareFn comparator, int from_index, int is_sorted)
{
    int found = 1;
    char *at;

    if (array == 0 || array->count == 0) {
        return -1;
    }
    if (is_sorted != 0) {
        at = (char *)array_bsearch(array->count - from_index, key, (char *)array->list + array->elemsize * from_index,
            array->elemsize, comparator, &found);
    } else {
        at = (char *)array_lsearch(key, (char *)array->list + array->elemsize * from_index, array->elemsize, comparator,
            array->count - from_index);
    }
    if (at == 0 || found == 0) {
        return -1;
    }
    return (int)(at - (char *)array->list) / array->elemsize;
}
''')
emit(0x61e000, 99, 'ArrayClear', 'deletes every element from the last down (free function, then the (empty) gap close).', ELEM + '''
void ArrayClear(DArray array)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        if (array->elemfreefn != 0) {
            array->elemfreefn(ELEM(array, i));
        }
        if (i < array->count - 1) {
            memmove(ELEM(array, i), ELEM(array, i + 1), (array->count - i - 1) * array->elemsize);
        }
        array->count--;
    }
}
''')
emit(0x61e070, 26, 'ArrayAppend', 'inserts at the end; nothing for a NULL array.', '''
void ArrayAppend(DArray array, const void *new_elem)
{
    if (array != 0) {
        ArrayInsertAt(array, new_elem, array->count);
    }
}
''')

# ---------------- hashtable.c
emit(0x61e090, 104, 'TableNew2', 'a 0x14 byte table with num_buckets chains (ArrayNew(elem_size, chains per bucket, free_fn)).', '''
HashTable TableNew2(int elem_size, int num_buckets, int num_chains_per_bucket, TableHashFn hash_fn,
    ArrayCompareFn comp_fn, ArrayElementFreeFn free_fn)
{
    HashTable table = (HashTable)malloc(sizeof(HashImplementation));
    int i;

    table->buckets = (DArray *)malloc(num_buckets * sizeof(DArray));
    for (i = 0; i < num_buckets; i++) {
        table->buckets[i] = ArrayNew(elem_size, num_chains_per_bucket, free_fn);
    }
    table->hashfn = hash_fn;
    table->nbuckets = num_buckets;
    table->freefn = free_fn;
    table->compfn = comp_fn;
    return table;
}
''')
emit(0x61e100, 58, 'TableFree', 'frees every bucket, the bucket list and the table.', '''
void TableFree(HashTable table)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayFree(table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}
''')
emit(0x61e170, 88, 'TableEnter', 'appends to the element\'s bucket, or replaces an equal element (linear search).', '''
void TableEnter(HashTable table, const void *new_elem)
{
    int bucket = table->hashfn(new_elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], new_elem, table->compfn, 0, 0);

    if (index == -1) {
        ArrayAppend(table->buckets[bucket], new_elem);
    } else {
        ArrayReplaceAt(table->buckets[bucket], new_elem, index);
    }
}
''')
emit(0x61e1d0, 79, 'TableRemove', 'deletes an equal element from its bucket; whether one was there.', '''
int TableRemove(HashTable table, const void *del_elem)
{
    int bucket = table->hashfn(del_elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], del_elem, table->compfn, 0, 0);

    if (index == -1) {
        return 0;
    }
    ArrayDeleteAt(table->buckets[bucket], index);
    return 1;
}
''')
emit(0x61e220, 74, 'TableLookup', 'the equal element in its bucket, or NULL.', '''
void *TableLookup(HashTable table, const void *elem)
{
    int bucket = table->hashfn(elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], elem, table->compfn, 0, 0);

    if (index == -1) {
        return 0;
    }
    return ArrayNth(table->buckets[bucket], index);
}
''')
emit(0x61e270, 61, 'TableMap', 'ArrayMap over every bucket.', '''
void TableMap(HashTable table, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayMap(table->buckets[i], fn, client_data);
    }
}
''')
emit(0x61e2b0, 61, 'TableMapSafe', 'ArrayMapBackwards over every bucket.', '''
void TableMapSafe(HashTable table, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayMapBackwards(table->buckets[i], fn, client_data);
    }
}
''')
emit(0x61e2f0, 67, 'TableMapSafe2', 'ArrayMapBackwards2 over the buckets; the first element it stops at, or NULL.', '''
void *TableMapSafe2(HashTable table, ArrayMapFn2 fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        void *elem = ArrayMapBackwards2(table->buckets[i], fn, client_data);

        if (elem != 0) {
            return elem;
        }
    }
    return 0;
}
''')

# ---------------- nonport.c
emit(0x61d200, 6, 'current_time', 'the GetTickCount import thunk (jmp [0x63a0d0]).', '''
unsigned long current_time(void)
{
    return GetTickCount();
}
''')
emit(0x61d210, 12, 'msleep', 'Sleep(msec).', '''
void msleep(unsigned long msec)
{
    Sleep(msec);
}
''')
emit(0x61d220, 51, 'SocketStartUp', 'WSAStartup(1.1) into a stack WSADATA (delay import WS2_32 #115).', '''
void SocketStartUp(void)
{
    WSADATA data;

    WSAStartup(0x101, &data);
}
''')
emit(0x61d260, 5, 'SocketShutDown', 'jmp to the WSACleanup delay-import thunk (WSOCK32 #116).', '''
void SocketShutDown(void)
{
    WSACleanup();
}
''')
emit(0x61d270, 61, 'goastrdup', 'a malloc copy of the string; NULL for NULL (or when malloc fails).', '''
char *goastrdup(const char *src)
{
    char *copy;

    if (src == 0) {
        return 0;
    }
    copy = (char *)malloc(strlen(src) + 1);
    if (copy != 0) {
        strcpy(copy, src);
    }
    return copy;
}
''')
emit(0x61d2b0, 41, 'SetSockBlocking', 'ioctlsocket(FIONBIO, !is_blocking) (WSOCK32 #12); whether it succeeded.', '''
int SetSockBlocking(SOCKET sock, int is_blocking)
{
    u_long argp = is_blocking == 0;

    return ioctlsocket(sock, FIONBIO, &argp) == 0;
}
''')
emit(0x61d2e0, 38, 'SetReceiveBufferSize', 'setsockopt(SOL_SOCKET, SO_RCVBUF, size) (WS2_32 #21); whether it did not fail.', '''
int SetReceiveBufferSize(SOCKET sock, int size)
{
    return setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size)) != SOCKET_ERROR;
}
''')
emit(0x61d310, 91, 'CanReceiveOnSocket', 'a zero-timeout select for readability (nfds 0x40, WSOCK32 #18); 1 when readable.', '''
int CanReceiveOnSocket(SOCKET sock)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    read_set.fd_array[0] = sock;
    read_set.fd_count = 1;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, &read_set, 0, 0, &timeout);
    return result != SOCKET_ERROR && result != 0;
}
''')
print('ok')
