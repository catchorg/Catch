/*
 *  Created by Phil Nash on 21/02/2017.
 *  Copyright (c) 2017 Two Blue Cubes Ltd. All rights reserved.
 *
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_MATCHERS_VECTOR_H_INCLUDED
#define TWOBLUECUBES_CATCH_MATCHERS_VECTOR_H_INCLUDED

#include <catch2/catch_matchers.h>
#include <catch2/catch_approx.h>

#include <algorithm>

namespace Catch {
namespace Matchers {

    namespace Vector {
        template<typename T>
        struct ContainsElementMatcher : MatcherBase<ContainsElementMatcher<T>> {

            ContainsElementMatcher(T const &comparator) : m_comparator( comparator) {}

            bool match(std::vector<T> const &v) const  {
                for (auto const& el : v) {
                    if (el == m_comparator) {
                        return true;
                    }
                }
                return false;
            }

            std::string describe() const  {
                return "Contains: " + ::Catch::Detail::stringify( m_comparator );
            }

            T const& m_comparator;
        };

        template<typename T>
        struct ContainsMatcher : MatcherBase<ContainsMatcher<T>> {

            ContainsMatcher(std::vector<T> const &comparator) : m_comparator( comparator ) {}

            bool match(std::vector<T> const &v) const  {
                // !TBD: see note in EqualsMatcher
                if (m_comparator.size() > v.size())
                    return false;
                for (auto const& comparator : m_comparator) {
                    auto present = false;
                    for (const auto& el : v) {
                        if (el == comparator) {
                            present = true;
                            break;
                        }
                    }
                    if (!present) {
                        return false;
                    }
                }
                return true;
            }
            std::string describe() const  {
                return "Contains: " + ::Catch::Detail::stringify( m_comparator );
            }

            std::vector<T> const& m_comparator;
        };

        template<typename T>
        struct EqualsMatcher : MatcherBase<EqualsMatcher<T>> {

            EqualsMatcher(std::vector<T> const &comparator) : m_comparator( comparator ) {}

            bool match(std::vector<T> const &v) const  {
                // !TBD: This currently works if all elements can be compared using !=
                // - a more general approach would be via a compare template that defaults
                // to using !=. but could be specialised for, e.g. std::vector<T> etc
                // - then just call that directly
                if (m_comparator.size() != v.size())
                    return false;
                for (std::size_t i = 0; i < v.size(); ++i)
                    if (m_comparator[i] != v[i])
                        return false;
                return true;
            }
            std::string describe() const  {
                return "Equals: " + ::Catch::Detail::stringify( m_comparator );
            }
            std::vector<T> const& m_comparator;
        };

        template<typename T>
        struct ApproxMatcher : MatcherBase<ApproxMatcher<T>> {

            ApproxMatcher(std::vector<T> const& comparator) : m_comparator( comparator ) {}

            bool match(std::vector<T> const &v) const {
                if (m_comparator.size() != v.size())
                    return false;
                for (std::size_t i = 0; i < v.size(); ++i)
                    if (m_comparator[i] != approx(v[i]))
                        return false;
                return true;
            }
            std::string describe() const {
                return "is approx: " + ::Catch::Detail::stringify( m_comparator );
            }
            template <typename = std::enable_if_t<std::is_constructible<double, T>::value>>
            ApproxMatcher& epsilon( T const& newEpsilon ) {
                approx.epsilon(newEpsilon);
                return *this;
            }
            template <typename = std::enable_if_t<std::is_constructible<double, T>::value>>
            ApproxMatcher& margin( T const& newMargin ) {
                approx.margin(newMargin);
                return *this;
            }
            template <typename = std::enable_if_t<std::is_constructible<double, T>::value>>
            ApproxMatcher& scale( T const& newScale ) {
                approx.scale(newScale);
                return *this;
            }

            std::vector<T> const& m_comparator;
            mutable Catch::Approx approx = Catch::Approx::custom();
        };

        template<typename T>
        struct UnorderedEqualsMatcher : MatcherBase<UnorderedEqualsMatcher<T>> {
            UnorderedEqualsMatcher(std::vector<T> const& target) : m_target(target) {}
            bool match(std::vector<T> const& vec) const  {
                // Note: This is a reimplementation of std::is_permutation,
                //       because I don't want to include <algorithm> inside the common path
                if (m_target.size() != vec.size()) {
                    return false;
                }
                return std::is_permutation(m_target.begin(), m_target.end(), vec.begin());
            }

            std::string describe() const  {
                return "UnorderedEquals: " + ::Catch::Detail::stringify(m_target);
            }
        private:
            std::vector<T> const& m_target;
        };

    } // namespace Vector

    // The following functions create the actual matcher objects.
    // This allows the types to be inferred

    template<typename T>
    Vector::ContainsMatcher<T> Contains( std::vector<T> const& comparator ) {
        return Vector::ContainsMatcher<T>( comparator );
    }

    template<typename T>
    Vector::ContainsElementMatcher<T> VectorContains( T const& comparator ) {
        return Vector::ContainsElementMatcher<T>( comparator );
    }

    template<typename T>
    Vector::EqualsMatcher<T> Equals( std::vector<T> const& comparator ) {
        return Vector::EqualsMatcher<T>( comparator );
    }

    template<typename T>
    Vector::ApproxMatcher<T> Approx( std::vector<T> const& comparator ) {
        return Vector::ApproxMatcher<T>( comparator );
    }

    template<typename T>
    Vector::UnorderedEqualsMatcher<T> UnorderedEquals(std::vector<T> const& target) {
        return Vector::UnorderedEqualsMatcher<T>(target);
    }

} // namespace Matchers
} // namespace Catch

#endif // TWOBLUECUBES_CATCH_MATCHERS_VECTOR_H_INCLUDED
