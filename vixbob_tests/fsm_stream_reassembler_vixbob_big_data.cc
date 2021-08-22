#include "byte_stream.hh"
#include "fsm_stream_reassembler_harness.hh"
#include "stream_reassembler.hh"
#include "util.hh"

#include <exception>
#include <iostream>
#include <random>
#include <string>

using namespace std;

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

int main() {
    try {
        {
            ReassemblerTestHarness test{1500000};
            random_device rd;
            string str("");
            for(int i = 0; i < 2000000; i++) {
                str.push_back(rd() % 26 + 'a');
            }
            for(int i = 0; i < 10000; i++) {
                size_t index = rd() % 2000000;
                size_t length = rd() % (min(static_cast<size_t>(100001), 2000000 - index)) + 1;
                DUMMY_CODE(index, length);
                string tmp = str.substr(index, length);
                test.execute(SubmitSegment{str.substr(index, length), length}.with_eof(index + length == 2000000));
            }
        }
    } catch (const exception &e) {
        cerr << "Exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
