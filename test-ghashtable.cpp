#include "ghashset.h"
#include <map>
#include <string>

int main()
{
    HT *h = ht_new();
    assert(h != NULL);

    char command[128] = {};
    size_t N = 600000;
    srand(179);
    std::map<std::string, size_t> map;
    for (size_t i = 0; i < N; ++i) {
        if (rand() % 15 == 1 || (i < N / 20))
            *command = 'A';
        else if (rand() % 5 == 2)
            *command = 'D';
        else if (rand() % 5 == 3)
            *command = 'P';
        // else if (rand() % 5 == 4)
        //    *command = 'U';

        char key[MAX_KEY_LEN] = {};
        size_t val = rand();
        for (size_t j = 0; j < 3; ++j) {
            if (rand() % 3 == 1) {
                key[j] = rand() % ('z' - 'a') + 'a';
            } else if (rand() % 3 == 1)
                key[j] = rand() % ('Z' - 'A') + 'A';
            else
                key[j] = rand() % ('9' - '0') + '0';
        }

        #ifdef EXTRA_VERBOSE
            fprintf(stderr, "command No. %zu (%s) with key = #%s#:\n", i, command, key);
        #endif

        switch (*command) {
        case 'A':
            if (map.find(std::string(key)) != map.end()) {
                if (ht_insert(h, key, val, false) != NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
            } else {
                map.insert(std::pair{std::string(key), val});
                if (ht_insert(h, key, val, false) == NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
            }
            break;
        case 'D':
            if (map.find(std::string(key)) == map.end()) {
                if (ht_erase(h, key) != NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
            } else {
                if (ht_erase(h, key) == NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
                map.erase(std::string(key));
            }
            break;
        case 'U':
            if (map.find(std::string(key)) != map.end()) {
                if (ht_insert(h, key, val, true) != NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
                map.erase(std::string(key));
                map.insert(std::pair{std::string(key), val});
            } else {
                if (ht_insert(h, key, val, true) == NOT_FOUND_VAL) {
                    fprintf(stderr, "key = $%s$\n", key);
                    assert(false);
                }
            }
            break;
        case 'P':
            val = ht_find(h, key);
            if (val == NOT_FOUND_VAL) {
                assert(map.find(key) == map.end());
                break;
            }
            assert(val != 0);
            assert((*(map.find(std::string(key)))).second == val);
            break;
        }
    }
    #ifdef EXTRA_VERBOSE
        ht_dump(h, stderr);
    #endif
    ht_delete(h);
}
