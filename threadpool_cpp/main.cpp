#include <iostream>
#include "threadpool.hpp"
using namespace std;

atomic_int64_t cnt = 0;
int64_t PrintHelloWorld(){
    return cnt++;
}

struct Number{
    bool isnan;
    double value;
    void operator()() const{
        if (isnan)
            // cout是原子的
            cout << "nan\n";
        else {
            char buf[1024];
            sprintf(buf, "%lf\n", value);
            cout << string(buf);
        }
    }
};


int main() {

    ThreadPool pool(10);
    vector<decltype(pool.commit(PrintHelloWorld))> results1;
    // 相当于vector<future<int64_t>> results1;
    results1.reserve(50);
    for (int i=0; i<50; i++){
        results1.emplace_back(pool.commit(PrintHelloWorld));
    }
    for (auto& r : results1){
        cout << r.get() << endl;
    }
    cout << "\n\n";
    for (int i=0; i<50; i++){
        pool.commit(Number{static_cast<bool>(rand() % 2), static_cast<double>(rand() % 200)});
    }
    while(!pool.IsAllFinished());

}
