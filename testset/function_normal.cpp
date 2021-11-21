int do_it(int n) {
    return n % 2;
}

int main() {
    printf("%d", do_it(5));  // should be 1
    return 0;
}