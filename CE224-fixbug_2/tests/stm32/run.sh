#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
build_dir=$(mktemp -d /tmp/ce224-stm32-tests.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
"${CC:-cc}" -std=c11 -Wall -Wextra -Wno-misleading-indentation ${SANITIZERS:--fsanitize=undefined} \
  -I tests/stm32 -I Xe_Can_Bang/Core/Inc -include tests/stm32/hal_mock.h \
  tests/stm32/regression.c Xe_Can_Bang/Core/Src/bmi160.c \
  Xe_Can_Bang/Core/Src/filter.c Xe_Can_Bang/Core/Src/encoder.c \
  -lm -o "$build_dir/regression"
"$build_dir/regression"
