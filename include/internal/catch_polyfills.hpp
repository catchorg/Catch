/*
 *  Created by Martin on 17/11/2017.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_POLYFILLS_HPP_INCLUDED
#define TWOBLUECUBES_CATCH_POLYFILLS_HPP_INCLUDED

namespace Catch {
    bool isnan(float f);
    bool isnan(double d);
    bool isnan(long double d);
}

#endif // TWOBLUECUBES_CATCH_POLYFILLS_HPP_INCLUDED
