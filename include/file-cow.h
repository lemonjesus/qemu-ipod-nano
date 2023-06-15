#ifndef _FILE_COW_H
#define _FILE_COW_H
#include <stdlib.h>
#include <stdio.h>

typedef struct cow_change {
    size_t offset;
    size_t size;
    void* data;
} cow_change;

typedef struct cow_file {
    FILE* file;
    cow_change* changes;
    size_t changes_count;
    char is_protected;
} cow_file;

// cow_open: opens a file for copy-on-write, returns NULL on failure
static cow_file* cow_open(char* filename) {
    cow_file* file = (cow_file*)malloc(sizeof(cow_file));
    file->file = fopen(filename, "rb+");

    if(file->file == NULL) {
        free(file);
        return NULL;
    }

    file->changes = malloc(sizeof(cow_change));
    file->changes_count = 0;
    file->is_protected = 1;
    return file;
}

// cow_close: closes the file and frees memory, losing any uncommitted changes
static void cow_close(cow_file* file) {
    fclose(file->file);

    for(size_t i = 0; i < file->changes_count; i++) {
        if(file->changes[i].data) free(file->changes[i].data);
    }

    free(file->changes);
    free(file);
}

// cow_read: reads from the file, including any changes you've made
static void cow_read(cow_file* file, void* buffer, size_t offset, size_t size) {
    // step one: read from file
    fseek(file->file, offset, SEEK_SET);
    fread(buffer, size, 1, file->file);

    // step two: apply changes if they exist
    for(int i = 0; i < file->changes_count; i++) {
        cow_change change = file->changes[i];

        if(change.offset >= offset && change.offset < offset + size) {
            size_t change_offset = change.offset - offset;
            size_t change_size = size - change_offset;
            if(change_size > change.size) change_size = change.size;

            memcpy(buffer + change_offset, change.data, change_size);
        }
    }
}

// cow_write: writes on top of the file, but doesn't actually write to disk until you commit
static void cow_write(cow_file* file, void* buffer, size_t offset, size_t size) {
    cow_change change;
    change.offset = offset;
    change.size = size;
    change.data = malloc(size);
    memcpy(change.data, buffer, size);

    file->changes_count++;
    file->changes = (cow_change*)realloc(file->changes, sizeof(cow_change) * file->changes_count);
    file->changes[file->changes_count - 1] = change;
}

// cow_size: returns the size of the file
static size_t cow_size(cow_file* file) {
    fseek(file->file, 0, SEEK_END);
    return ftell(file->file);
}

// cow_unprotect: allows the file to be committed
static void cow_unprotect(cow_file* file) {
    file->is_protected = 0;
}

// cow_protect: prevents the file from being committed (default)
static void cow_protect(cow_file* file) {
    file->is_protected = 1;
}

// cow_commit: writes all changes to disk and removes them from memory
static void cow_commit(cow_file* file) {
    if(file->is_protected) return;

    for(size_t i = 0; i < file->changes_count; i++) {
        cow_change change = file->changes[i];
        fseek(file->file, change.offset, SEEK_SET);
        fwrite(change.data, change.size, 1, file->file);
        free(change.data);
    }

    file->changes_count = 0;
    file->changes = (cow_change*)realloc(file->changes, sizeof(cow_change));
}

#endif
