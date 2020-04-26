/* math.h
   Rémi Attab (remi.attab@gmail.com), 26 Apr 2020
   FreeBSD-style copyright and disclaimer apply
*/

#pragma once

// -----------------------------------------------------------------------------
// min/max
// -----------------------------------------------------------------------------

#define pond_min(x, y)                          \
    ({                                          \
        typeof(x) x_ = (x);                     \
        typeof(y) y_ = (y);                     \
        x_ <= y_ ? x_ : y_;                     \
    })


#define pond_max(x, y)                          \
    ({                                          \
        typeof(x) x_ = (x);                     \
        typeof(y) y_ = (y);                     \
        x_ >= y_ ? x_ : y_;                     \
    })
