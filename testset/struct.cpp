struct test {
    int a;
};

int main() {
    struct test t;
    int b[1];
    b[0]=1;
    t->a=1;
    t.a = b[0]+t.a * t.a*3+t.a;
    int a=1;
    a=a*a+a;
    printf("%d", t.a);
    return 0;
}