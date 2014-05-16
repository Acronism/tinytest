tinytest
========

A lightweight unit testing framework written in C++.

Building
========

There are a few options available for building tinytest: Boost Build, Make, and Visual Studio. Building should be simple for anyone who's used those utilities before.

The output for 'nix builds will be put the headers and library files into include and lib folders, respectively.

Visual Studio will do its own thing. VS is mostly provided for compatibility though isn't tested as regularly. If there's a problem, please do put in a pull request so that we can get it working for everyone!

Usage
=====

To create unit tests, simply include `tinytest.h` and some `TEST_SUITE`s to contain your tests. All of these should be compiled into a separate executable as tinytest contains its own `main` function.

    #include <tinytest/tinytest.h>

    TEST_SUITE("Math Test") {
        TEST("1 + 1 = 2") {
            CHECK(1 + 1 == 2);
        };

        TEST("1 - 1 = 0") {
            CHECK(1 - 1 == 0);
        };
    };

Note the semicolons after each `TEST_SUITE` and `TEST` block. `CHECK` accepts any boolean convertible expression.

To run your tests, simply compile your program and run. The tinytest framework will already have a main function ready for you which means that you can include any number of test files.

    ===========================================================
    Running test suite Math Test...
    Testing 1 + 1 = 2...
    Testing 1 - 1 = 0...
    All 2 checks passed!
    ========================= SUMMARY =========================
    Total checks performed: 2
    Total checks failed: 0
    ===========================================================

Tinytest also support setup/teardown routines, testing for exceptions, and aborting suites with custom messages.

    TEST_SUITE("Abort Suite") {
        SETUP() {
            ABORT_SUITE("Aborting suite with message test");
        };

        TEST("Hello World") {
            std::vector v = {1, 2, 3};
            CHECK_EXCEPTION(v.at(4), std::exception);
        };
    };

To see more examples of usage, take a look at the examples folder.

How it works
============

Tinytest uses the magic of static initialization to instantiate a series of lambda-like objects upon program execution. This is why you don't need to provide any headers or explicitly invoke the tests cases and why tinytest doesn't need to know about your test suites prior to execution. When the program is invoked, all of the `TEST_SUITE` objects are added to a global container which is then traversed in main.

This is obviously not a good design pattern for many products and thus such voodoo magic should probably be avoided in most cases. In this framework, however, the magic is contained which allows for test suites to be written quickly. The end product promotes TDD and makes TDD easy to work with.