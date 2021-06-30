<a id="top"></a>
# Changes overview
- Catch2 now suggests statically compiled library as its distribution model.
- Functionality is split into multiple header files.
- ...

# Migrating from v2 to v3

If you want to migrate to v3, there are two basic approaches to do so.

1. Use `catch_amalgamated.hpp` and `catch_amalgamated.cpp`.
2. Build Catch2 as a proper (static) library, and move to piecewise headers

Doing 1 means downloading the [amalgamated header](/extras/catch_amalgamated.hpp)
and the [amalgamated sources](/extras/catch_amalgamated.cpp) from `extras`,
dropping them into your test project, and rewriting your includes from
`<catch2/catch.hpp>` to `"catch_amalgamated.hpp"` (or something similar,
based on how you set up your paths).

The disadvantage of using this approach are increased compilation times,
at least compared to the second approach, but it does let you avoid
dealing with consuming libraries in your build system of choice.


However, we recommend doing 2, and taking extra time to migrate to v3
properly. This lets you reap the benefits of significantly improved
compilation times in the v3 version. The basic steps to do so are:

1. Change your CMakeLists.txt to link against `Catch2WithMain` target if
you use Catch2's default main. (If you do not, keep linking against
the `Catch2` target.)
2. Delete TU with `CATCH_CONFIG_RUNNER` or `CATCH_CONFIG_MAIN` defined,
as it is no longer needed.
3. Change `#include <catch2/catch.hpp>` to `#include <catch2/catch_all.hpp>`
4. Check that everything compiles. You might have to modify namespaces,
or perform some other changes (see the
[Things that can break during porting](#things-that-can-break-during-porting)
section for the most common things).
5. Start migrating your test TUs from including `<catch2/catch_all.hpp>`
to piecemeal includes. You will likely want to start by including
`<catch2/catch_test_macros.hpp>`, and then go from there. (see
[other notes](#other-notes) for further ideas)

## Other notes
* The main test include is now `<catch2/catch_test_macros.hpp>`
* Big "subparts" like Matchers, or Generators, have their own folder, and
also their own "big header", so if you just want to include all matchers,
you can include `<catch2/matchers/catch_matchers_all.hpp>`,
or `<catch2/generators/catch_generators_all.hpp>`


## Things that can break during porting
* The namespace on Matchers were cleaned up, they are no longer first declared
deep within an internal namespace and then brought up. All Matchers now live
in `Catch::Matchers`.
* The reporter interfaces changed in a breaking manner. If you wrote custom
reporter or listener, you might need to modify them a bit.


---

[Home](Readme.md#top)
