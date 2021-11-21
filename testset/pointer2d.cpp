int main() {
    int a=5, b=7;
	int *pa, *pb;
	int **ppa, **ppb;
	pa = &a;
	pb = &b;
	ppa = &pa;
	ppb = &pb;
	**ppa += **ppb;
	printf("%d, %d",**ppa, **ppb);
    return 0;
}