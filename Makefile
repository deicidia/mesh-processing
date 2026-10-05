IMAGE_NAME ?= mesh-processing
MESH       ?= data/unit_square_132.meshb

.PHONY: all help clean \
        amd nvidia cpu hip cuda \
        test test-cpu test-amd test-hip test-docker-amd test-docker-cpu \
        docker-amd docker-nvidia docker-cpu docker-hip docker-cuda

all: help

help:
	@echo "Local execution (Host):"
	@echo "  make amd               # Build & run on AMD GPU (ROCm / HIP)"
	@echo "  make nvidia            # Build & run on NVIDIA GPU (CUDA)"
	@echo "  make cpu               # Build & run on CPU (Serial)"
	@echo ""
	@echo "Tests & Validation (Kokkos & libMeshb):"
	@echo "  make test              # Run installation tests on CPU"
	@echo "  make test-cpu          # Run installation tests on CPU (Serial)"
	@echo "  make test-amd          # Run installation tests on AMD GPU (ROCm / HIP)"
	@echo "  make test-docker-amd   # Run installation tests in Docker (AMD GPU)"
	@echo "  make test-docker-cpu   # Run installation tests in Docker (CPU)"
	@echo ""
	@echo "Docker execution:"
	@echo "  make docker-amd        # Run in Docker on AMD GPU"
	@echo "  make docker-nvidia     # Run in Docker on NVIDIA GPU"
	@echo "  make docker-cpu        # Run in Docker on CPU"
	@echo ""
	@echo "Option:"
	@echo "  MESH=<path>            # e.g. make amd MESH=data/unit_square_11k.meshb"

# ==============================================================================
# Local Execution (Host CMake)
# ==============================================================================

amd:
	@echo "--> [Local] Building preset 'hip' (AMD GPU)..."
	@if [ ! -d "build-hip" ]; then cmake --preset hip; fi
	cmake --build --preset hip
	@echo "--> [Local] Running on AMD GPU..."
	./build-hip/app $(MESH)

hip: amd

nvidia:
	@echo "--> [Local] Building preset 'cuda' (NVIDIA GPU)..."
	@if [ ! -d "build-cuda" ]; then cmake --preset cuda; fi
	cmake --build --preset cuda
	@echo "--> [Local] Running on NVIDIA GPU..."
	./build-cuda/app $(MESH)

cuda: nvidia

cpu:
	@echo "--> [Local] Building preset 'default' (CPU)..."
	@if [ ! -d "build" ]; then cmake --preset default; fi
	cmake --build --preset default
	@echo "--> [Local] Running on CPU..."
	./build/app $(MESH)

# ==============================================================================
# Installation Tests (Kokkos & libMeshb)
# ==============================================================================

test-cpu:
	@echo "--> [Local] Building preset 'default' (CPU)..."
	@if [ ! -d "build" ]; then cmake --preset default; fi
	cmake --build --preset default --target test_kokkos test_meshb
	@echo ""
	@echo "=================================================="
	@echo " [1/2] Test Kokkos (Configuration & Exécution)"
	@echo "=================================================="
	./build/test_kokkos
	@echo ""
	@echo "=================================================="
	@echo " [2/2] Test libMeshb (Lecture de maillage)"
	@echo "=================================================="
	./build/test_meshb $(MESH)

test-amd:
	@echo "--> [Local] Building preset 'hip' (AMD GPU)..."
	@if [ ! -d "build-hip" ]; then cmake --preset hip; fi
	cmake --build --preset hip --target test_kokkos test_meshb
	@echo ""
	@echo "=================================================="
	@echo " [1/2] Test Kokkos (AMD GPU / HIP)"
	@echo "=================================================="
	./build-hip/test_kokkos
	@echo ""
	@echo "=================================================="
	@echo " [2/2] Test libMeshb (AMD GPU / HIP)"
	@echo "=================================================="
	./build-hip/test_meshb $(MESH)

test-hip: test-amd

test-docker-amd:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):hip 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):hip (ROCm base image)..."; \
		docker build --build-arg BASE_IMAGE=rocm/dev-ubuntu-24.04:latest --build-arg PRESET=hip -t $(IMAGE_NAME):hip .; \
	fi
	@echo "--> [Docker] Running installation tests on AMD GPU..."
	@echo ""
	@echo "=================================================="
	@echo " [1/2] Test Kokkos (Docker AMD GPU / HIP)"
	@echo "=================================================="
	docker run --rm \
		--device=/dev/kfd --device=/dev/dri --security-opt seccomp=unconfined \
		--group-add video --group-add $(RENDER_GID) \
		-e HSA_OVERRIDE_GFX_VERSION=$${HSA_OVERRIDE_GFX_VERSION:-11.0.0} \
		--entrypoint /app/build-hip/test_kokkos \
		$(IMAGE_NAME):hip
	@echo ""
	@echo "=================================================="
	@echo " [2/2] Test libMeshb (Docker AMD GPU / HIP)"
	@echo "=================================================="
	docker run --rm \
		-v $$(pwd)/data:/app/data \
		--entrypoint /app/build-hip/test_meshb \
		$(IMAGE_NAME):hip \
		$(MESH)

test-docker-cpu:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):default 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):default (CPU base image)..."; \
		docker build --build-arg BASE_IMAGE=ubuntu:24.04 --build-arg PRESET=default -t $(IMAGE_NAME):default .; \
	fi
	@echo "--> [Docker] Running installation tests on CPU..."
	@echo ""
	@echo "=================================================="
	@echo " [1/2] Test Kokkos (Docker CPU)"
	@echo "=================================================="
	docker run --rm \
		--entrypoint /app/build/test_kokkos \
		$(IMAGE_NAME):default
	@echo ""
	@echo "=================================================="
	@echo " [2/2] Test libMeshb (Docker CPU)"
	@echo "=================================================="
	docker run --rm \
		-v $$(pwd)/data:/app/data \
		--entrypoint /app/build/test_meshb \
		$(IMAGE_NAME):default \
		$(MESH)

test: test-cpu

# ==============================================================================
# Docker Execution
# ==============================================================================

RENDER_GID ?= $(shell stat -c '%g' /dev/dri/renderD128 2>/dev/null || echo 992)

docker-amd:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):hip 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):hip (ROCm base image)..."; \
		docker build --build-arg BASE_IMAGE=rocm/dev-ubuntu-24.04:latest --build-arg PRESET=hip -t $(IMAGE_NAME):hip .; \
	fi
	@echo "--> [Docker] Running on AMD GPU (ROCm / HIP)..."
	docker run --rm \
		--device=/dev/kfd --device=/dev/dri --security-opt seccomp=unconfined \
		--group-add video --group-add $(RENDER_GID) \
		-e HSA_OVERRIDE_GFX_VERSION=$${HSA_OVERRIDE_GFX_VERSION:-11.0.0} \
		-v $$(pwd)/data:/app/data \
		$(IMAGE_NAME):hip \
		$(MESH)

docker-hip: docker-amd

docker-nvidia:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):cuda 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):cuda (CUDA base image)..."; \
		docker build --build-arg BASE_IMAGE=nvidia/cuda:12.6.0-devel-ubuntu24.04 --build-arg PRESET=cuda -t $(IMAGE_NAME):cuda .; \
	fi
	@echo "--> [Docker] Running on NVIDIA GPU (CUDA)..."
	docker run --rm \
		--gpus all \
		-v $$(pwd)/data:/app/data \
		$(IMAGE_NAME):cuda \
		$(MESH)

docker-cuda: docker-nvidia

docker-cpu:
	@if [ -z "$$(docker images -q $(IMAGE_NAME):default 2> /dev/null)" ]; then \
		echo "--> [Docker] Building $(IMAGE_NAME):default (CPU base image)..."; \
		docker build --build-arg BASE_IMAGE=ubuntu:24.04 --build-arg PRESET=default -t $(IMAGE_NAME):default .; \
	fi
	@echo "--> [Docker] Running on CPU..."
	docker run --rm \
		-v $$(pwd)/data:/app/data \
		$(IMAGE_NAME):default \
		$(MESH)

# ==============================================================================
# Cleanup
# ==============================================================================

clean:
	rm -rf build build-hip build-cuda
