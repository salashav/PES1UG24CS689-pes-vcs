#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

// Forward declarations to fix the "undefined" errors
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
uint32_t get_file_mode(const char *path);

#define PES_INDEX_FILE ".pes/index"

// ─── PROVIDED (DO NOT DELETE THESE) ──────────────────────────────────────────

IndexEntry* index_find(Index *index, const char *path) {
for (int i = 0; i < index->count; i++) {
if (strcmp(index->entries[i].path, path) == 0)
return &index->entries[i];
}
return NULL;
}

int index_remove(Index *index, const char *path) {
for (int i = 0; i < index->count; i++) {
if (strcmp(index->entries[i].path, path) == 0) {
int remaining = index->count - i - 1;
if (remaining > 0)
memmove(&index->entries[i], &index->entries[i + 1], remaining * sizeof(IndexEntry));
index->count--;
return index_save(index);
}
}
return -1;
}

int index_status(const Index *index) {
printf("Staged changes:\n");
for (int i = 0; i < index->count; i++) {
printf("  staged:     %s\n", index->entries[i].path);
}
if (index->count == 0) printf("  (nothing to show)\n");
return 0;
}

// ─── TODO (The Logic) ────────────────────────────────────────────────────────
// ─── TODO 

static int compare_index_entries(const void *a, const void *b) {
return strcmp(((const IndexEntry *)a)->path, ((const IndexEntry *)b)->path);
@@ -88,53 +88,4 @@
FILE *f = fopen(tmp, "w");
if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&index->entries[i].hash, hex);
        fprintf(f, "%o %s %lu %u %s\n", 
                index->entries[i].mode, hex, 
                (unsigned long)index->entries[i].mtime_sec, 
                (unsigned int)index->entries[i].size, 
                index->entries[i].path);
    }
    fclose(f);
    rename(tmp, PES_INDEX_FILE);
    return 0;
}

int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    void *data = malloc(st.st_size + 1);
    if (st.st_size > 0) fread(data, 1, st.st_size, f);
    fclose(f);

    ObjectID bid;
    object_write(OBJ_BLOB, data, st.st_size, &bid);
    free(data);

    // Manual find to avoid undefined reference if index_find is missing
    IndexEntry *e = NULL;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            e = &index->entries[i];
            break;
        }
    }

    if (!e) {
        if (index->count >= MAX_INDEX_ENTRIES) return -1;
        e = &index->entries[index->count++];
        strncpy(e->path, path, sizeof(e->path) - 1);
    }

    e->mode = get_file_mode(path);
    e->hash = bid;
    e->mtime_sec = (uint64_t)st.st_mtime;
    e->size = (uint32_t)st.st_size;

    return index_save(index);
}
