
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE_1_0.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <catch2/internal/catch_test_case_registry_impl.hpp>

#include <catch2/internal/catch_context.hpp>
#include <catch2/internal/catch_enforce.hpp>
#include <catch2/interfaces/catch_interfaces_registry_hub.hpp>
#include <catch2/internal/catch_random_number_generator.hpp>
#include <catch2/internal/catch_run_context.hpp>
#include <catch2/internal/catch_sharding.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_spec.hpp>

#include <algorithm>
#include <set>

namespace Catch {

namespace {
    struct TestHasher {
        using hash_t = uint64_t;

        explicit TestHasher( hash_t hashSuffix ):
            m_hashSuffix( hashSuffix ) {}

        uint64_t m_hashSuffix;

        uint32_t operator()( TestCaseInfo const& t ) const {
            // FNV-1a hash with multiplication fold.
            const hash_t prime = 1099511628211u;
            hash_t hash = 14695981039346656037u;
            for (const char c : t.name) {
                hash ^= c;
                hash *= prime;
            }
            hash ^= m_hashSuffix;
            hash *= prime;
            const uint32_t low{ static_cast<uint32_t>(hash) };
            const uint32_t high{ static_cast<uint32_t>(hash >> 32) };
            return low * high;
        }
    };
} // end anonymous namespace

    std::vector<TestCaseHandle> sortTests( IConfig const& config, std::vector<TestCaseHandle> const& unsortedTestCases ) {
        switch (config.runOrder()) {
        case TestRunOrder::Declared:
            return unsortedTestCases;

        case TestRunOrder::LexicographicallySorted: {
            std::vector<TestCaseHandle> sorted = unsortedTestCases;
            std::sort(sorted.begin(), sorted.end());
            return sorted;
        }
        case TestRunOrder::Randomized: {
            seedRng(config);
            TestHasher h{ config.rngSeed() };
            std::vector<std::pair<TestHasher::hash_t, TestCaseHandle>> indexed_tests;
            indexed_tests.reserve(unsortedTestCases.size());

            for (auto const& handle : unsortedTestCases) {
                indexed_tests.emplace_back(h(handle.getTestCaseInfo()), handle);
            }

            std::sort(indexed_tests.begin(), indexed_tests.end());

            std::vector<TestCaseHandle> randomized;
            randomized.reserve(indexed_tests.size());

            for (auto const& indexed : indexed_tests) {
                randomized.push_back(indexed.second);
            }

            return randomized;
        }
        }

        CATCH_INTERNAL_ERROR("Unknown test order value!");
    }

    bool isThrowSafe( TestCaseHandle const& testCase, IConfig const& config ) {
        return !testCase.getTestCaseInfo().throws() || config.allowThrows();
    }

    bool matchTest( TestCaseHandle const& testCase, TestSpec const& testSpec, IConfig const& config ) {
        return testSpec.matches( testCase.getTestCaseInfo() ) && isThrowSafe( testCase, config );
    }

    void enforceNoDuplicateTestCases( std::vector<TestCaseHandle> const& functions ) {
        std::set<TestCaseHandle> seenFunctions;
        for( auto const& function : functions ) {
            auto prev = seenFunctions.insert( function );
            CATCH_ENFORCE( prev.second,
                    "error: TEST_CASE( \"" << function.getTestCaseInfo().name << "\" ) already defined.\n"
                    << "\tFirst seen at " << prev.first->getTestCaseInfo().lineInfo << "\n"
                    << "\tRedefined at " << function.getTestCaseInfo().lineInfo );
        }
    }

    std::vector<TestCaseHandle> filterTests( std::vector<TestCaseHandle> const& testCases, TestSpec const& testSpec, IConfig const& config ) {
        std::vector<TestCaseHandle> filtered;
        filtered.reserve( testCases.size() );
        for (auto const& testCase : testCases) {
            if ((!testSpec.hasFilters() && !testCase.getTestCaseInfo().isHidden()) ||
                (testSpec.hasFilters() && matchTest(testCase, testSpec, config))) {
                filtered.push_back(testCase);
            }
        }
        return createShard(filtered, config);
    }
    std::vector<TestCaseHandle> const& getAllTestCasesSorted( IConfig const& config ) {
        return getRegistryHub().getTestCaseRegistry().getAllTestsSorted( config );
    }

    void TestRegistry::registerTest(Detail::unique_ptr<TestCaseInfo> testInfo, Detail::unique_ptr<ITestInvoker> testInvoker) {
        m_handles.emplace_back(testInfo.get(), testInvoker.get());
        m_viewed_test_infos.push_back(testInfo.get());
        m_owned_test_infos.push_back(std::move(testInfo));
        m_invokers.push_back(std::move(testInvoker));
    }

    std::vector<TestCaseInfo*> const& TestRegistry::getAllInfos() const {
        return m_viewed_test_infos;
    }

    std::vector<TestCaseHandle> const& TestRegistry::getAllTests() const {
        return m_handles;
    }
    std::vector<TestCaseHandle> const& TestRegistry::getAllTestsSorted( IConfig const& config ) const {
        if( m_sortedFunctions.empty() )
            enforceNoDuplicateTestCases( m_handles );

        if(  m_currentSortOrder != config.runOrder() || m_sortedFunctions.empty() ) {
            m_sortedFunctions = sortTests( config, m_handles );
            m_currentSortOrder = config.runOrder();
        }
        return m_sortedFunctions;
    }



    ///////////////////////////////////////////////////////////////////////////
    void TestInvokerAsFunction::invoke() const {
        m_testAsFunction();
    }

} // end namespace Catch
