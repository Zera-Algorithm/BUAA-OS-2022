extern int some_global_variable;

int do_something (int var);

int main () {
	int some_stack_variable = do_something(3);
	some_global_variable = 0xf4561230;
	return 0;
}
