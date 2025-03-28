package main

import (
	"fmt"
	"syscall"
	"unsafe"
)

const (
	SHM_NAME    = "JIT_CORE_OPEN_MEM"
	SHM_SIZE    = 16384
	QUEUE_SIZE  = 30
	PACKET_SIZE = 512
)

type SharedQueue struct {
	Head    int32
	Tail    int32
	Count   int32
	Packets [QUEUE_SIZE][PACKET_SIZE]byte
	Mutex   uintptr
}

func main() {
	kernel32 := syscall.NewLazyDLL("kernel32.dll")
	openFileMapping := kernel32.NewProc("OpenFileMappingW")
	mapViewOfFile := kernel32.NewProc("MapViewOfFile")

	hMap, _, _ := openFileMapping.Call(0x0002, 0, uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(SHM_NAME))))
	if hMap == 0 {
		fmt.Println("Failed to open shared memory")
		return
	}

	// Map shared memory to our process
	ptr, _, _ := mapViewOfFile.Call(hMap, 0x0004, 0, 0, SHM_SIZE)
	if ptr == 0 {
		fmt.Println("Failed to map shared memory")
		return
	}

	queue := (*SharedQueue)(unsafe.Pointer(ptr))

	fmt.Println("Reading packets from shared memory queue...")

	for {
		if queue.Count > 0 {
			packet := queue.Packets[queue.Head]
			fmt.Printf("Packet from queue: %s\n", string(packet[:]))
			queue.Head = (queue.Head + 1) % QUEUE_SIZE
			queue.Count--
		}
	}
}
