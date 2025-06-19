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
	SHM_SIZE    = 16384
	QUEUE_SIZE  = 30
	PACKET_SIZE = 512
	SEM_PROD    = "Global\\SemProd"
	SEM_CONS    = "Global\\SemCons"
)

type SharedQueue struct {
	Head     int32
	Tail     int32
	Count    int32
	Packets  [QUEUE_SIZE][PACKET_SIZE]byte
	Shutdown int32 // Add shutdown flag
}

func main() {
	// Load kernel32.dll
	kernel32 := syscall.NewLazyDLL("kernel32.dll")
	openFileMapping := kernel32.NewProc("OpenFileMappingW")
	mapViewOfFile := kernel32.NewProc("MapViewOfFile")
	closeHandle := kernel32.NewProc("CloseHandle")
	unmapViewOfFile := kernel32.NewProc("UnmapViewOfFile")
	openSemaphore := kernel32.NewProc("OpenSemaphoreW")
	waitForSingleObject := kernel32.NewProc("WaitForSingleObject")
	releaseSemaphore := kernel32.NewProc("ReleaseSemaphore")

	// Open output file
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

	// Open semaphores with retry mechanism
	var semProd, semCons uintptr
	for retries := 0; retries < 5; retries++ {
		semProd, _, err = openSemaphore.Call(0x1F0003, 0, uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(SEM_PROD))))
		if semProd != 0 {
			break
		}
		fmt.Printf("Failed to open semaphore (SemProd), retrying (%d/5): %v\n", retries+1, err)
		time.Sleep(500 * time.Millisecond) // Wait 500ms before retrying
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

	// Convert shared memory pointer to SharedQueue struct
	queue := (*SharedQueue)(unsafe.Pointer(ptr))

	fmt.Println("Reading packets from shared memory queue...")

	for {
		// Waiting for packets to be available
		_, _, err := waitForSingleObject.Call(semCons, 0xFFFFFFFF)
		if err != nil && err != syscall.Errno(0) {
			fmt.Printf("WaitForSingleObject failed: %v\n", err)
			return
		}

		// Check for shutdown
		if queue.Shutdown != 0 {
			fmt.Println("Shutdown signal received, exiting...")
			break
		}

		// Check queue count
		if queue.Count > 0 {
			// Dequeue packet
			packet := queue.Packets[queue.Head]
			queue.Head = (queue.Head + 1) % QUEUE_SIZE
			queue.Count--

			// Parse packet (assuming format: 4 bytes ID, 8 bytes timestamp, rest is string)
			var id uint32
			var timestamp int64
			if len(packet) >= 12 {
				id = uint32(packet[0]) | uint32(packet[1])<<8 | uint32(packet[2])<<16 | uint32(packet[3])<<24
				timestamp = int64(packet[4]) | int64(packet[5])<<8 | int64(packet[6])<<16 | int64(packet[7])<<24 |
					int64(packet[8])<<32 | int64(packet[9])<<40 | int64(packet[10])<<48 | int64(packet[11])<<56
			}
			payload := string(packet[12:])

			// Create description
			description := fmt.Sprintf(
				"Packet received at %s\n  Queue Head: %d\n  Queue Count: %d\n  Packet ID: %d\n  Timestamp: %d\n  Payload: %s\n\n",
				time.Now().Format(time.RFC3339), queue.Head, queue.Count, id, timestamp, payload)

			// Write description to file
			if _, err := file.WriteString(description); err != nil {
				fmt.Printf("Failed to write to file: %v\n", err)
			}

			// Print to console
			fmt.Print(description)

			// Signal producer that a slot is available
			_, _, err := releaseSemaphore.Call(semProd, 1, 0)
			if err != nil && err != syscall.Errno(0) {
				fmt.Printf("ReleaseSemaphore failed: %v\n", err)
				return
			}
		} else {
			fmt.Println("Queue count is 0, but semaphore signaled. Possible race condition.")
		}

		// Small delay to avoid busy-looping if no packets
		time.Sleep(10 * time.Millisecond)
	}
}
