package main

import (
	"fmt"
	"os"
	"syscall"
	"time"
	"unsafe"
)

const (
	SHM_NAME    = "JIT_CORE_OPEN_MEM"
	SHM_SIZE    = 65536
	QUEUE_SIZE  = 30
	PACKET_SIZE = 512
	SEM_PROD    = "Global\\SemProd"
	SEM_CONS    = "Global\\SemCons"
)

type SharedQueue struct {
	Head     int32
	Tail     int32
	Count    int32
	Lengths  [QUEUE_SIZE]int32
	Packets  [QUEUE_SIZE][PACKET_SIZE]byte
	Shutdown int32
}

func main() {
	kernel32 := syscall.NewLazyDLL("kernel32.dll")
	openFileMapping := kernel32.NewProc("OpenFileMappingW")
	mapViewOfFile := kernel32.NewProc("MapViewOfFile")
	closeHandle := kernel32.NewProc("CloseHandle")
	unmapViewOfFile := kernel32.NewProc("UnmapViewOfFile")
	openSemaphore := kernel32.NewProc("OpenSemaphoreW")
	waitForSingleObject := kernel32.NewProc("WaitForSingleObject")
	releaseSemaphore := kernel32.NewProc("ReleaseSemaphore")

	file, err := os.OpenFile("build/test-run.pmo", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		fmt.Printf("Failed to open output file: %v\n", err)
		return
	}
	defer file.Close()

	hMap, _, err := openFileMapping.Call(0x000F001F, 0, uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(SHM_NAME))))
	if hMap == 0 {
		fmt.Printf("Failed to open shared memory: %v\n", err)
		return
	}
	defer closeHandle.Call(hMap)

	ptr, _, err := mapViewOfFile.Call(hMap, 0x000F001F, 0, 0, SHM_SIZE)
	if ptr == 0 {
		fmt.Printf("Failed to map shared memory: %v\n", err)
		return
	}
	defer unmapViewOfFile.Call(ptr)

	var semProd, semCons uintptr
	for retries := 0; retries < 5; retries++ {
		semProd, _, err = openSemaphore.Call(0x1F0003, 0, uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(SEM_PROD))))
		if semProd != 0 {
			break
		}
		fmt.Printf("Failed to open semaphore (SemProd), retrying (%d/5): %v\n", retries+1, err)
		time.Sleep(500 * time.Millisecond)
	}
	if semProd == 0 {
		fmt.Printf("Failed to open semaphore (SemProd) after retries: %v\n", err)
		return
	}
	defer closeHandle.Call(semProd)

	for retries := 0; retries < 5; retries++ {
		semCons, _, err = openSemaphore.Call(0x1F0003, 0, uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(SEM_CONS))))
		if semCons != 0 {
			break
		}
		fmt.Printf("Failed to open semaphore (SemCons), retrying (%d/5): %v\n", retries+1, err)
		time.Sleep(500 * time.Millisecond)
	}
	if semCons == 0 {
		fmt.Printf("Failed to open semaphore (SemCons) after retries: %v\n", err)
		return
	}
	defer closeHandle.Call(semCons)

	queue := (*SharedQueue)(unsafe.Pointer(ptr))

	fmt.Println("Reading packets from shared memory queue...")

	for {
		_, _, err := waitForSingleObject.Call(semCons, 0xFFFFFFFF)
		if err != nil && err != syscall.Errno(0) {
			fmt.Printf("WaitForSingleObject failed: %v\n", err)
			return
		}

		if queue.Shutdown != 0 {
			fmt.Println("Shutdown signal received, exiting...")
			break
		}

		if queue.Count > 0 {
			length := queue.Lengths[queue.Head]
			if length > PACKET_SIZE {
				length = PACKET_SIZE
			}
			packet := queue.Packets[queue.Head][:length]
			queue.Head = (queue.Head + 1) % QUEUE_SIZE
			queue.Count--

			description := fmt.Sprintf(
				"Packet received at %s\n  Queue Head: %d\n  Queue Count: %d\n  Length: %d bytes\n  Data: % X\n\n",
				time.Now().Format(time.RFC3339), queue.Head, queue.Count, length, packet)

			if _, err := file.WriteString(description); err != nil {
				fmt.Printf("Failed to write to file: %v\n", err)
			}
			fmt.Print(description)

			_, _, err := releaseSemaphore.Call(semProd, 1, 0)
			if err != nil && err != syscall.Errno(0) {
				fmt.Printf("ReleaseSemaphore failed: %v\n", err)
				return
			}
		} else {
			fmt.Println("Queue count is 0, but semaphore signaled. Possible race condition.")
		}

		time.Sleep(10 * time.Millisecond)
	}
}
