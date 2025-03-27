package main

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


kernel32 := syscall.NewLazyDLL("kernel32.dll")
openFileMapping := kernel32.NewProc("OpenFileMappingW")
mapViewOfFile := kernel32.NewProc("MapViewOfFile")