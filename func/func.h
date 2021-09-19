struct __attribute__ ((packed)) func_request {
    int func_id;
    uint64_t req_id;
    int64_t x;
    int64_t y;
    int64_t MOD;
};
struct __attribute__ ((packed)) func_response {
    int func_id;
    uint64_t req_id;
    int64_t ans;
};
