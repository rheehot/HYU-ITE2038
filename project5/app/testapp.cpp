#include <atomic>
#include <cstring>
#include <chrono>
#include <random>
#include <thread>

#include "dbms.hpp"

int main() {
    Database dbms(100000, false);
    tableid_t tid = dbms.open_table("database1.db");

    constexpr size_t NUM_THREAD = 4;
    constexpr size_t MAX_TRX = 1000;
    constexpr size_t MAX_TRXSEQ = 1000;
    constexpr size_t MAX_KEY_RANGE = 100000;

    std::atomic<int> n_query(0);

    std::thread threads[NUM_THREAD];
    for (int i = 0; i < NUM_THREAD; ++i) {
        threads[i] = std::thread([&] {
            std::random_device rd;
            std::default_random_engine gen(rd());

            int num_trx = gen() % MAX_TRX;
            for (int j = 0; j < num_trx; ++j) {
                trxid_t xid = dbms.begin_trx();

                int num_seq = gen() % MAX_TRXSEQ;
                for (int k = 0; k < num_seq; ++k) {
                    ++n_query;
                    prikey_t key = gen() % MAX_KEY_RANGE;
                    if (gen() % 2 == 0) {
                        dbms.find(tid, key, nullptr, xid);
                    } else {
                        Record record;
                        std::strncpy(reinterpret_cast<char*>(record.value), "hello", 10);
                        dbms.update(tid, key, record, xid);
                    }
                }
                dbms.end_trx(xid);
            }
        });
    }

    bool runnable = true;
    std::thread logger([&] {
        while (runnable) {
            std::printf("\r%d", n_query.load());
        }
    });

    using namespace std::chrono;

    auto now = steady_clock::now();
    for (int i = 0; i < NUM_THREAD; ++i) {
        threads[i].join();
    }
    auto end = steady_clock::now();

    runnable = false;
    logger.join();

    size_t tick = duration_cast<milliseconds>(end - now).count();
    std::cout
        << ' '
        << tick << "ms (" << (static_cast<double>(n_query.load()) / tick) << "/ms)"
        << std::endl;

    return 0;
}