#!/bin/sh

test_description='in-place checkout'
. ./test-lib.sh

test_expect_success 'setup' '

	test_commit hello world &&
	git branch other &&
	test_commit hello-again world
'

test_expect_success 'in-place checkout overwrites open file' '

	git config core.checkoutInPlace true &&
	git checkout -f master &&
	exec 8<world &&
	git checkout other &&
	exec 8<&- &&
	echo hello >expect &&
	test_cmp expect world
'

test_expect_success 'in-place checkout overwrites read-only file' '

	git config core.checkoutInPlace true &&
	git checkout -f master &&
	chmod -w world &&
	git checkout other &&
	echo hello >expect &&
	test_cmp expect world
'

test_expect_success 'in-place checkout updates executable permission' '

	git config core.checkoutInPlace true &&
	git checkout -f master^0 &&
	test_chmod +x world &&
	git commit -m executable &&
	git checkout other &&
	test ! -x world
'

test_expect_success POSIXPERM 'regular checkout respects umask' '

	git config core.checkoutInPlace false &&
	git checkout -f master &&
	chmod 0660 world &&
	umask 0022 &&
	git checkout other &&
	actual=$(ls -l world) &&
	case "$actual" in
	-rw-r--r--*)
		: happy
		;;
	*)
		echo Oops, world is not 0644 but $actual
		false
		;;
	esac
'

test_expect_success POSIXPERM 'in-place checkout ignores umask' '

	git config core.checkoutInPlace true &&
	git checkout -f master &&
	chmod 0660 world &&
	umask 0022 &&
	git checkout other &&
	actual=$(ls -l world) &&
	case "$actual" in
	-rw-rw----*)
		: happy
		;;
	*)
		echo Oops, world is not 0660 but $actual
		false
		;;
	esac
'

test_done
