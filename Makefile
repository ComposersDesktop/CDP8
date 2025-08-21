
.PHONY: all build clean

all: build

build:
	@mkdir -p build && cd build && \
		cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5 && \
		cmake --build . --config Release

clean:
	@rm -rf build


