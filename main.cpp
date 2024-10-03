#include <catch2/catch_session.hpp>

#include "referencesemantics/observerexamples_referencesemantics.h"
#include "valuesemantics/observerexamples_valuesemantics.h"

int main(const int argc, const char* const argv[])
{
    return Catch::Session().run(argc, argv);
}
