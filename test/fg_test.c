/**
 * to test the utility functions
 */


#include <stdio.h>
#include <string.h>
#include "fg_util.h"

static void test_is_substr(void)
{
	printf("\n\nTest is_substr:\n\n");
	char a[100] = "this is a test of substr";
	char test[100];

	strcpy(test, "this");
	printf("\t%10s is a substr of %s : %x\n", test, a, is_substr(a, test));
	strcpy(test, "test");
	printf("\t%10s is a substr of %s : %x\n", test, a, is_substr(a, test));
	strcpy(test, "this12");
	printf("\t%10s is a substr of %s : %x\n", test, a, is_substr(a, test));
	strcpy(test, "this is");
	printf("\t%10s is a substr of %s : %x\n", test, a, is_substr(a, test));
}

static void test_get_last_component(void)
{
	printf("\n\nTest get_last_component:\n\n");
	char a[100] = "/home/varun/test1/test2/test3";
	char b[100] = "/home/varun/test1/test2/test3/";
	char test[100];

	get_last_component(a, test);
	printf("\tlast component of %s is %s\n", a, test);
	get_last_component(b, test);
	printf("\tlast component of %s is %s\n", b, test);
}

static void test_get_next_component(void)
{
	printf("\n\nTest get_next_component:\n\n");
	char a[100] = "/home/varun/test1/test2/test3/";
	char name[100] = "";
	
	printf("\tPath is: %s\n", a);
	
	int hier = 0;
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
	get_next_component(a, hier++, name);
	printf("\t%i : %s\n", hier, name);
}

int main(int argc, char *argv[])
{
	test_is_substr();
	test_get_last_component();
	test_get_next_component();
	return 0;
}
