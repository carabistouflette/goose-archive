GOOSE ?= .toolchain/goose/build/goose
CC ?= cc
SANITIZER_CC ?= clang
FUZZ_CC ?= clang
FUZZ_RUNS ?= 10000
BENCH_ITERATIONS ?= 200

FAULT_WRAPS = -Wl,--wrap=write,--wrap=close,--wrap=openat,--wrap=mkdirat,--wrap=fcntl

.PHONY: setup format check test fixtures native-test sanitize fuzz bench
setup:
	./scripts/setup-goose.sh
format:
	python3 scripts/native-manifest.py
	"$(GOOSE)" format archive.goose examples tests bench
check:
	python3 scripts/native-manifest.py --check
	"$(GOOSE)" format --check archive.goose examples tests bench
	"$(GOOSE)" check tests/api.goose
	"$(GOOSE)" check examples/inspect.goose
	"$(GOOSE)" check bench/bench.goose
fixtures:
	python3 scripts/fixtures.py
.build/libarchive-test.so: native/archive.c native/archive.h
	mkdir -p .build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared native/archive.c -larchive -lz -o $@
.build/libarchive-faults.so: native/archive.c native/archive.h tests/faults.c
	mkdir -p .build
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -fPIC -shared native/archive.c tests/faults.c $(FAULT_WRAPS) -larchive -lz -o $@
native-test: .build/libarchive-test.so .build/libarchive-faults.so
	python3 -m unittest discover -s tests -v
test: fixtures native-test
	python3 scripts/test-api.py "$(GOOSE)"
	"$(GOOSE)" run examples/inspect.goose -- .build/fixtures/small.tar.gz
sanitize: fixtures
	mkdir -p .build
	$(SANITIZER_CC) -std=c11 -g -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie native/archive.c tests/driver.c tests/faults.c $(FAULT_WRAPS) -larchive -lz -o .build/driver-$(SANITIZER_CC)
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 .build/driver-$(SANITIZER_CC) --selftest
	ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ARCHIVE_DRIVER="$(CURDIR)/.build/driver-$(SANITIZER_CC)" python3 -m unittest discover -s tests -v
fuzz: fixtures
	$(FUZZ_CC) -std=c11 -g -O1 -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer native/archive.c tests/fuzz.c -larchive -lz -o .build/fuzz
	mkdir -p .build/fuzz-artifacts
	UBSAN_OPTIONS=halt_on_error=1 .build/fuzz .build/fuzz-corpus -artifact_prefix=.build/fuzz-artifacts/ -seed=219 -dict=tests/fuzz.dict -max_len=1048576 -runs=$(FUZZ_RUNS)
bench: fixtures
	"$(GOOSE)" build bench/bench.goose -O2 -o .build/bench
	python3 bench/measure.py --goose "$(GOOSE)" --iterations $(BENCH_ITERATIONS) --output .build/benchmark.json
