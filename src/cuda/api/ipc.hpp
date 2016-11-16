#pragma once
#ifndef CUDA_API_WRAPPERS_IPC_HPP_
#define CUDA_API_WRAPPERS_IPC_HPP_

#include "cuda/api/error.hpp"
#include "cuda/api/types.h"

#include <cuda_runtime_api.h>

#include <string>

namespace cuda {
namespace memory {
namespace ipc {

using handle_t = cudaIpcMemHandle_t;

/**
 * Obtain a handle for a region of on-device memory which can
 * be transmitted for use in another operating system process
 *
 * @note The name contains an underscore so as not to clash
 * with the C++ reserved word export
 *
 * @param device_ptr beginning of the region of memory
 * to be shared with other processes
 * @return a handle which another process can call @ref import
 * on to obtain a device pointer it can use
 */
handle_t export_(void* device_ptr) {
	handle_t handle;
	auto status = cudaIpcGetMemHandle(&handle, device_ptr);
		throw_if_error(status,
			"Failed producing an IPC memory handle for device pointer " + cuda::detail::ptr_as_hex(device_ptr));
	return handle;
}

template <typename T = void>
inline T* import(const handle_t& handle)
{
	void* device_ptr;
	auto status = cudaIpcOpenMemHandle(&device_ptr, handle, cudaIpcMemLazyEnablePeerAccess);
	throw_if_error(status,
		"Failed obtaining a device pointer from an IPC memory handle");
	return reinterpret_cast<T*>(device_ptr);
}

void unmap(void* ipc_mapped_ptr)
{
	auto status = cudaIpcCloseMemHandle(ipc_mapped_ptr);
	throw_if_error(status, "Failed unmapping IPC memory mapped to " + cuda::detail::ptr_as_hex(ipc_mapped_ptr));
}

template <typename T = void>
class imported_t {
public: // constructors & destructor
	imported_t(const handle_t& handle) : ptr_(import<T>(handle))
	{
		if (ptr_ == nullptr) {
			throw std::logic_error("IPC memory handle yielded a null pointer");
		}
	}

	/**
	 * @note May throw! Be very careful.
	 */
	~imported_t() {
		if (ptr_ == nullptr) { return; }
		unmap(ptr_);
	}

public: // operators

	imported_t(const imported_t& other) = delete;
	imported_t& operator=(const imported_t& other) = delete;
	imported_t& operator=(imported_t&& other) = delete;
	imported_t(const imported_t&& other) = delete;

	operator T*() const { return ptr_; }

public: // getters

	T* get() const { return ptr_; }

protected: // data members
	/**
	 * Also used to indicate ownership of the handle; if it's nullptr,
	 * ownership has passed to another imported_t and we don't need
	 * to close the handle
	 */
	T*         ptr_;
}; // class imported_t

} // namespace ipc
} // namespace memory

namespace event {
namespace ipc {

using handle_t = cudaIpcEventHandle_t;

handle_t export_(id_t event_id)
{
	handle_t ipc_handle;
	auto status = cudaIpcGetEventHandle(&ipc_handle, event_id);
	throw_if_error(status,
		"Failed obtaining an IPC event handle for event " + cuda::detail::ptr_as_hex(event_id));
	return ipc_handle;
}

inline event::id_t import(const handle_t& handle)
{
	event::id_t event_id;
	auto status = cudaIpcOpenEventHandle(&event_id, handle);
	throw_if_error(status,
		"Failed obtaining an event ID from an IPC event handle");
	return event_id;
}

} // namespace ipc
} // namespace event
} // namespace cuda

#endif /* CUDA_API_WRAPPERS_IPC_HPP_ */
