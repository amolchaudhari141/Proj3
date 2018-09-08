package main

import (
	"fmt"
	"net"
	"strings"
)

func CheckError(err error) {
	if err != nil {
		fmt.Println("Error: ", err)
	}
}

func sendResponse(conn *net.UDPConn, addr *net.UDPAddr, msg string) {
	_, err := conn.WriteToUDP([]byte(msg), addr)
	if err != nil {
		fmt.Printf("Couldn't send response %v", err)
	}
}

func main() {
	// Prepare an address at any address at port 33104
	ServerAddr, err := net.ResolveUDPAddr("udp", ":33104")
	CheckError(err)

	// Listen
	ServerCLNS, err := net.ListenUDP("udp", ServerAddr)
	CheckError(err)
	defer ServerCLNS.Close()

	buf := make([]byte, 1024)

	services := make(map[string]string)

	for {
		returnMsg := ""
		n, remoteAddr, err := ServerCLNS.ReadFromUDP(buf)
		CheckError(err)

		displayAddr := strings.SplitAfterN(remoteAddr.String(), ":", 2)
		msgArray := strings.Split(string(buf[0:n]), " ")
		if len(msgArray) > 0 {
			msgType := msgArray[0]
			fmt.Println("Received from", displayAddr[0], string(buf[0:n]))
			if !strings.EqualFold(msgType, "PUT") && !strings.EqualFold(msgType, "GET") {
				fmt.Println("Error: invalid request type")
				returnMsg = "NOT OK"
			}
			if strings.EqualFold(msgType, "PUT") {
				if len(msgArray) != 3 {
					fmt.Println("Error: invalid request type")
					returnMsg = "NOT OK"
				} else {
					services[msgArray[1]] = msgArray[2]
					returnMsg = "OK"
				}
			}
			if strings.EqualFold(msgType, "GET") {
				if len(msgArray) != 2 {
					fmt.Println("Error: invalid request type")
				} else {
					if val, ok := services[msgArray[1]]; ok {
						returnMsg = val
					} else {
						returnMsg = "NOT OK"
						fmt.Println("Error: Request service name does not exist")
					}
				}
			}

			if !strings.EqualFold(returnMsg, "") {
				go sendResponse(ServerCLNS, remoteAddr, returnMsg)
				CheckError(err)
			}
		}
	}
}
