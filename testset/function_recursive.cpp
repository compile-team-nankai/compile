int do_it(int n) {
    if (n == 1) {
        return 0;
    }
    return do_it(n-1) + n;
}

int main() {
    printf("%d", do_it(5));  // should be 14
    return 0;
}