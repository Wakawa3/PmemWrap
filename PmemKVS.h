#define LAYOUT "KVS_layout"
//#define NUM_LAYOUT "number"

// Maximum length of our buffer
#define MAX_BUF_LEN 30
#define MAX_STORE 100

#define PATH "/mnt/pmem0/data"
#define PMEM_POS "/mnt/pmem0/"
// #define PATH "/mnt/pmem0/data_replica"

#define READ_FROM_KEY 0
#define READ_ALL 1
#define READ_FROM_ID 2

// Root structure
struct subKVstruct {
    int keylen;
	char key[MAX_BUF_LEN];
    int value;
};

struct KVstruct{
    struct subKVstruct data[MAX_STORE];
    int number;
};