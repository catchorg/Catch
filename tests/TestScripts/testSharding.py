#!/usr/bin/env python3

"""
This test script verifies that the random ordering of tests inside
Catch2 is invariant in regards to subsetting. This is done by running
the binary 3 times, once with all tests selected, and twice with smaller
subsets of tests selected, and verifying that the selected tests are in
the same relative order.
"""

import itertools
import multiprocessing
import random
import subprocess
import sys
import xml.etree.ElementTree as ET

from collections import namedtuple

TestCase = namedtuple("TestCase", ("shard_count", "order", "tags", "rng_seed"))

def list_tests(self_test_exe, test_case, shard_index=None): # tags, rng_seed, shard_count=None, shard_index=None):
    cmd = [
        self_test_exe,
        '--reporter', 'xml',
        '--list-tests',
        '--order', test_case.order,
        '--rng-seed', str(test_case.rng_seed)
    ]

    if shard_index is not None:
        cmd.extend([
            "--shard-count", str(test_case.shard_count),
            "--shard-index", str(shard_index)
        ])

    tags_arg = ','.join('[{}]~[.]'.format(t) for t in test_case.tags)
    if tags_arg:
        cmd.append(tags_arg)
    process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if stderr:
        raise RuntimeError("Unexpected error output:\n" + process.stderr)

    root = ET.fromstring(stdout)
    result = [elem.text for elem in root.findall('./TestCase/Name')]

    if len(result) < 2:
        raise RuntimeError("Unexpectedly few tests listed (got {})".format(
            len(result)))
    return result

def execute_tests(self_test_exe, test_case, shard_index):
    cmd = [
        self_test_exe,
        '--reporter', 'xml',
        '--order', test_case.order,
        '--rng-seed', str(test_case.rng_seed),
        "--shard-count", str(test_case.shard_count),
        "--shard-index", str(shard_index)
    ]

    tags_arg = ','.join('[{}]~[.]'.format(t) for t in test_case.tags)
    if tags_arg:
        cmd.append(tags_arg)
    process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if stderr:
        raise RuntimeError("Unexpected error output:\n" + process.stderr)

    root = ET.fromstring(stdout)
    result = [elem.attrib["name"] for elem in root.findall('./Group/TestCase')]

    if len(result) < 2:
        raise RuntimeError("Unexpectedly few tests listed (got {})".format(
            len(result)))
    return result

def check_listed_tests_match(all_tests, sharded_tests):
    # Make sure the number of tests in the full list, and the sharded lists are the same.
    total_test_count = len(all_tests)
    sharded_test_count = sum([len(shard) for shard in sharded_tests])

    assert total_test_count == sharded_test_count, f"Sharded test count ({sharded_test_count}) does not match the total test count ({total_test_count})"

    # Make sure all the tests in the shards are from the full list, in the same order. Together with the previous check, this
    # ensures that all tests in the full list, are in the shards without duplication.
    test_index = 0
    for shard_index, shard in enumerate(sharded_tests):
        for shard_test_index, test_name in enumerate(shard):
            assert test_name == all_tests[test_index], f"Sharding does not split the test list while maintaining order {test_index}:\n'{test_name}' vs '{all_tests[test_index]}'"

            test_index += 1

def check_listed_and_executed_tests_match(listed_tests, executed_tests):
    for shard_index, listed_shard in enumerate(listed_tests):
        listed_shard_names = set(listed_shard)
        executed_shard_names = set(executed_tests[shard_index])

        listed_string = "\n".join(listed_shard_names)
        exeucted_string = "\n".join(executed_shard_names)

        assert listed_shard_names == executed_shard_names, f"Executed tests do not match the listed tests:\nExecuted:\n{exeucted_string}\n\nListed:\n{listed_string}"

def test_shards_cover_all_test(self_test_exe, test_case):
    all_tests = list_tests(self_test_exe, test_case)
    sharded_tests = [list_tests(self_test_exe, test_case, index) for index in range(test_case.shard_count)]

    check_listed_tests_match(all_tests, sharded_tests)

    executed_tests = [execute_tests(self_test_exe, test_case, index) for index in range(test_case.shard_count)]

    check_listed_and_executed_tests_match(sharded_tests, executed_tests)


def main():
    self_test_exe, = sys.argv[1:]

    # We want a random seed for the test, but want to avoid 0, because it has special meaning
    shard_counts = [1, 5]
    seeds = [random.randint(1, 2 ** 32 - 1), random.randint(1, 2 ** 32 - 1)]
    tags = [["generators"], ["generators", "matchers"], []]
    orders = ["rand", "decl", "lex"]

    test_cases = [TestCase(*t) for t in itertools.product(shard_counts, orders, tags, seeds)]

    # We use multiprocessing here because there are quite a few test cases, and running them
    # serially is slow
    pool = multiprocessing.Pool()
    pool.starmap(test_shards_cover_all_test, itertools.product([self_test_exe], test_cases))

if __name__ == '__main__':
    sys.exit(main())
