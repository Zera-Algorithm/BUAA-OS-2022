char _my_getchar();
void _my_putchar(char ch);
void _my_exit();

void my_cal() {
	char ch;
	char ans[32];
	int len = 0;
	int i;
	unsigned int num = 0;
	while (1) {
		do {
			ch = _my_getchar();
		} while(ch == 0);
		
		_my_putchar(ch);
		if (ch >= '0' && ch <= '9') {
			num = num * 10 + (ch - '0');
		}
		else if (ch == '\n') {
			break;
		}
	}
	
	do {
		ans[len++] = '0' + (num % 2);
		num /= 2;
	} while (num != 0);
	for (i = len-1; i >= 0; i--) {
		_my_putchar(ans[i]);
	}
}
