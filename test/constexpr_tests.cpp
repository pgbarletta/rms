#include <catch2/catch_test_macros.hpp>

#include "include/parsers.hpp"

TEST_CASE("Parm7 constants are constexpr", "[constexpr]") {
  STATIC_REQUIRE(rms::kAmberChargeScale > 18.0);
  STATIC_REQUIRE(rms::kParm7PointerCount == 31);
}
