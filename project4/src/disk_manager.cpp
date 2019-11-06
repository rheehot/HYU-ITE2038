#include "disk_manager.hpp"

filenum_t FileManager::create_filenum(std::string const& filename) {
    unsigned long hash = 0;
    for (char c : filename) {
        if (c == '/' || c == '\\') {
            hash = 0;
            continue;
        }
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return (filenum_t)hash;
}

FileManager::FileManager() {
    EXIT_ON_FAILURE(file_init());
}

FileManager::FileManager(std::string const& filename) {
    // if file exist
    if (fexist(filename.c_str())) {
        EXIT_ON_NULL(fp = fopen(filename.c_str(), "r+"));
        id = create_filenum(filename);
    } else {
        // create file
        EXIT_ON_FAILURE(file_create(filename));
    }
}

FileManager::~FileManager() {
    fclose(fp);
}

filenum_t FileManager::get_id() const {
    return id;
}

Status FileManager::file_init() {
    CHECK_TRUE(fresize(fp, PAGE_SIZE));
    // zero-initialization
    FileHeader file_header;
    file_header.free_page_number = 0;
    file_header.root_page_number = 0;
    file_header.number_of_pages = 0;
    // write file header
    CHECK_TRUE(fpwrite(&file_header, sizeof(FileHeader), 0, fp));
    return Status::SUCCESS;
}

Status FileManager::file_create(std::string const& filename) {
    CHECK_NULL(fp = fopen(filename.c_str(), "w+"));
    id = create_filenum(filename);
    return file_init();
}

pagenum_t FileManager::page_create() const {
    return Page::create(
        [this](pagenum_t target, auto func) {
            return rwcallback(target, func);
        }, fp);
}

Status FileManager::page_free(pagenum_t pagenum) const {
    return Page::release(
        [this](pagenum_t target, auto&& func) { 
            return rwcallback(target, func);
        }, pagenum);
}

Status FileManager::page_read(pagenum_t pagenum, Page& dst) const {
    CHECK_TRUE(fpread(&dst, sizeof(Page), pagenum * PAGE_SIZE, fp));
    return Status::SUCCESS;
}

Status FileManager::page_write(pagenum_t pagenum, Page const& src) const {
    CHECK_TRUE(fpwrite(&src, sizeof(Page), pagenum * PAGE_SIZE, fp));
    return Status::SUCCESS;
}
