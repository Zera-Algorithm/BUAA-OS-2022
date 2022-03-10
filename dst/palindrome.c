#include<stdio.h>
int main()
{
	int n;
	scanf("%d",&n);
	int num[20];
	int cnt = 0;
	while(n!=0) {
		num[cnt++] = n % 10;
		n /= 10;
	}
	int isPan = 1;
	for (int i = 0; i < cnt; i++) {
		if (num[i] != num[cnt-1-i]) {
			isPan = 0;
			break;
		}
	}
	if(isPan){
		printf("Y");
	}else{
		printf("N");
	}
	return 0;
}
