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
	char c[100] = "/";
	char test[100];

	get_last_component(a, test);
	printf("\tlast component of %s is %s\n", a, test);
	get_last_component(b, test);
	printf("\tlast component of %s is %s\n", b, test);
	get_last_component(c, test);
	printf("\tlast component of %s is %s\n", c, test);
}

static void test_get_next_component(void)
{
	printf("\n\nTest get_next_component:\n\n");
	char a[100] = "/home/varun/test1/test2/test3";
	char b[100] = "/home/varun/test1/test2/test3/";
	char name[100] = "";
	int r;
	int hier;
	
	printf("\tPath is: %s\n", a);
	
	hier = 0;
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(a, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);

	printf("\tPath is: %s\n", b);
		
	hier = 0;
	*name = '\0';
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);
	r = get_next_component(b, hier++, name);
	printf("\t%d : %d : %s\n", r, hier, name);

}

void test_get_parent_path(void)
{
	printf("\n\nTest get_parent_path:\n\n");
	char a[100] = "/this/is/a/path/";
	char b[100] = "/this/is/a/path";
	char c[100] = "/";
	char d[100] = "/abc";
	char *tmp;
	char test[100];

	tmp = a;
	get_parent_path(tmp, test);
	printf("\t%s : %s\n", tmp, test);
	get_parent_path(NULL, test);
	printf("\tPrevious result : %s\n", test);
	tmp = b;
	get_parent_path(tmp, test);
	printf("\t%s : %s\n", tmp, test);
	tmp = c;
	get_parent_path(tmp, test);
	printf("\t%s : %s\n", tmp, test);
	tmp = d;
	get_parent_path(tmp, test);
	printf("\t%s : %s\n", tmp, test);
}

int main(int argc, char *argv[])
{
	test_is_substr();
	test_get_last_component();
	test_get_next_component();
	test_get_parent_path();
	return 0;
}
