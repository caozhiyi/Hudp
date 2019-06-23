#ifndef HEADER_BASE_TSQUEUE
#define HEADER_BASE_TSQUEUE

#include <mutex>
namespace base {

    // thread safe queue
    template<typename T>
    class CTSQueue {
    public:
        CTSQueue() {

        }

        ~CTSQueue() {

        }

        void Push(const T& element) {
            std::unique_lock<std::mutex> lock(_mutex);
            _block_queue.push(element);
        }

        bool Pop(T& value) {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_queue.empty()) {
                return false;
            }
            T ret = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        void Clear() {
            std::unique_lock<std::mutex> lock(_mutex);
            while (!_block_queue.empty()) {
                _block_queue.pop();
            }
        }

        size_t Size() {
            std::unique_lock<std::mutex> lock(_mutex);
            return _queue.size();
        }

    private:
        std::queue<T>		_queue;
        std::mutex			_mutex;
    };
}

#endif