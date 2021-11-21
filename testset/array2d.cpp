// #include <stdio.h>

int main() {
    int a[5][7];
	int num = 0;
    for (int i=0; i < 5; i=i+1) {
		for (int j=0; j<7; j=j+1){
			a[i][j]=num;
			num ++;
		}
    }
	printf("%d",a[3][2]);
    return 0;
}