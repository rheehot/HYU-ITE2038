#ifndef BUFFER_MANAGER_HPP
#define BUFFER_MANAGER_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include "disk_manager.hpp"
#include "headers.hpp"
#include "status.hpp"

#ifdef TEST_MODULE
#include "test.hpp"
#endif

/// Buffer manager.
class BufferManager;

/// Buffer structure.
class Buffer {
public:
    /// Default constructor, init with block idx -1, null buffer manager.
    Buffer();

    /// Default destructor, call release.
    ~Buffer();

    /// Deleted copy constructor.
    Buffer(Buffer const&) = delete;

    /// Deleted move constructor.
    Buffer(Buffer&&) = delete;

    /// Deleted copy assignment.
    Buffer& operator=(Buffer const&) = delete;

    /// Deleted move assignment.
    Buffer& operator=(Buffer&&) = delete;

    /// Get page frame.
    /// \return Page&, page frame.
    Page& page();

    /// Get page frame.
    /// \return Page const&, page frame.
    Page const& page() const;

    /// Adjacent buffers.
    struct Adjacent { Buffer *prev, *next; };

    /// Get adjacent buffers.
    /// \return Adjacent, adjacent buffers, prev use and next use.
    Adjacent adjacent_buffers() const;

    /// Read buffer.
    /// \param F typename, callback type, Status(Page const&).
    /// \param callback F&&, callback.
    /// \return Status, whether success or not.
    template <typename F>
    inline Status read(F&& callback) {
        ++pin;
        std::shared_lock<std::shared_timed_mutex> lock(mtx);
        CHECK_SUCCESS(callback(static_cast<Page const&>(page())));
        CHECK_SUCCESS(append_mru(true));
        --pin;
        return Status::SUCCESS;
    }

    /// Write buffer.
    /// \param F typename, callback type, Status(Page&).
    /// \param callback F&&, callback.
    /// \return Status, whether success or not.
    template <typename F>
    inline Status write(F&& callback) {
        ++pin;
        std::unique_lock<std::shared_timed_mutex> lock(mtx);
        CHECK_SUCCESS(callback(page()));
        CHECK_SUCCESS(append_mru(true));
        is_dirty = true;
        --pin;
        return Status::SUCCESS;
    }

private:
    Page frame;                     /// page frame.
    pagenum_t pagenum;              /// page ID.
    int index;                      /// block index from manager.
    bool is_allocated;              /// whether buffer is allocated or not.
    bool is_dirty;                  /// whether any values are written in this page frame.
    std::atomic<int> pin;           /// the number of the blocks attaching this buffer.
    std::shared_timed_mutex mtx;    /// whether block is used now.
    Buffer* prev_use;               /// previous used block, for page replacement policy.
    Buffer* next_use;               /// next used block, for page replacement policy.
    FileManager* file;              /// file pointer which current page exist.
    BufferManager* manager;         /// buffer manager which current buffer exist.

    friend class Ubuffer;

    friend class BufferManager;

    /// Clear buffer with given block index and manager.
    /// \param index int, index of the block in buffer.
    /// \param parent BufferManager*, buffer manager.
    /// \return Status, whether success or not.
    Status clear(int index, BufferManager* parent);

    /// Load page frame from file manager.
    /// \param file FileManager&, file manager.
    /// \param pagenum pagenum_t, page ID.
    /// \param virtual_page bool, make virtual page or not.
    /// \return Status, whether success or not.
    Status load(
        FileManager& file, pagenum_t pagenum, bool virtual_page = false);

    /// Create new page frame from file manager.
    /// \param file FileManager&, file manager.
    /// \return Status, whether success or not.
    Status new_page(FileManager& file);

    /// Link neighbor blocks as prev_use and next_use.
    /// WARNING: this method is not thread safe on manage usage.
    /// \return Status, whether success or not.
    Status link_neighbor();

    /// Append block to MRU.
    /// WARNING: this method is not thread safe on manager usage.
    /// \param link bool, link neighbors or not.
    /// \return Status, whether success or not.
    Status append_mru(bool link);

    /// Release page frame.
    /// \return Status, whether success or not.
    Status release();

#ifdef TEST_MODULE
    friend struct BufferTest;

    friend struct BufferManagerTest;
#endif
};

/// Buffer for user povision.
class Ubuffer {
public:
    /// Construct ubuffer.
    /// \param buf Buffer*, target buffer.
    Ubuffer(Buffer& buf);

    /// Construct null buffer.
    /// \param nullptr, null pointer.
    Ubuffer(std::nullptr_t);

    /// Default destructor.
    ~Ubuffer() = default;

    /// Deleted copy constructor.
    Ubuffer(Ubuffer const& ubuffer) = delete;

    /// Move constructor.
    Ubuffer(Ubuffer&& ubuffer) noexcept;

    /// Deleted move assignment.
    Ubuffer& operator=(Ubuffer const& ubuffer) = delete;

    /// Move assignment.
    Ubuffer& operator=(Ubuffer&& ubuffer) noexcept;

    /// Get page frame.
    /// \return Page&, page frame.
    Page& page();

    /// Get page frame.
    /// \return Page const&, page frame.
    Page const& page() const;

    /// Reload buffer.
    /// \return Status, whether success or not.
    Status reload();

    /// Check buffer consistentcy.
    /// \return Status, whether success or not.
    Status check_and_reload();

    /// Get page ID safely.
    /// \return pagenum_t, page ID.
    pagenum_t to_pagenum() const;

    /// Read buffer frame safely.
    /// \param callback Status(Page&), callback.
    /// \return Status, whether success or not.
    template <typename F>
    inline Status read(F&& callback) {
        CHECK_SUCCESS(check_and_reload());
        return buf->read(std::forward<F>(callback));
    }

    /// WRite buffer frame safely.
    /// \param callback Status(Page&), callback.
    /// \return Status, whether success or not.
    template <typename F>
    inline Status write(F&& callback) {
        CHECK_SUCCESS(check_and_reload());
        return buf->write(std::forward<F>(callback));

    }

private:
    Buffer* buf;                /// buffer pointer.
    pagenum_t pagenum;          /// page ID for buffer validation.
    FileManager* file;          /// file pointer for buffer validation.

    /// Construct ubuffer with specified buffer infos.
    /// \param buf Buffer*, target buffer.
    /// \param pagenum pagenum_t, page ID.
    /// \param file FileManager*, file manager.
    Ubuffer(Buffer* buf, pagenum_t pagenum, FileManager* file);

#ifdef TEST_MODULE
    friend struct UbufferTest;

    friend struct BufferManagerTest;
#endif
};

/// Page replacement policy.
struct ReleasePolicy {
    /// Initial searching state.
    /// \param manager BufferManager const&, buffer manager.
    /// \return Buffer*, initial buffer.
    virtual Buffer* init(BufferManager const& manager) const = 0;

    /// Next buffer index.
    /// \param buffer Buffer const&, buffer.
    /// \return int, target buffer.
    virtual Buffer* next(Buffer const& buffer) const = 0;
};

/// Buffer manager.
class BufferManager {
public:
    /// Consturct buffer manager with pool size.
    BufferManager(int num_buffer);

    /// Destructor.
    ~BufferManager() = default;

    /// Deleted copy constructor.
    BufferManager(BufferManager const&) = delete;

    /// Deleted move constructor.
    BufferManager(BufferManager&&) = delete;

    /// Deleted copy assignment.
    BufferManager& operator=(BufferManager const&) = delete;

    /// Deleted move assignment.
    BufferManager& operator=(BufferManager&&) = delete;

    /// Get mru buffer.
    /// \return Buffer*, mru buffer.
    Buffer* most_recently_used() const;

    /// Get lru buffer.
    /// \return Buffer*, lru buffer.
    Buffer* least_recently_used() const;

    /// Shutdown manager.
    /// \return Status, whether success or not.
    Status shutdown();

    /// Return requested buffer.
    /// \param file FileManager&, file manager.
    /// \param pagenum pagenum_t, page ID.
    /// \param virtual_page bool, virtualize page or not.
    /// \return Ubuffer, buffer for user provision.
    Ubuffer buffering(
        FileManager& file, pagenum_t pagenum, bool virtual_page = false);

    /// Create page with given file manager.
    /// \param file FileManager&, file manager.
    /// \return Ubuffer, buffer for user provision.
    Ubuffer new_page(FileManager& file);

    /// Release page from file manager.
    /// \param file FileManager&, file manager.
    /// \param pagenum pagenum_t, page ID.
    /// \return Status, whether success or not.
    Status free_page(FileManager& file, pagenum_t pagenum);

    /// Release all buffer blocks relative to given file id.
    /// \param fileid fileid_t, file ID.
    /// \return Status, whether success or not.
    Status release_file(fileid_t fileid);

private:
    std::recursive_mutex mtx;               /// lock for buffer manager.
    int capacity;                           /// buffer array size.
    int num_buffer;                         /// number of the element.
    Buffer* lru;                            /// least recently used block index.
    Buffer* mru;                            /// most recently used block index.
    std::unique_ptr<Buffer[]> dummy;      /// buffer array.
    std::unique_ptr<Buffer*[]> buffers;      /// usage array.

    friend class Buffer;

    /// Allocate buffer frame.
    /// \return int, index value.
    int allocate_block();

    /// Load page frame to buffer arrays.
    /// \param file FileManager&, file manager.
    /// \param pagenum pagenum_t, page ID.
    /// \param virtual_page bool, virtualize page or not.
    /// \return int, buffer index.
    int load(FileManager& file, pagenum_t pagenum, bool virtual_page = false);

    /// Release buffer block.
    /// \param idx int, buffer index.
    /// \return Status, whether sucess or not.
    Status release_block(int idx);

    /// Release buffer with given policy.
    /// \return int, released index.
    int release(ReleasePolicy const& policy);

    /// Find buffers with given file ID and page ID.
    /// \param fileid fileid_t, file ID.
    /// \param pagenum pagenum_t, page ID.
    /// \return int, found index.
    int find(fileid_t fileid, pagenum_t pagenum);

#ifdef TEST_MODULE
    friend struct BufferManagerTest;

    friend struct BufferTest;
#endif
};

/// Release least recently used buffer.
struct ReleaseLRU : ReleasePolicy {
    /// Initialize searching state.
    /// \return Buffer*, index of lru block.
    Buffer* init(BufferManager const& manager) const override {
        return manager.least_recently_used();
    }
    /// Next searhing state.
    /// \return Buffer*, next usage.
    Buffer* next(Buffer const& buffer) const override {
        return buffer.adjacent_buffers().next;
    }
    /// Get instance.
    static ReleaseLRU const& inst() {
        static ReleaseLRU lru;
        return lru;
    }
};

/// Release most recently used buffer.
struct ReleaseMRU : ReleasePolicy {
    /// Initialize searching state.
    /// \return Buffer*, idnex of mru block.
    Buffer* init(BufferManager const& manager) const override {
        return manager.most_recently_used();
    }
    /// Next searching state.
    /// \return Buffer*, previous usage.
    Buffer* next(Buffer const& buffer) const override {
        return buffer.adjacent_buffers().prev;
    }
    /// Get instance.
    static ReleaseMRU const& inst() {
        static ReleaseMRU mru;
        return mru;
    }
};

#endif