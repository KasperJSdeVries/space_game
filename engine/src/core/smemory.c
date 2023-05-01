#include "core/smemory.h"

#include "core/logger.h"
#include "core/sstring.h"
#include "platform/platform.h"

// TODO: Custom string lib
#include <stdio.h>

#define USAGE_STRING_BUFFER_SIZE 8000

struct memory_stats {
	u64 total_allocated;
	u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char *memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
	"UNKNOWN",
	"ARRAY",
	"DARRAY",
	"DICT",
	"RING_QUEUE",
	"BST",
	"STRING",
	"APPLICATION",
	"RENDERER",
	"GAME",
	"JOB",
	"TEXTURE",
	"MATERIAL_INSTANCE",
	"TRANSFORM",
	"ENTITY",
	"ENTITY_NODE",
	"SCENE",
};

static struct memory_stats stats;

void memory_initialize() { platform_zero_memory(&stats, sizeof(stats)); }

void memory_shutdown() {}

void *sallocate(u64 size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		SWARN("sallocate called using MEMORY_TAG_UNKNOWN. Re-class this "
			  "allocation.");
	}

	stats.total_allocated += size;
	stats.tagged_allocations[tag] += size;

	// TODO: Memory alignment
	void *block = platform_allocate(size, false);
	platform_zero_memory(block, size);

	return block;
}

void sfree(void *block, u64 size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		SWARN("sfree called using MEMORY_TAG_UNKNOWN. Re-class this "
			  "allocation.");
	}

	stats.total_allocated -= size;
	stats.tagged_allocations[tag] -= size;

	// TODO: Memory alignment
	platform_free(block, false);
}

void *szero_memory(void *block, u64 size) { return platform_zero_memory(block, size); }

void *scopy_memory(void *dest, const void *source, u64 size) { return platform_copy_memory(dest, source, size); }

void *sset_memory(void *block, i32 value, u64 size) { return platform_set_memory(block, value, size); }

char *get_memory_usage_string() {
	const u64 gib = 1024 * 1024 * 1024;
	const u64 mib = 1024 * 1024;
	const u64 kib = 1024;

	static u64 longest_memory_tag_string;
	if (longest_memory_tag_string == 0) {
		for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
			longest_memory_tag_string = SMAX(longest_memory_tag_string, string_length(memory_tag_strings[i]));
		}
	}

	char buffer[USAGE_STRING_BUFFER_SIZE] = "System memory use (tagged):\n";
	u64 offset                            = string_length(buffer);

	for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
		char unit[4] = "XiB";
		f32 amount   = 1.0f;
		if (stats.tagged_allocations[i] >= gib) {
			unit[0] = 'G';
			amount  = (f32)stats.tagged_allocations[i] / (f32)gib;
		} else if (stats.tagged_allocations[i] >= mib) {
			unit[0] = 'M';
			amount  = (f32)stats.tagged_allocations[i] / (f32)mib;
		} else if (stats.tagged_allocations[i] >= kib) {
			unit[0] = 'K';
			amount  = (f32)stats.tagged_allocations[i] / (f32)kib;
		} else {
			unit[0] = 'B';
			unit[1] = '\0';
			amount  = (f32)stats.tagged_allocations[i];
		}

		i32 length = snprintf(buffer + offset,
							  USAGE_STRING_BUFFER_SIZE,
							  "  %-*s: %.2f%s\n",
							  (i32)longest_memory_tag_string,
							  memory_tag_strings[i],
							  amount,
							  unit);
		offset += (u64)length;
	}
	char *out_string = string_duplicate(buffer);
	return out_string;
}
