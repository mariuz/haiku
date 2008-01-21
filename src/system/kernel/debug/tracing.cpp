/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <tracing.h>

#include <stdarg.h>
#include <stdlib.h>

#include <debug.h>
#include <kernel.h>
#include <util/AutoLock.h>


#if ENABLE_TRACING

//#define TRACE_TRACING
#ifdef TRACE_TRACING
#	define TRACE(x) dprintf_no_syslog x
#else
#	define TRACE(x) ;
#endif


enum {
	WRAP_ENTRY			= 0x01,
	ENTRY_INITIALIZED	= 0x02,
	BUFFER_ENTRY		= 0x04
};

static const size_t kBufferSize = MAX_TRACE_SIZE / 4;

static trace_entry* sBuffer;
static trace_entry* sFirstEntry;
static trace_entry* sAfterLastEntry;
static uint32 sEntries;
static uint32 sWritten;
static spinlock sLock;


static trace_entry*
next_entry(trace_entry* entry)
{
	entry += entry->size;
	if ((entry->flags & WRAP_ENTRY) != 0)
		entry = sBuffer;

	if (entry == sAfterLastEntry)
		return NULL;

	return entry;
}


static trace_entry*
previous_entry(trace_entry* entry)
{
	if (entry == sFirstEntry)
		return NULL;

	if (entry == sBuffer) {
		// beginning of buffer -- previous entry is a wrap entry
		entry = sBuffer + kBufferSize - entry->previous_size;
	}

	return entry - entry->previous_size;
}


static bool
free_first_entry()
{
	TRACE(("  skip start %p, %lu*4 bytes\n", sFirstEntry, sFirstEntry->size));

	trace_entry* newFirst = next_entry(sFirstEntry);

	if (sFirstEntry->flags & BUFFER_ENTRY) {
		// a buffer entry -- just skip it
	} else if (sFirstEntry->flags & ENTRY_INITIALIZED) {
		// fully initialized TraceEntry -- destroy it
		((TraceEntry*)sFirstEntry)->~TraceEntry();
		sEntries--;
	} else {
		// Not fully initialized TraceEntry. We can't free it, since
		// then it's constructor might still write into the memory and
		// overwrite data of the entry we're going to allocate.
		// We can't do anything until this entry can be discarded.
		return false;
	}

	if (newFirst == NULL) {
		// everything is freed -- that practically this can't happen, if
		// the buffer is large enough to hold three max-sized entries
		sFirstEntry = sAfterLastEntry = sBuffer;
		TRACE(("free_first_entry(): all entries freed!\n"));
	} else
		sFirstEntry = newFirst;

	return true;
}


/*!	Makes sure we have needed * 4 bytes of memory at sAfterLastEntry.
	Returns \c false, if unable to free that much.
*/
static bool
make_space(size_t needed)
{
	// we need space for sAfterLastEntry, too (in case we need to wrap around
	// later)
	needed++;

	// If there's not enough space (free or occupied) after sAfterLastEntry,
	// we free all entries in that region and wrap around.
	if (sAfterLastEntry + needed > sBuffer + kBufferSize) {
		TRACE(("make_space(%lu), wrapping around: after last: %p\n", needed,
			sAfterLastEntry));

		// Free all entries after sAfterLastEntry and one more at the beginning
		// of the buffer.
		while (sFirstEntry > sAfterLastEntry) {
			if (!free_first_entry())
				return false;
		}
		if (sAfterLastEntry != sBuffer && !free_first_entry())
			return false;

		// just in case free_first_entry() freed the very last existing entry
		if (sAfterLastEntry == sBuffer)
			return true;

		// mark as wrap entry and actually wrap around
		trace_entry* wrapEntry = sAfterLastEntry;
		wrapEntry->size = 0;
		wrapEntry->flags = WRAP_ENTRY;
		sAfterLastEntry = sBuffer;
		sAfterLastEntry->previous_size = sBuffer + kBufferSize - wrapEntry;
	}

	if (sFirstEntry <= sAfterLastEntry) {
		// buffer is empty or the space after sAfterLastEntry is unoccupied
		return true;
	}

	// free the first entries, until there's enough space
	size_t space = sFirstEntry - sAfterLastEntry;

	if (space < needed) {
		TRACE(("make_space(%lu), left %ld\n", needed, space));
	}

	while (space < needed) {
		space += sFirstEntry->size;

		if (!free_first_entry())
			return false;
	}

	TRACE(("  out: start %p, entries %ld\n", sFirstEntry, sEntries));

	return true;
}


static trace_entry*
allocate_entry(size_t size, uint16 flags)
{
	if (sBuffer == NULL || size == 0 || size >= 65532)
		return NULL;

	InterruptsSpinLocker _(sLock);

	size = (size + 3) >> 2;
		// 4 byte aligned, don't store the lower 2 bits

	TRACE(("allocate_entry(%lu), start %p, end %p, buffer %p\n", size * 4,
		sFirstEntry, sAfterLastEntry, sBuffer));

	if (!make_space(size))
		return NULL;

	trace_entry* entry = sAfterLastEntry;
	entry->size = size;
	entry->flags = flags;
	sAfterLastEntry += size;
	sAfterLastEntry->previous_size = size;

	if (!(flags & BUFFER_ENTRY))
		sEntries++;

	TRACE(("  entry: %p, end %p, start %p, entries %ld\n", entry,
		sAfterLastEntry, sFirstEntry, sEntries));

	return entry;
}


#endif	// ENABLE_TRACING


// #pragma mark -


TraceOutput::TraceOutput(char* buffer, size_t bufferSize)
	: fBuffer(buffer),
	  fCapacity(bufferSize)
{
	Clear();
}


void
TraceOutput::Clear()
{
	if (fCapacity > 0)
		fBuffer[0] = '\0';
	fSize = 0;
}


void
TraceOutput::Print(const char* format,...)
{
	if (IsFull())
		return;

	va_list args;
	va_start(args, format);
	fSize += vsnprintf(fBuffer + fSize, fCapacity - fSize, format, args);
	va_end(args);
}


//	#pragma mark -


TraceEntry::TraceEntry()
{
}


TraceEntry::~TraceEntry()
{
}


void
TraceEntry::Dump(TraceOutput& out)
{
#if ENABLE_TRACING
	// to be overridden by subclasses
	out.Print("ENTRY %p", this);
#endif
}


void
TraceEntry::Initialized()
{
#if ENABLE_TRACING
	flags |= ENTRY_INITIALIZED;
	sWritten++;
#endif
}


void*
TraceEntry::operator new(size_t size, const std::nothrow_t&) throw()
{
#if ENABLE_TRACING
	return allocate_entry(size, 0);
#else
	return NULL;
#endif
}


//	#pragma mark -


AbstractTraceEntry::~AbstractTraceEntry()
{
}


void
AbstractTraceEntry::Dump(TraceOutput& out)
{
	out.Print("[%6ld] %Ld: ", fThread, fTime);
	AddDump(out);
}


void
AbstractTraceEntry::AddDump(TraceOutput& out)
{
}


//	#pragma mark - trace filters


class LazyTraceOutput : public TraceOutput {
public:
	LazyTraceOutput(char* buffer, size_t bufferSize)
		: TraceOutput(buffer, bufferSize)
	{
	}

	const char* DumpEntry(const TraceEntry* entry)
	{
		if (Size() == 0) {
			const_cast<TraceEntry*>(entry)->Dump(*this);
				// Dump() should probably be const
		}

		return Buffer();
	}
};


class TraceFilter {
public:
	virtual ~TraceFilter()
	{
	}

	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return false;
	}

public:
	union {
		thread_id	fThread;
		const char*	fString;
		struct {
			TraceFilter*	first;
			TraceFilter*	second;
		} fSubFilters;
	};
};


class ThreadTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* _entry, LazyTraceOutput& out)
	{
		const AbstractTraceEntry* entry
			= dynamic_cast<const AbstractTraceEntry*>(_entry);
		return (entry != NULL && entry->Thread() == fThread);
	}
};


class PatternTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return strstr(out.DumpEntry(entry), fString) != NULL;
	}
};


class NotTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return !fSubFilters.first->Filter(entry, out);
	}
};


class AndTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fSubFilters.first->Filter(entry, out)
			&& fSubFilters.second->Filter(entry, out);
	}
};


class OrTraceFilter : public TraceFilter {
public:
	virtual bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fSubFilters.first->Filter(entry, out)
			|| fSubFilters.second->Filter(entry, out);
	}
};


class TraceFilterParser {
public:
	static TraceFilterParser* Default()
	{
		return &sParser;
	}

	bool Parse(int argc, const char* const* argv)
	{
		fTokens = argv;
		fTokenCount = argc;
		fTokenIndex = 0;
		fFilterCount = 0;

		TraceFilter* filter = _ParseExpression();
		return fTokenIndex == fTokenCount && filter != NULL;
	}

	bool Filter(const TraceEntry* entry, LazyTraceOutput& out)
	{
		return fFilters[0].Filter(entry, out);
	}

private:
	TraceFilter* _ParseExpression()
	{
		const char* token = _NextToken();
		if (!token) {
			// unexpected end of expression
			return NULL;
		}

		if (fFilterCount == MAX_FILTERS) {
			// too many filters
			return NULL;
		}

		if (token[0] == '#') {
			TraceFilter* filter = new(&fFilters[fFilterCount++])
				PatternTraceFilter;
			filter->fString = token + 1;
			return filter;
		} else if (strcmp(token, "not") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) NotTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL)
				return filter;
			return NULL;
		} else if (strcmp(token, "and") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) AndTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL
				&& (filter->fSubFilters.second = _ParseExpression()) != NULL) {
				return filter;
			}
			return NULL;
		} else if (strcmp(token, "or") == 0) {
			TraceFilter* filter = new(&fFilters[fFilterCount++]) OrTraceFilter;
			if ((filter->fSubFilters.first = _ParseExpression()) != NULL
				&& (filter->fSubFilters.second = _ParseExpression()) != NULL) {
				return filter;
			}
			return NULL;
		} else if (strcmp(token, "thread") == 0) {
			const char* arg = _NextToken();
			if (arg == NULL) {
				// unexpected end of expression
				return NULL;
			}

			TraceFilter* filter = new(&fFilters[fFilterCount++])
				ThreadTraceFilter;
			filter->fThread = strtol(arg, NULL, 0);
			return filter;
		} else {
			// invalid token
			return NULL;
		}
	}

	const char* _CurrentToken() const
	{
		if (fTokenIndex >= 1 && fTokenIndex <= fTokenCount)
			return fTokens[fTokenIndex - 1];
		return NULL;
	}

	const char* _NextToken()
	{
		if (fTokenIndex >= fTokenCount)
			return NULL;
		return fTokens[fTokenIndex++];
	}

private:
	enum { MAX_FILTERS = 32 };

	const char* const*			fTokens;
	int							fTokenCount;
	int							fTokenIndex;
	TraceFilter					fFilters[MAX_FILTERS];
	int							fFilterCount;

	static TraceFilterParser	sParser;
};


TraceFilterParser TraceFilterParser::sParser;


//	#pragma mark -


#if ENABLE_TRACING


class TraceEntryIterator {
public:
	TraceEntryIterator(bool startFront)
		:
		fEntry(NULL),
		fIndex(startFront ? 0 : sEntries + 1)
	{
	}

	int32 Index() const
	{
		return fIndex;
	}

	TraceEntry* Current() const
	{
		return (TraceEntry*)fEntry;
	}

	TraceEntry* Next()
	{
		if (fIndex == 0) {
			fEntry = _NextNonBufferEntry(sFirstEntry);
			fIndex = 1;
		} else if (fEntry != NULL) {
			fEntry = _NextNonBufferEntry(next_entry(fEntry));
			fIndex++;
		}

		return Current();
	}

	TraceEntry* Previous()
	{
		if (fIndex == (int32)sEntries + 1)
			fEntry = sAfterLastEntry;

		if (fEntry != NULL) {
			fEntry = _PreviousNonBufferEntry(previous_entry(fEntry));
			fIndex--;
		}

		return Current();
	}

	TraceEntry* MoveTo(int32 index)
	{
		if (index == fIndex)
			return Current();

		if (index <= 0 || index > (int32)sEntries) {
			fIndex = (index <= 0 ? 0 : sEntries + 1);
			fEntry = NULL;
			return NULL;
		}

		// get the shortest iteration path
		int32 distance = index - fIndex;
		int32 direction = distance < 0 ? -1 : 1;
		distance *= direction;

		if (index < distance) {
			distance = index;
			direction = 1;
			fEntry = NULL;
			fIndex = 0;
		}
		if ((int32)sEntries + 1 - fIndex < distance) {
			distance = sEntries + 1 - fIndex;
			direction = -1;
			fEntry = NULL;
			fIndex = sEntries + 1;
		}

		// iterate to the index
		if (direction < 0) {
			while (fIndex != index)
				Previous();
		} else {
			while (fIndex != index)
				Next();
		}

		return Current();
	}

private:
	trace_entry* _NextNonBufferEntry(trace_entry* entry)
	{
		while (entry != NULL && (entry->flags & BUFFER_ENTRY) != 0)
			entry = next_entry(entry);

		return entry;
	}

	trace_entry* _PreviousNonBufferEntry(trace_entry* entry)
	{
		while (entry != NULL && (entry->flags & BUFFER_ENTRY) != 0)
			entry = previous_entry(entry);

		return entry;
	}

private:
	trace_entry*	fEntry;
	int32			fIndex;
};


int
dump_tracing(int argc, char** argv)
{
	int argi = 1;

	// variables in which we store our state to be continuable
	static int32 _previousCount = 0;
	static bool _previousHasFilter = false;
	static int32 _previousMaxToCheck = 0;
	static int32 _previousFirstChecked = 1;
	static int32 _previousLastChecked = -1;
	static uint32 _previousWritten = 0;

	// Note: start and index are Pascal-like indices (i.e. in [1, sEntries]).
	int32 start = 0;	// special index: print the last count entries
	int32 count = 0;
	int32 maxToCheck = 0;
	int32 cont = 0;

	bool hasFilter = false;

	if (argi < argc) {
		if (strcmp(argv[argi], "forward") == 0) {
			cont = 1;
			argi++;
		} else if (strcmp(argv[argi], "backward") == 0) {
			cont = -1;
			argi++;
		}
	}

	if (cont != 0) {
		if (argi < argc) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
		if (sWritten == 0 || sWritten != _previousWritten) {
			kprintf("Can't continue iteration. \"%s\" has not been invoked "
				"before, or there were new entries written since the last "
				"invocation.\n", argv[0]);
			return 0;
		}
	}

	// get start, count, maxToCheck
	int32* params[3] = { &start, &count, &maxToCheck };
	for (int i = 0; i < 3 && !hasFilter && argi < argc; i++) {
		if (strcmp(argv[argi], "filter") == 0) {
			hasFilter = true;
			argi++;
		} else if (argv[argi][0] == '#') {
			hasFilter = true;
		} else {
			*params[i] = parse_expression(argv[argi]);
			argi++;
		}
	}

	// filter specification
	if (argi < argc) {
		hasFilter = true;
		if (strcmp(argv[argi], "filter") == 0)
			argi++;

		if (!TraceFilterParser::Default()->Parse(argc - argi, argv + argi)) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	int32 direction;
	int32 firstToCheck;
	int32 lastToCheck;

	if (cont != 0) {
		// get values from the previous iteration
		direction = cont;
		count = _previousCount;
		maxToCheck = _previousMaxToCheck;
		hasFilter = _previousHasFilter;

		if (direction < 0)
			start = _previousFirstChecked - 1;
		else
			start = _previousLastChecked + 1;
	} else {
		// defaults for count and maxToCheck
		if (count == 0)
			count = 30;
		if (maxToCheck == 0 || !hasFilter)
			maxToCheck = count;
		else if (maxToCheck < 0)
			maxToCheck = sEntries;

		// determine iteration direction
		direction = (start <= 0 || count < 0 ? -1 : 1);

		// validate count and maxToCheck
		if (count < 0)
			count = -count;
		if (maxToCheck < 0)
			maxToCheck = -maxToCheck;
		if (maxToCheck > (int32)sEntries)
			maxToCheck = sEntries;
		if (count > maxToCheck)
			count = maxToCheck;

		// validate start
		if (start <= 0 || start > (int32)sEntries)
			start = max_c(1, sEntries);
	}

	if (direction < 0) {
		firstToCheck = max_c(1, start - maxToCheck + 1);
		lastToCheck = start;
	} else {
		firstToCheck = start;
		lastToCheck = min_c((int32)sEntries, start + maxToCheck - 1);
	}

	TraceEntryIterator iterator(true);

	char buffer[256];
	LazyTraceOutput out(buffer, sizeof(buffer));

	if (direction < 0 && hasFilter && lastToCheck - firstToCheck >= count) {
		// iteration direction is backwards

		// From the last entry to check iterate backwards to check filter
		// matches.
		int32 matching = 0;

		// move to the entry after the last entry to check
		iterator.MoveTo(lastToCheck + 1);

		// iterate backwards
		while (iterator.Index() > firstToCheck) {
			TraceEntry* entry = iterator.Previous();
			if ((entry->flags & ENTRY_INITIALIZED) != 0) {
				out.Clear();
				if (TraceFilterParser::Default()->Filter(entry, out)) {
					matching++;
					if (matching >= count)
						break;
				}
			}
		}

		firstToCheck = iterator.Index();

		// iterate to the previous entry, so that the next loop starts at the
		// right one
		iterator.Previous();
	}

	int32 dumped = 0;

	while (TraceEntry* entry = iterator.Next()) {
		int32 index = iterator.Index();
		if (index < firstToCheck)
			continue;
		if (index > lastToCheck || dumped >= count) {
			if (direction > 0)
				lastToCheck = index - 1;
			break;
		}

		if ((entry->flags & ENTRY_INITIALIZED) != 0) {
			out.Clear();
			if (hasFilter && !TraceFilterParser::Default()->Filter(entry, out))
				continue;

			kprintf("%5ld. %s\n", index, out.DumpEntry(entry));
		} else if (!hasFilter)
			kprintf("%5ld. ** uninitialized entry **\n", index);

		dumped++;
	}

	kprintf("printed %ld entries within range %ld to %ld (%ld of %ld total, "
		"%ld ever)\n", dumped, firstToCheck, lastToCheck,
		lastToCheck - firstToCheck + 1, sEntries, sWritten);

	// store iteration state
	_previousCount = count;
	_previousMaxToCheck = maxToCheck;
	_previousHasFilter = hasFilter;
	_previousFirstChecked = firstToCheck;
	_previousLastChecked = lastToCheck;
	_previousWritten = sWritten;

	return cont != 0 ? B_KDEBUG_CONT : 0;
}


#endif	// ENABLE_TRACING


extern "C" uint8*
alloc_tracing_buffer(size_t size)
{
#if	ENABLE_TRACING
	trace_entry* entry = allocate_entry(size + sizeof(trace_entry),
		BUFFER_ENTRY);
	if (entry == NULL)
		return NULL;

	return (uint8*)(entry + 1);
#else
	return NULL;
#endif
}


uint8*
alloc_tracing_buffer_memcpy(const void* source, size_t size, bool user)
{
	if (user && !IS_USER_ADDRESS(source))
		return NULL;

	uint8* buffer = alloc_tracing_buffer(size);
	if (buffer == NULL)
		return NULL;

	if (user) {
		if (user_memcpy(buffer, source, size) != B_OK)
			return NULL;
	} else
		memcpy(buffer, source, size);

	return buffer;
}


char*
alloc_tracing_buffer_strcpy(const char* source, size_t maxSize, bool user)
{
	if (source == NULL || maxSize == 0)
		return NULL;

	if (user && !IS_USER_ADDRESS(source))
		return NULL;

	// limit maxSize to the actual source string len
	if (user) {
		ssize_t size = user_strlcpy(NULL, source, 0);
			// there's no user_strnlen()
		if (size < 0)
			return 0;
		maxSize = min_c(maxSize, (size_t)size + 1);
	} else
		maxSize = strnlen(source, maxSize - 1) + 1;

	char* buffer = (char*)alloc_tracing_buffer(maxSize);
	if (buffer == NULL)
		return NULL;

	if (user) {
		if (user_strlcpy(buffer, source, maxSize) < B_OK)
			return NULL;
	} else
		strlcpy(buffer, source, maxSize);

	return buffer;
}


extern "C" status_t
tracing_init(void)
{
#if	ENABLE_TRACING
	area_id area = create_area("tracing log", (void**)&sBuffer,
		B_ANY_KERNEL_ADDRESS, MAX_TRACE_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	sFirstEntry = sBuffer;
	sAfterLastEntry = sBuffer;

	add_debugger_command_etc("traced", &dump_tracing,
		"Dump recorded trace entries",
		"(\"forward\" | \"backward\") | ([ <start> [ <count> [ <range> ] ] ] "
			"[ #<pattern> | (\"filter\" <filter>) ])\n"
		"Prints recorded trace entries. If \"backward\" or \"forward\" is\n"
		"specified, the command continues where the previous invocation left\n"
		"off, i.e. printing the previous respectively next entries (as many\n"
		"as printed before). In this case the command is continuable, that is\n"
		"afterwards entering an empty line in the debugger will reinvoke it.\n"
		"  <start>    - The base index of the entries to print. Depending on\n"
		"               whether the iteration direction is forward or\n"
		"               backward this will be the first or last entry printed\n"
		"               (potentially, if a filter is specified). The index of\n"
		"               the first entry in the trace buffer is 1. If 0 is\n"
		"               specified, the last <count> recorded entries are\n"
		"               printed (iteration direction is backward). Defaults \n"
		"               to 0.\n"
		"  <count>    - The number of entries to be printed. Defaults to 30.\n"
		"               If negative, the -<count> entries before and\n"
		"               including <start> will be printed.\n"
		"  <range>    - Only relevant if a filter is specified. Specifies the\n"
		"               number of entries to be filtered -- depending on the\n"
		"               iteration direction the entries before or after\n"
		"               <start>. If more than <count> entries match the\n"
		"               filter, only the first (forward) or last (backward)\n"
		"               <count> matching entries will be printed. If 0 is\n"
		"               specified <range> will be set to <count>. If -1,\n"
		"               <range> will be set to the number of recorded\n"
		"               entries.\n"
		"  <pattern>  - If specified only entries containing this string are\n"
		"               printed.\n"
		"  <filter>   - If specified only entries matching this filter\n"
		"               expression are printed. The expression can consist of\n"
		"               prefix operators \"not\", \"and\", \"or\", filters of\n"
		"               the kind \"'thread' <thread>\" (matching entries\n"
		"               with the given thread ID), or filter of the kind\n"
		"               \"#<pattern>\" (matching entries containing the given\n"
		"               string.\n", 0);
#endif	// ENABLE_TRACING
	return B_OK;
}

