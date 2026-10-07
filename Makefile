IMAGE_NAME ?= mesh-processing
MESH       ?= data/unit_square_132.meshb
RENDER_GID ?= $(shell stat -c '%g' /dev/dri/renderD128 2>/dev/null || echo 992)


# Common Docker run wrappers
DOCKER_RUN_AMD = docker run --rm \
	--device=/dev/kfd --device=/dev/dri --security-opt seccomp=unconfined \
	--group-add video --group-add $(RENDER_GID) \
	-e HSA_OVERRIDE_GFX_VERSION=$${HSA_OVERRIDE_GFX_VERSION:-11.0.0} \
	-v $$(pwd)/data:/app/data

DOCKER_RUN_NVIDIA = docker run --rm \
	--gpus all \
	-v $$(pwd)/data:/app/data

DOCKER_RUN_CPU = docker run --rm \
	-v $$(pwd)/data:/app/data

.PHONY: all help clean bench \
        amd nvidia cpu hip cuda \
        test test-cpu test-amd test-hip test-nvidia \
        docker-amd docker-nvidia docker-cpu docker-hip docker-cuda \
        test-docker-amd test-docker-cpu

all: help

help:
	@echo "Local execution (Host):"
	@echo "  make amd              # Build & run on AMD GPU (ROCm / HIP)"
	@echo "  make nvidia           # Build & run on NVIDIA GPU (CUDA)"
	@echo "  make cpu              # Build & run on CPU (Serial)"
	@echo "  make bench            # Benchmark Hash Map vs Half-Edges"
	@echo ""
	@echo "Tests & Validation (CTest):"
	@echo "  make test             # Run test suite on CPU (Serial)"
	@echo "  make test-amd         # Run test suite on AMD GPU (ROCm / HIP)"
	@echo "  make test-docker-amd  # Run test suite in Docker (AMD GPU)"
	@echo "  make test-docker-cpu  # Run test suite in Docker (CPU)"
	@echo ""
	@echo "Docker execution:"
	@echo "  make docker-amd       # Run on AMD GPU in Docker"
	@echo "  make docker-nvidia    # Run on NVIDIA GPU in Docker"
	@echo "  make docker-cpu       # Run on CPU in Docker"
	@echo ""
	@echo "Option:"
	@echo "  MESH=<path>           # e.g. make bench MESH=data/unit_square_1m.meshb"


# ==============================================================================
# Local Execution
# ==============================================================================

amd:
	@if [ ! -d "build-hip" ]; then cmake --preset hip; fi
	cmake --build --preset hip
	./build-hip/app $(MESH)

hip: amd

nvidia:
	@if [ ! -d "build-cuda" ]; then cmake --preset cuda; fi
	cmake --build --preset cuda
	./build-cuda/app $(MESH)

cuda: nvidia

cpu:
	@if [ ! -d "build" ]; then cmake --preset default; fi
	cmake --build --preset default
	./build/app $(MESH)

bench:
	@if [ ! -d "build" ]; then cmake --preset default; fi
	cmake --build --preset default --target benchmark
	./build/benchmark $(if $(filter data/unit_square_132.meshb,$(MESH)),data/unit_square_1m.meshb,$(MESH))

# ==============================================================================
# Tests (CTest)
# ==============================================================================

test: test-cpu

test-cpu:
	@if [ ! -d "build" ]; then cmake --preset default; fi
	cmake --build --preset default
	ctest --preset default

test-amd:
	@if [ ! -d "build-hip" ]; then cmake --preset hip; fi
	cmake --build --preset hip
	ctest --preset hip

test-hip: test-amd

test-nvidia:
	@if [ ! -d "build-cuda" ]; then cmake --preset cuda; fi
	cmake --build --preset cuda
	ctest --preset cuda

# ==============================================================================
# Docker Execution
# ==============================================================================

docker-amd:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):hip 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):hip (ROCm base image)..."; \
		docker build --build-arg BASE_IMAGE=rocm/dev-ubuntu-24.04:latest --build-arg PRESET=hip -t $(IMAGE_NAME):hip .; \
	fi
	$(DOCKER_RUN_AMD) $(IMAGE_NAME):hip $(MESH)

docker-hip: docker-amd

docker-nvidia:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):cuda 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):cuda (CUDA base image)..."; \
		docker build --build-arg BASE_IMAGE=nvidia/cuda:12.6.0-devel-ubuntu24.04 --build-arg PRESET=cuda -t $(IMAGE_NAME):cuda .; \
	fi
	$(DOCKER_RUN_NVIDIA) $(IMAGE_NAME):cuda $(MESH)

docker-cuda: docker-nvidia

docker-cpu:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):default 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):default (CPU base image)..."; \
		docker build --build-arg BASE_IMAGE=ubuntu:24.04 --build-arg PRESET=default -t $(IMAGE_NAME):default .; \
	fi
	$(DOCKER_RUN_CPU) $(IMAGE_NAME):default $(MESH)

test-docker-amd:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):hip 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):hip (ROCm base image)..."; \
		docker build --build-arg BASE_IMAGE=rocm/dev-ubuntu-24.04:latest --build-arg PRESET=hip -t $(IMAGE_NAME):hip .; \
	fi
	$(DOCKER_RUN_AMD) --entrypoint ctest $(IMAGE_NAME):hip --preset hip

test-docker-cpu:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):default 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):default (CPU base image)..."; \
		docker build --build-arg BASE_IMAGE=ubuntu:24.04 --build-arg PRESET=default -t $(IMAGE_NAME):default .; \
	fi
	$(DOCKER_RUN_CPU) --entrypoint ctest $(IMAGE_NAME):default --preset default

# ==============================================================================
# Cleanup
# ==============================================================================

clean:
	rm -rf build build-hip build-cuda
